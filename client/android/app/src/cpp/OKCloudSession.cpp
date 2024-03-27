//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ok_defines.h"

#include <openxr/openxr.h>

#ifndef XR_USE_GRAPHICS_API_OPENGL_ES
#define XR_USE_GRAPHICS_API_OPENGL_ES
#endif

#ifndef XR_USE_PLATFORM_ANDROID
#define XR_USE_PLATFORM_ANDROID
#endif

#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

class _jobject;
typedef _jobject*       jobject;
#include <openxr/openxr_platform.h>

#include "shell/openxr/mobile/opengl/XrAppImplGLES.h"

#include "IGLU/managedUniformBuffer/ManagedUniformBuffer.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <glm/detail/qualifier.hpp>
#include <igl/NameHandle.h>
#include <igl/ShaderCreator.h>
#include <igl/opengl/Device.h>
#include <igl/opengl/GLIncludes.h>
#include <igl/opengl/RenderCommandEncoder.h>
#include <igl/Log.h>

#include <glm/glm/gtc/quaternion.hpp>
#include "CloudXRMatrixHelpers.h"
#include <igl/opengl/egl/Context.h>

#include "OKCloudSession.h"
#include "shell/shared/renderSession/ShellParams.h"

#define EXTERNAL_XR_BUILD
#undef IGL_DEBUG
#include "shell/openxr/mobile/XrApp.cpp"
#include "shell/openxr/src/XrSwapchainProvider.cpp"
#undef EXTERNAL_XR_BUILD

#ifndef XR_USE_TIMESPEC
#define XR_USE_TIMESPEC
#endif

namespace igl::shell
{
struct VertexPosUvw
{
    glm::vec3 position;
    glm::vec3 uvw;
};

const float half = 1.0f;

VertexPosUvw vertexData0[] =
{
{{-half, half, -half}, {0.0, 1.0, 0.0}},
{{half, half, -half}, {1.0, 1.0, 0.0}},
{{-half, -half, -half}, {0.0, 0.0, 0.0}},
{{half, -half, -half}, {1.0, 0.0, 0.0}},
{{half, half, half}, {1.0, 1.0, 1.0}},
{{-half, half, half}, {0.0, 1.0, 1.0}},
{{half, -half, half}, {1.0, 0.0, 1.0}},
{{-half, -half, half}, {0.0, 0.0, 1.0}},
};

uint16_t indexData[] = {0, 1, 2, 1, 3, 2, 1, 4, 3, 4, 6, 3, 4, 5, 6, 5, 7, 6,
                        5, 0, 7, 0, 2, 7, 5, 4, 0, 4, 1, 0, 2, 3, 7, 3, 6, 7};

std::string getProlog(igl::IDevice& device)
{
#if IGL_BACKEND_OPENGL
    const auto shaderVersion = device.getShaderVersion();
    if (shaderVersion.majorVersion >= 3 || shaderVersion.minorVersion >= 30)
    {
        std::string prependVersionString = igl::opengl::getStringFromShaderVersion(shaderVersion);
        prependVersionString += "\n#extension GL_OVR_multiview2 : require\n";
        prependVersionString += "\nprecision highp float;\n";
        return prependVersionString;
    }
#endif // IGL_BACKEND_OPENGL
    return "";
};

std::string getOpenGLFragmentShaderSource(igl::IDevice& device)
{
    return getProlog(device) + std::string(R"(
                  precision highp float;
                  precision highp sampler2D;
                  in vec3 uvw;
                  in vec3 color;
                  uniform sampler2D inputImage;
                  out vec4 fragmentColor;
                  void main() {
                    fragmentColor = texture(inputImage, uvw.xy) * vec4(color, 1.0);
                  })");
}

std::string getOpenGLVertexShaderSource(igl::IDevice& device)
{
    return getProlog(device) + R"(
                  layout(num_views = 2) in;
                  precision highp float;

                  uniform mat4 modelMatrix;
                  uniform mat4 viewProjectionMatrix[2];
                  uniform float scaleZ;

                  in vec3 position;
                  in vec3 uvw_in;
                  out vec3 uvw;
                  out vec3 color;

                  void main() {
                    mat4 mvpMatrix = viewProjectionMatrix[gl_ViewID_OVR] * modelMatrix;
                    gl_Position = mvpMatrix * vec4(position, 1.0);
                    uvw = vec3(uvw_in.x, uvw_in.y, (uvw_in.z - 0.5) * scaleZ + 0.5);
                    color = vec3(1.0, gl_ViewID_OVR, 0.0);
                  })";
}

static const char* getVulkanFragmentShaderSource()
{
    return R"(
                  precision highp float;
                  precision highp sampler2D;

                  layout(location = 0) in vec3 uvw;
                  layout(location = 1) in vec3 color;
                  layout(set = 0, binding = 0) uniform sampler2D inputImage;
                  layout(location = 0) out vec4 fragmentColor;

                  void main() {
                    fragmentColor = texture(inputImage, uvw.xy) * vec4(color, 1.0);
                  })";
}

static const char* getVulkanVertexShaderSource()
{
    return R"(
                  #extension GL_OVR_multiview2 : require
                  layout(num_views = 2) in;
                  precision highp float;

                  layout (set = 1, binding = 1, std140) uniform PerFrame {
                    mat4 modelMatrix;
                    mat4 viewProjectionMatrix[2];
                    float scaleZ;
                  } perFrame;

                  layout(location = 0) in vec3 position;
                  layout(location = 1) in vec3 uvw_in;
                  layout(location = 0) out vec3 uvw;
                  layout(location = 1) out vec3 color;

                  void main() {
                    mat4 mvpMatrix = perFrame.viewProjectionMatrix[gl_ViewID_OVR] * perFrame.modelMatrix;
                    gl_Position = mvpMatrix * vec4(position, 1.0);
                    uvw = vec3(uvw_in.x, uvw_in.y, (uvw_in.z - 0.5) * perFrame.scaleZ + 0.5);
                    color = vec3(1.0, gl_ViewID_OVR, 0.0);
                  })";
}

std::unique_ptr<igl::IShaderStages> getShaderStagesForBackend(igl::IDevice& device)
{
    switch (device.getBackendType())
    {
        // @fb-only
        // @fb-only
        // @fb-only
        case igl::BackendType::Vulkan:
            return igl::ShaderStagesCreator::fromModuleStringInput(device,
                                                                   getVulkanVertexShaderSource(),
                                                                   "main",
                                                                   "",
                                                                   getVulkanFragmentShaderSource(),
                                                                   "main",
                                                                   "",
                                                                   nullptr);
            return nullptr;
        case igl::BackendType::OpenGL:
            return igl::ShaderStagesCreator::fromModuleStringInput(
                    device,
                    getOpenGLVertexShaderSource(device).c_str(),
                    "main",
                    "",
                    getOpenGLFragmentShaderSource(device).c_str(),
                    "main",
                    "",
                    nullptr);
        default:
            IGL_ASSERT_NOT_REACHED();
            return nullptr;
    }
}

bool isDeviceCompatible(IDevice& device) noexcept
{
    return device.hasFeature(DeviceFeatures::Multiview);
}

glm::mat4 perspectiveAsymmetricFovRH(const igl::shell::Fov& fov, float nearZ, float farZ)
{
    glm::mat4 mat;

    const float tanLeft = tanf(fov.angleLeft);
    const float tanRight = tanf(fov.angleRight);
    const float tanDown = tanf(fov.angleDown);
    const float tanUp = tanf(fov.angleUp);

    const float tanWidth = tanRight - tanLeft;
    const float tanHeight = tanUp - tanDown;

    mat[0][0] = 2.0f / tanWidth;
    mat[1][0] = 0.0f;
    mat[2][0] = (tanRight + tanLeft) / tanWidth;
    mat[3][0] = 0.0f;

    mat[0][1] = 0.0f;
    mat[1][1] = 2.0f / tanHeight;
    mat[2][1] = (tanUp + tanDown) / tanHeight;
    mat[3][1] = 0.0f;

    mat[0][2] = 0.0f;
    mat[1][2] = 0.0f;
    mat[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    mat[3][2] = -2.0f * farZ * nearZ / (farZ - nearZ);

    mat[0][3] = 0.0f;
    mat[1][3] = 0.0f;
    mat[2][3] = -1.0f;
    mat[3][3] = 0.0f;

    return mat;
}

void OKCloudSession::createSamplerAndTextures(const igl::IDevice& device)
{
    SamplerStateDesc samplerDesc;
    samplerDesc.minFilter = samplerDesc.magFilter = SamplerMinMagFilter::Linear;
    samplerDesc.addressModeU = SamplerAddressMode::MirrorRepeat;
    samplerDesc.addressModeV = SamplerAddressMode::MirrorRepeat;
    samplerDesc.addressModeW = SamplerAddressMode::MirrorRepeat;
    samp0_ = device.createSamplerState(samplerDesc, nullptr);

    tex0_ = getPlatform().loadTexture("macbeth.png");
}

OKCloudSession::OKCloudSession(std::shared_ptr<Platform> platform) : RenderSession(std::move(platform))
{
}

void OKCloudSession::initialize() noexcept
{
    auto& device = getPlatform().getDevice();

    if (!isDeviceCompatible(device))
    {
        return;
    }

    // Vertex buffer, Index buffer and Vertex Input
    const BufferDesc vb0Desc =
            BufferDesc(BufferDesc::BufferTypeBits::Vertex, vertexData0, sizeof(vertexData0));

    vb0_ = device.createBuffer(vb0Desc, nullptr);

    const BufferDesc ibDesc =
            BufferDesc(BufferDesc::BufferTypeBits::Index, indexData, sizeof(indexData));

    ib0_ = device.createBuffer(ibDesc, nullptr);

    VertexInputStateDesc inputDesc;
    inputDesc.numAttributes = 2;
    inputDesc.attributes[0].format = VertexAttributeFormat::Float3;
    inputDesc.attributes[0].offset = offsetof(VertexPosUvw, position);
    inputDesc.attributes[0].bufferIndex = 0;
    inputDesc.attributes[0].name = "position";
    inputDesc.attributes[0].location = 0;
    inputDesc.attributes[1].format = VertexAttributeFormat::Float3;
    inputDesc.attributes[1].offset = offsetof(VertexPosUvw, uvw);
    inputDesc.attributes[1].bufferIndex = 0;
    inputDesc.attributes[1].name = "uvw_in";
    inputDesc.attributes[1].location = 1;
    inputDesc.numInputBindings = 1;
    inputDesc.inputBindings[0].stride = sizeof(VertexPosUvw);
    vertexInput0_ = device.createVertexInputState(inputDesc, nullptr);

    createSamplerAndTextures(device);
    shaderStages_ = getShaderStagesForBackend(device);

    // Command queue: backed by different types of GPU HW queues
    const CommandQueueDesc desc{igl::CommandQueueType::Graphics};
    commandQueue_ = device.createCommandQueue(desc, nullptr);

    // Set up vertex uniform data
    vertexParameters_.scaleZ = 1.0f;

    renderPass_.colorAttachments.resize(1);
    renderPass_.colorAttachments[0].loadAction = LoadAction::Clear;
    renderPass_.colorAttachments[0].storeAction = StoreAction::Store;
#if defined(IGL_OPENXR_MR_MODE)
    renderPass_.colorAttachments[0].clearColor = {0.0, 0.0, 1.0, 0.0f};
#else
    renderPass_.colorAttachments[0].clearColor = {0.0, 0.0, 0.0, 0.0f};
#endif
    renderPass_.depthAttachment.loadAction = LoadAction::Clear;
    renderPass_.depthAttachment.clearDepth = 1.0;
}

void OKCloudSession::setVertexParams()
{
    // rotating animation
    static float angle = 0.0f, scaleZ = 1.0f, ss = 0.005f;
    angle += 0.005f;
    scaleZ += ss;
    scaleZ = scaleZ < 0.0f ? 0.0f : (scaleZ > 1.0 ? 1.0f : scaleZ);
    if (scaleZ <= 0.05f || scaleZ >= 1.0f) {
        ss *= -1.0f;
    }

    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)) *
                       glm::rotate(glm::mat4(1.0f), -0.2f, glm::vec3(1.0f, 0.0f, 0.0f));

    vertexParameters_.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.f, 8.0f)) *
                                    rotMat *
                                    glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, scaleZ));

    for (size_t i = 0; i < std::min(shellParams().viewParams.size(), size_t(2)); ++i)
    {
        vertexParameters_.viewProjectionMatrix[i] =
                perspectiveAsymmetricFovRH(shellParams().viewParams[i].fov, 0.1f, 100.0f) *
                shellParams().viewParams[i].viewMatrix;
    }

    vertexParameters_.scaleZ = scaleZ;
}

bool OKCloudSession::pre_update() noexcept
{
    return true;
}

void OKCloudSession::update(igl::SurfaceTextures surfaceTextures) noexcept
{
    auto& device = getPlatform().getDevice();

    if (!isDeviceCompatible(device))
    {
        return;
    }

    igl::Result ret;

    if (framebuffer_ == nullptr)
    {
        igl::FramebufferDesc framebufferDesc;
        framebufferDesc.colorAttachments[0].texture = surfaceTextures.color;
        framebufferDesc.depthAttachment.texture = surfaceTextures.depth;

        framebufferDesc.mode = surfaceTextures.color->getNumLayers() > 1 ? FramebufferMode::Stereo
                                                                         : FramebufferMode::Mono;

        framebuffer_ = getPlatform().getDevice().createFramebuffer(framebufferDesc, &ret);
        IGL_ASSERT(ret.isOk());
        IGL_ASSERT(framebuffer_ != nullptr);
    }
    else
    {
        framebuffer_->updateDrawable(surfaceTextures.color);
    }

#if (ENABLE_CLOUDXR && 1)
    if (!ok_client_.is_cxr_initialized() && ok_client_.is_ready_to_connect())
    {
        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
        igl::shell::openxr::XrInputState& xr_inputs = xr_app.xr_inputs_;

        ok_inputs_.handSubactionPath[BVR::LEFT_CONTROLLER] = xr_inputs.handSubactionPath[BVR::LEFT_CONTROLLER];
        ok_inputs_.handSubactionPath[BVR::RIGHT_CONTROLLER] = xr_inputs.handSubactionPath[BVR::RIGHT_CONTROLLER];

        ok_inputs_.actionSet = xr_inputs.actionSet;
        ok_inputs_.grabAction = xr_inputs.grabAction;
        ok_inputs_.vibrateAction = xr_inputs.vibrateAction;

        ok_inputs_.gripPoseAction = xr_inputs.gripPoseAction;
        ok_inputs_.aimPoseAction = xr_inputs.aimPoseAction;
        ok_inputs_.menuClickAction = xr_inputs.menuClickAction;

        ok_inputs_.triggerClickAction = xr_inputs.triggerClickAction;
        ok_inputs_.triggerTouchAction = xr_inputs.triggerTouchAction;
        ok_inputs_.triggerValueAction = xr_inputs.triggerValueAction;

        ok_inputs_.squeezeClickAction = xr_inputs.squeezeClickAction;
        ok_inputs_.squeezeTouchAction = xr_inputs.squeezeTouchAction;
        ok_inputs_.squeezeValueAction = xr_inputs.squeezeValueAction;
        //ok_inputs_.squeezeForceAction = xr_inputs.squeezeForceAction;

        ok_inputs_.thumbstickTouchAction = xr_inputs.thumbstickTouchAction;
        ok_inputs_.thumbstickClickAction = xr_inputs.thumbstickClickAction;

        ok_inputs_.thumbstickXAction = xr_inputs.thumbstickXAction;
        ok_inputs_.thumbstickYAction = xr_inputs.thumbstickYAction;

        ok_inputs_.thumbRestTouchAction = xr_inputs.thumbRestTouchAction;
        ok_inputs_.thumbRestClickAction = xr_inputs.thumbRestClickAction;
        ok_inputs_.thumbRestForceAction = xr_inputs.thumbRestForceAction;
        ok_inputs_.thumbProximityAction = xr_inputs.thumbProximityAction;

        ok_inputs_.pinchValueAction = xr_inputs.pinchValueAction;
        ok_inputs_.pinchForceAction = xr_inputs.pinchForceAction;

        ok_inputs_.buttonAXClickAction = xr_inputs.buttonAXClickAction;
        ok_inputs_.buttonAXTouchAction = xr_inputs.buttonAXTouchAction;

        ok_inputs_.buttonBYClickAction = xr_inputs.buttonBYClickAction;
        ok_inputs_.buttonBYTouchAction = xr_inputs.buttonBYTouchAction;

        ok_inputs_.trackpadXAction = xr_inputs.trackpadXAction;
        ok_inputs_.trackpadYAction = xr_inputs.trackpadYAction;

        Platform& platform = getPlatform();
        const igl::IDevice& device = platform.getDevice();
        const igl::opengl::Device* gl_device_ptr = (const igl::opengl::Device*)&device;
        const opengl::egl::Context* egl_context_ptr = (opengl::egl::Context*)(&gl_device_ptr->getContext());

        EGLDisplay egl_display = egl_context_ptr->getDisplay();
        EGLContext egl_context = egl_context_ptr->get();

        const bool init_cxr_ok = ok_client_.init_android_gles(this, egl_display, egl_context);

        if (init_cxr_ok && ok_client_.ok_config_.enable_auto_connect_)
        {
            ok_client_.connect();
        }
    }

    const int view_id = shellParams().current_view_id_;

    if ((view_id == LEFT) && ok_client_.is_connected())
    {
        ok_client_.latch_frame();
    }

    BVR::GLMPose eye_pose;

    if (ok_client_.blit_frame(view_id, eye_pose))
    {
        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
        xr_app.override_eye_poses_[view_id] = BVR::convert_to_xr_pose(eye_pose);

        return;
    }
#endif

    std::shared_ptr<ICommandBuffer> buffer = commandQueue_->createCommandBuffer(CommandBufferDesc{}, nullptr);

    const std::shared_ptr<igl::IRenderCommandEncoder> commands =
            buffer->createRenderCommandEncoder(renderPass_, framebuffer_);

#if DRAW_CUBE_UNTIL_CONNECTED
    // cube animation
    setVertexParams();

    constexpr uint32_t textureUnit = 0;

    if (pipelineState_ == nullptr)
    {
        // Graphics pipeline: state batch that fully configures GPU for rendering

        RenderPipelineDesc graphicsDesc;
        graphicsDesc.vertexInputState = vertexInput0_;
        graphicsDesc.shaderStages = shaderStages_;
        graphicsDesc.targetDesc.colorAttachments.resize(1);
        graphicsDesc.targetDesc.colorAttachments[0].textureFormat =
                framebuffer_->getColorAttachment(0)->getProperties().format;
        graphicsDesc.targetDesc.depthAttachmentFormat =
                framebuffer_->getDepthAttachment()->getProperties().format;
        graphicsDesc.fragmentUnitSamplerMap[textureUnit] = IGL_NAMEHANDLE("inputImage");
        graphicsDesc.cullMode = igl::CullMode::Back;
        graphicsDesc.frontFaceWinding = igl::WindingMode::CounterClockwise;
        pipelineState_ = getPlatform().getDevice().createRenderPipeline(graphicsDesc, nullptr);
    }

    commands->bindBuffer(0, BindTarget::kVertex, vb0_, 0);

    // to preserve a beauty of new C++ standard!
    // Bind Vertex Uniform Data
    iglu::ManagedUniformBufferInfo info;
    info.index = 1;
    info.length = sizeof(VertexFormat);
    info.uniforms = std::vector<igl::UniformDesc>
    {
    igl::UniformDesc
    {
            "modelMatrix", -1, igl::UniformType::Mat4x4, 1, offsetof(VertexFormat, modelMatrix), 0},
    igl::UniformDesc{"viewProjectionMatrix",
                     -1,
                     igl::UniformType::Mat4x4,
                     2,
                     offsetof(VertexFormat, viewProjectionMatrix),
                     sizeof(glm::mat4)},
    igl::UniformDesc{
            "scaleZ", -1, igl::UniformType::Float, 1, offsetof(VertexFormat, scaleZ), 0}};

    const auto vertUniformBuffer = std::make_shared<iglu::ManagedUniformBuffer>(device, info);
    IGL_ASSERT(vertUniformBuffer->result.isOk());
    *static_cast<VertexFormat*>(vertUniformBuffer->getData()) = vertexParameters_;

    vertUniformBuffer->bind(device, *pipelineState_, *commands);

    commands->bindTexture(textureUnit, BindTarget::kFragment, tex0_.get());
    commands->bindSamplerState(textureUnit, BindTarget::kFragment, samp0_.get());

    commands->bindRenderPipelineState(pipelineState_);

    constexpr auto indexCount = 3u * 6u * 2u;
    commands->drawIndexed(PrimitiveType::Triangle, indexCount, IndexFormat::UInt16, *ib0_, 0);
#endif

    commands->endEncoding();
    buffer->present(framebuffer_->getColorAttachment(0));
    commandQueue_->submit(*buffer);
}

bool OKCloudSession::post_update() noexcept
{
#if ENABLE_CLOUDXR
    ok_client_.release_frame();
#endif

    return true;
}

#if ENABLE_CLOUDXR
XrInstance OKCloudSession::get_instance()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.instance();
}

XrSession OKCloudSession::get_session()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.session();
}

BVR::OKOpenXRControllerActions& OKCloudSession::get_actions()
{
    return ok_inputs_;
}

const BVR::OKOpenXRControllerActions& OKCloudSession::get_actions() const
{
    return ok_inputs_;
}

XrTime OKCloudSession::get_predicted_display_time_ns()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.get_predicted_display_time_ns();
}

float OKCloudSession::get_current_refresh_rate()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.getCurrentRefreshRate();
}

void OKCloudSession::query_refresh_rates()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    xr_app.querySupportedRefreshRates();
}

bool OKCloudSession::set_refresh_rate(const float refresh_rate)
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.setRefreshRate(refresh_rate);
}

#if ENABLE_CLOUDXR_LINK_SHARPENING
void OKCloudSession::set_sharpening_enabled(const bool enabled)
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    xr_app.setSharpeningEnabled(enabled);
}
#endif

void OKCloudSession::handle_stream_connected()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    xr_app.should_override_eye_poses_ = true;
    xr_app.enableAsyncPolling_ = true;
    xr_app.enableMainThreadPolling_ = !xr_app.enableAsyncPolling_;
}

void OKCloudSession::handle_stream_disconnected()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    xr_app.should_override_eye_poses_ = false;
    xr_app.enableAsyncPolling_ = false;
    xr_app.enableMainThreadPolling_ = !xr_app.enableAsyncPolling_;
}

const XrView OKCloudSession::get_view(const int view_id)
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.views_[view_id];
}

void OKCloudSession::poll_actions(const bool main_thread)
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    xr_app.pollActions(main_thread);
}

XrSpace OKCloudSession::get_base_space()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.currentSpace_;
}

XrSpace OKCloudSession::get_head_space()
{
    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
    return xr_app.headSpace_;
}
#endif


} // namespace BVR

