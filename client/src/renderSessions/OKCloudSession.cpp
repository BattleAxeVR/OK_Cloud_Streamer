//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"

#if ENABLE_CLOUDXR

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

#include "GLMPose.cpp"
#include "OKConfig.cpp"
#include "OKPlayerState.cpp"
#include "OKController.cpp"
#include "OKDigitalButton.cpp"
#include "OKAnalogAxis.cpp"

#ifndef XR_USE_TIMESPEC
#define XR_USE_TIMESPEC
#endif

#include <openxr/openxr_platform.h>

cxrVector3 convert_xr_to_cxr(const XrVector3f &xr_vec)
{
    cxrVector3 cxr_vec;
    cxr_vec.v[0] = xr_vec.x;
    cxr_vec.v[1] = xr_vec.y;
    cxr_vec.v[2] = xr_vec.z;
    return cxr_vec;
}

XrVector3f convert_cxr_to_xr(const cxrVector3 &cxr_vec)
{
    XrVector3f xr_vec;
    xr_vec.x = cxr_vec.v[0];
    xr_vec.y = cxr_vec.v[1];
    xr_vec.z = cxr_vec.v[2];
    return xr_vec;
}

cxrQuaternion convert_xr_to_cxr(const XrQuaternionf &xr_quat)
{
    cxrQuaternion cxr_quat;
    cxr_quat.w = xr_quat.w;
    cxr_quat.x = xr_quat.x;
    cxr_quat.y = xr_quat.y;
    cxr_quat.z = xr_quat.z;
    return cxr_quat;
}

XrQuaternionf convert_cxr_to_xr(const cxrQuaternion &cxr_quat)
{
    XrQuaternionf xr_quat;
    xr_quat.w = cxr_quat.w;
    xr_quat.x = cxr_quat.x;
    xr_quat.y = cxr_quat.y;
    xr_quat.z = cxr_quat.z;
    return xr_quat;
}

extern "C" void dispatchLogMsg(cxrLogLevel level, cxrMessageCategory category, void *extra, const char *tag, const char *fmt, ...)
{
}

namespace BVR
{
    const char *cxr_input_paths[] =
    {
            "/input/system/click",
            "/input/application_menu/click",
            "/input/trigger/click",
            "/input/trigger/touch",
            "/input/trigger/value",
            "/input/grip/click",
            "/input/grip/touch",
            "/input/grip/value",
            "/input/joystick/click",
            "/input/joystick/touch",
            "/input/joystick/x",
            "/input/joystick/y",
            "/input/a/click",
            "/input/b/click",
            "/input/x/click",
            "/input/y/click",
            "/input/a/touch",
            "/input/b/touch",
            "/input/x/touch",
            "/input/y/touch",
            "/input/thumb_rest/touch",
    };

    cxrInputValueType cxr_input_value_types[] =
    {
            cxrInputValueType_boolean,  // input/system/click
            cxrInputValueType_boolean,  // input/application_menu/click
            cxrInputValueType_boolean,  // input/trigger/click
            cxrInputValueType_boolean,  // input/trigger/touch
            cxrInputValueType_float32,  // input/trigger/value
            cxrInputValueType_boolean,  // input/grip/click
            cxrInputValueType_boolean,  // input/grip/touch
            cxrInputValueType_float32,  // input/grip/value
            cxrInputValueType_boolean,  // input/joystick/click
            cxrInputValueType_boolean,  // input/joystick/touch
            cxrInputValueType_float32,  // input/joystick/x
            cxrInputValueType_float32,  // input/joystick/y
            cxrInputValueType_boolean,  // input/a/click
            cxrInputValueType_boolean,  // input/b/click
            cxrInputValueType_boolean,  // input/x/click
            cxrInputValueType_boolean,  // input/y/click
            cxrInputValueType_boolean,  // input/a/touch
            cxrInputValueType_boolean,  // input/b/touch
            cxrInputValueType_boolean,  // input/x/touch
            cxrInputValueType_boolean,  // input/y/touch
            cxrInputValueType_boolean,  // input/thumb_rest/touch
    };

    cxrTrackedDevicePose convert_glm_to_cxr_pose(const GLMPose &glm_pose)
    {
        cxrTrackedDevicePose cxr_pose = {};

        cxr_pose.position.v[0] = glm_pose.translation_.x;
        cxr_pose.position.v[1] = glm_pose.translation_.y;
        cxr_pose.position.v[2] = glm_pose.translation_.z;

        cxr_pose.rotation.w = glm_pose.rotation_.w;
        cxr_pose.rotation.x = glm_pose.rotation_.x;
        cxr_pose.rotation.y = glm_pose.rotation_.y;
        cxr_pose.rotation.z = glm_pose.rotation_.z;

        cxr_pose.deviceIsConnected = true;
        cxr_pose.poseIsValid = glm_pose.is_valid_;
        cxr_pose.trackingResult = cxrTrackingResult_Running_OK;

        return cxr_pose;
    }

    cxrVector3 convert_xr_to_cxr_vector3(const XrVector3f &input)
    {
        cxrVector3 output = {0};

        output.v[0] = input.x;
        output.v[1] = input.y;
        output.v[2] = input.z;

        return output;
    }

    cxrQuaternion convert_xr_to_cxr_quat(const XrQuaternionf &input)
    {
        cxrQuaternion output = {0};

        output.w = input.w;
        output.x = input.x;
        output.y = input.y;
        output.z = input.z;

        return output;
    }

    cxrTrackedDevicePose convert_xr_to_cxr_pose(XrSpaceLocation &location)
    {
        const XrPosef &xr_pose = location.pose;

        cxrTrackedDevicePose cxr_pose = {};
        cxr_pose.position = convert_xr_to_cxr_vector3(xr_pose.position);
        cxr_pose.rotation = convert_xr_to_cxr_quat(xr_pose.orientation);

        XrSpaceVelocity *velocity = (XrSpaceVelocity *) location.next;

        if (velocity && velocity->type == XR_TYPE_SPACE_VELOCITY)
        {
            if (velocity->velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
            {
                cxr_pose.velocity = convert_xr_to_cxr_vector3(velocity->linearVelocity);
            }

            if (velocity->velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
            {
                cxr_pose.angularVelocity = convert_xr_to_cxr_vector3(velocity->angularVelocity);
            }
        }

        cxr_pose.deviceIsConnected = true;
        cxr_pose.poseIsValid = true;
        cxr_pose.trackingResult = cxrTrackingResult_Running_OK;

        return cxr_pose;
    }
}

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
        if (!is_cxr_initialized_ && is_ready_to_connect())
        {
            const bool init_cxr_ok = init_cxr();

            if (init_cxr_ok)
            {
#if AUTO_CONNECT_TO_CLOUDXR
                connect();
#endif
            }
        }

        if (is_connected())
        {
#if LATCH_FRAMES_BOTH_EYES
            latch_frame(INVALID_INDEX);
            //return is_latched_;
#endif
        }

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

#if ENABLE_CLOUDXR_FRAME_BLIT
        int view_id = shellParams().current_view_id_;

#if LATCH_FRAMES_PER_EYE
        latch_frame(view_id);
#endif

        if (is_connected() && is_latched_)
        {
            uint32_t frame_mask = (view_id == LEFT) ? cxrFrameMask_Left : cxrFrameMask_Right;
            cxrError blit_error = cxrBlitFrame(cxr_receiver_, &latched_frames_, frame_mask);

            if (blit_error)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "cxrBlitFrame error = %s\n", cxrErrorString(blit_error));
            }
            else
            {
                if (shellParams().xr_app_ptr_)
                {
                    openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;

                    xr_app.cloudxr_connected_ = true;

                    cxrVector3 cxr_hmd_position = {};
                    cxrQuaternion cxr_hmd_rotation = {};
                    cxrMatrixToVecQuat(&latched_frames_.poseMatrix, &cxr_hmd_position, &cxr_hmd_rotation);

                    XrVector3f xr_hmd_position = convert_cxr_to_xr(cxr_hmd_position);
                    XrQuaternionf xr_hmd_rotation = convert_cxr_to_xr(cxr_hmd_rotation);

                    const BVR::GLMPose hmd_pose(BVR::convert_to_glm(xr_hmd_position), BVR::convert_to_glm(xr_hmd_rotation));
                    BVR::GLMPose eye_pose = hmd_pose;

                    const float half_ipd = ipd_meters_ * 0.5f;
                    const float ipd_offset = (view_id == LEFT) ? -half_ipd : half_ipd;
                    const glm::vec3 ipd_offset_vec = glm::vec3(ipd_offset, 0.0f, 0.0f);
                    eye_pose.translation_ += eye_pose.rotation_ * ipd_offset_vec;

                    xr_app.override_eye_poses_[view_id] = BVR::convert_to_xr_pose(eye_pose);
                }

#if LATCH_FRAMES_PER_EYE
                release_frame();
#endif

            }
            return;
        }
#if DRAW_CUBE_UNTIL_CONNECTED
        else if (is_connected())
#endif
        {
#if 1
            auto buffer = commandQueue_->createCommandBuffer(CommandBufferDesc{}, nullptr);

            const std::shared_ptr<igl::IRenderCommandEncoder> commands =
                    buffer->createRenderCommandEncoder(renderPass_, framebuffer_);

            commands->endEncoding();
            buffer->present(framebuffer_->getColorAttachment(0));
            commandQueue_->submit(*buffer);
#endif

            return;
        }
#endif

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

        // Command buffers (1-N per thread): create, submit and forget
        auto buffer = commandQueue_->createCommandBuffer(CommandBufferDesc{}, nullptr);

        const std::shared_ptr<igl::IRenderCommandEncoder> commands =
                buffer->createRenderCommandEncoder(renderPass_, framebuffer_);

        commands->bindBuffer(0, BindTarget::kVertex, vb0_, 0);

#if defined(IGL_UWP_VS_FIX)
        iglu::ManagedUniformBufferInfo info;
  info.index = 1;
  info.length = sizeof(VertexFormat);
  {
    igl::UniformDesc e;
    e.name = "modelMatrix";
    e.type = igl::UniformType::Mat4x4;
    e.offset = offsetof(VertexFormat, modelMatrix);
    info.uniforms.push_back(std::move(e));
  }
  {
    igl::UniformDesc e;
    e.name = "viewProjectionMatrix";
    e.type = igl::UniformType::Mat4x4;
    e.numElements = 2;
    e.offset = offsetof(VertexFormat, viewProjectionMatrix);
    e.elementStride = sizeof(glm::mat4);
    info.uniforms.push_back(std::move(e));
  }
  {
    igl::UniformDesc e;
    e.name = "scaleZ";
    e.type = igl::UniformType::Float;
    e.offset = offsetof(VertexFormat, scaleZ);
    info.uniforms.push_back(std::move(e));
  }

#else // to preserve a beauty of new C++ standard!
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
#endif

        const auto vertUniformBuffer = std::make_shared<iglu::ManagedUniformBuffer>(device, info);
        IGL_ASSERT(vertUniformBuffer->result.isOk());
        *static_cast<VertexFormat*>(vertUniformBuffer->getData()) = vertexParameters_;

        vertUniformBuffer->bind(device, *pipelineState_, *commands);

        commands->bindTexture(textureUnit, BindTarget::kFragment, tex0_.get());
        commands->bindSamplerState(textureUnit, BindTarget::kFragment, samp0_.get());

        commands->bindRenderPipelineState(pipelineState_);

        constexpr auto indexCount = 3u * 6u * 2u;
        commands->drawIndexed(PrimitiveType::Triangle, indexCount, IndexFormat::UInt16, *ib0_, 0);

        commands->endEncoding();

        buffer->present(framebuffer_->getColorAttachment(0));

        commandQueue_->submit(*buffer);
#endif
    }

    bool OKCloudSession::post_update() noexcept
    {
        if (is_connected())
        {
#if ENABLE_CLOUDXR_FRAME_LATCH
            release_frame();
#endif

            //send_all_analog_controller_values_ = false;
            //send_all_digital_controller_values_ = false;
        }

        return true;
    }

    bool OKCloudSession::init_cxr()
    {
        if (is_cxr_initialized_)
        {
            return true;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::init_cxr\n");

#if ENABLE_OBOE
        init_audio();
#endif

        const bool init_ok = create_receiver();

        is_cxr_initialized_ = init_ok;
        return init_ok;
    }

    void OKCloudSession::update_cxr_state(cxrClientState state, cxrError error)
    {
        const bool was_connected = is_connected();

        switch (state)
        {
            case cxrClientState_ReadyToConnect:
                IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_ReadyToConnect");
                break;
            case cxrClientState_ConnectionAttemptInProgress:
                IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_ConnectionAttemptInProgress");
                break;
            case cxrClientState_ConnectionAttemptFailed:
                IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_ConnectionAttemptFailed");
                break;
            case cxrClientState_StreamingSessionInProgress:
                IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_StreamingSessionInProgress");
                break;
            case cxrClientState_Disconnected:
                IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_Disconnected, setting back to cxrClientState_ReadyToConnect");
                cxr_client_state_ = cxrClientState_ReadyToConnect;
                return;
            case cxrClientState_Exiting:
                IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_Exiting, setting back to cxrClientState_ReadyToConnect");
                cxr_client_state_ = cxrClientState_ReadyToConnect;
                return;
        }

        cxr_client_state_ = state;

        const bool is_connected_now = is_connected();

        if (is_connected_now != was_connected) {
            openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;

            xr_app.enableAsyncPolling_ = is_connected_now;
            xr_app.enableMainThreadPolling_ = !xr_app.enableAsyncPolling_;

#if ENABLE_CLOUDXR_LINK_SHARPENING
            xr_app.setSharpeningEnabled(is_connected_now);
#endif
        }
    }

    void OKCloudSession::shutdown_cxr()
    {
        if (!is_cxr_initialized_)
        {
            return;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::shutdown_cxr\n");

        disconnect();

#if ENABLE_OBOE
        shutdown_audio();
#endif

        is_cxr_initialized_ = false;
    }

    bool OKCloudSession::connect()
    {
        if (!is_cxr_initialized_ || !is_ready_to_connect() )
        {
            return false;
        }

        if (!cxr_receiver_)
        {
            create_receiver();

            if (!cxr_receiver_)
            {
                return false;
            }
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::connect IP = %s\n", ip_address_.c_str());

        cxrConnectionDesc connection_desc = {0};
        connection_desc.async = true;
        connection_desc.useL4S = false;
        connection_desc.clientNetwork = cxrNetworkInterface_Unknown;
        connection_desc.topology = cxrNetworkTopology_LAN;

        cxrError error = cxrConnect(cxr_receiver_, ip_address_.c_str(), &connection_desc);

        if (error)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "cxrConnect error = %s\n", cxrErrorString(error));
            return false;
        }

        return true;
    }

    void OKCloudSession::disconnect()
    {
        if (!is_cxr_initialized_ || !is_connected() || !is_connecting())
        {
            return;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::disconnect\n");
        destroy_receiver();
    }

    bool OKCloudSession::create_receiver()
    {
        if (cxr_receiver_  || !shellParams().xr_app_ptr_)
        {
            return false;
        }

        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::create_receiver\n");

        // Set parameters here...
        cxrReceiverDesc receiver_desc = {0};
        receiver_desc.requestedVersion = CLOUDXR_VERSION_DWORD;

        Platform& platform = getPlatform();
        const igl::IDevice& device = platform.getDevice();
        const igl::opengl::Device* gl_device_ptr = (const igl::opengl::Device*)&device;
        const opengl::egl::Context* egl_context_ptr = (opengl::egl::Context*)(&gl_device_ptr->getContext());

        EGLDisplay egl_display = egl_context_ptr->getDisplay();
        EGLContext egl_context = egl_context_ptr->get();

        graphics_context_.type = cxrGraphicsContext_GLES;
        graphics_context_.egl.display = (void *)egl_display;
        graphics_context_.egl.context = (void *)egl_context;

        receiver_desc.shareContext = &graphics_context_;

        // Debug flags
        receiver_desc.debugFlags = cxrDebugFlags_EnableAImageReaderDecoder;

#if ENABLE_CLOUDXR_LOGGING
        {
            // Logging
            //receiver_desc.debugFlags |= cxrDebugFlags_LogVerbose;

            receiver_desc.logMaxSizeKB = CLOUDXR_LOG_MAX_DEFAULT;
            receiver_desc.logMaxAgeDays = CLOUDXR_LOG_MAX_DEFAULT;

            const std::string application_id = "OK Cloud Streamer v1.0";

            std::string log_dir = "/sdcard/Android/data/com.battleaxevr.okcloudstreamer.gles/files/logs/";
            strncpy(receiver_desc.appOutputPath, log_dir.c_str(), CXR_MAX_PATH - 1);

            receiver_desc.appOutputPath[CXR_MAX_PATH - 1] = 0;
        }
#endif

        cxrDeviceDesc& device_desc = receiver_desc.deviceDesc;
        device_desc.maxResFactor = DEFAULT_CLOUDXR_MAX_RES_FACTOR;

        compute_ipd();
        device_desc.ipd = ipd_meters_;
        device_desc.foveationModeCaps = cxrFoveation_PiecewiseQuadratic;

        const uint32_t number_of_streams = 2;
        device_desc.numVideoStreamDescs = number_of_streams;

        uint32_t max_bitrate = DEFAULT_CLOUDXR_MAX_BITRATE;
        float foveation = DEFAULT_CLOUDXR_FOVEATION;

        uint32_t per_eye_width = DEFAULT_CLOUDXR_PER_EYE_WIDTH;
        uint32_t per_eye_height = DEFAULT_CLOUDXR_PER_EYE_HEIGHT;

        float fps = DEFAULT_CLOUDXR_FRAMERATE;

        float current_refresh_rate = xr_app.getCurrentRefreshRate();

        if (current_refresh_rate > 0.0f)
        {
            xr_app.querySupportedRefreshRates();
            xr_app.setRefreshRate(DEFAULT_CLOUDXR_FRAMERATE);
            fps = xr_app.getCurrentRefreshRate();
        }

        xr_app.setSharpeningEnabled(true);

        for (uint32_t stream_index = 0; stream_index < number_of_streams; stream_index++)
        {
            device_desc.videoStreamDescs[stream_index].width = per_eye_width;
            device_desc.videoStreamDescs[stream_index].height = per_eye_height;
            device_desc.videoStreamDescs[stream_index].format = cxrClientSurfaceFormat_RGB;
            device_desc.videoStreamDescs[stream_index].fps = fps;
            device_desc.videoStreamDescs[stream_index].maxBitrate = max_bitrate;
        }

        device_desc.disableVVSync = false;
        device_desc.embedInfoInVideo = false;
        device_desc.foveatedScaleFactor = foveation;
        device_desc.stereoDisplay = true;
        device_desc.predOffset = DEFAULT_CLOUDXR_PREDICTION_OFFSET_NS * NS_TO_SEC;
        device_desc.posePollFreq = (uint32_t)roundf(DEFAULT_CLOUDXR_POSE_POLL_FREQUENCY_MULT * fps);

#if ENABLE_OBOE
        device_desc.receiveAudio = enable_audio_playback_;
        device_desc.sendAudio = enable_audio_recording_;
#else
        device_desc.receiveAudio = false;
        device_desc.sendAudio = false;
#endif
        device_desc.disablePosePrediction = false;
        device_desc.angularVelocityInDeviceSpace = false;

        {
            cxrClientCallbacks& receiver_callbacks = receiver_desc.clientCallbacks;
            receiver_callbacks.clientContext = this;

            receiver_callbacks.UpdateClientState = [](void *context, cxrClientState cxr_client_state, cxrError error) {
                reinterpret_cast<OKCloudSession*>(context)->update_cxr_state(cxr_client_state, error);
            };


            receiver_callbacks.GetTrackingState = [](void *context, cxrVRTrackingState* cxr_tracking_state_ptr) {
                reinterpret_cast<OKCloudSession*>(context)->get_tracking_state(cxr_tracking_state_ptr);
            };

#if ENABLE_OBOE
            receiver_callbacks.RenderAudio = [](void *context, const cxrAudioFrame *audioFrame) {
                return reinterpret_cast<OKCloudSession*>(context)->render_audio(audioFrame);
            };
#endif

#if ENABLE_HAPTICS
            receiver_callbacks.TriggerHaptic = [](void *context, const cxrHapticFeedback* haptics) {
                reinterpret_cast<OKCloudSession*>(context)->trigger_haptics(haptics);
            };
#endif
        }

        {
            // Default Chaperone
            device_desc.chaperone.universe = cxrUniverseOrigin_Standing;

            device_desc.chaperone.origin.m[0][0] = 1.0f;
            device_desc.chaperone.origin.m[0][1] = 0.0f;
            device_desc.chaperone.origin.m[0][2] = 0.0f;
            device_desc.chaperone.origin.m[0][3] = 0.0f;

            device_desc.chaperone.origin.m[1][0] = 0.0f;
            device_desc.chaperone.origin.m[1][1] = 1.0f;
            device_desc.chaperone.origin.m[1][2] = 0.0f;
            device_desc.chaperone.origin.m[1][3] = 0.0f;

            device_desc.chaperone.origin.m[2][0] = 0.0f;
            device_desc.chaperone.origin.m[2][1] = 0.0f;
            device_desc.chaperone.origin.m[2][2] = 1.0f;
            device_desc.chaperone.origin.m[2][3] = 0.0f;

            device_desc.chaperone.playArea.v[0] = 1.5f;
            device_desc.chaperone.playArea.v[1] = 1.5f;
        }

        {
            //FOV
            for (int view_id = LEFT; view_id < NUM_SIDES; view_id++)
            {
                const XrFovf& fov = xr_app.views_[view_id].fov;

                device_desc.proj[view_id][0] = tanf(fov.angleLeft);
                device_desc.proj[view_id][1] = tanf(fov.angleRight);
                device_desc.proj[view_id][2] = tanf(fov.angleDown);
                device_desc.proj[view_id][3] = tanf(fov.angleUp);
            }
        }

        cxrError error = cxrCreateReceiver(&receiver_desc, &cxr_receiver_);

        if (error)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "cxrCreateReceiver error = %s\n", cxrErrorString(error));
            return false;
        }

        return true;
    }

    void OKCloudSession::destroy_receiver()
    {
        if (!is_cxr_initialized_ || !cxr_receiver_)
        {
            return;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::destroy_receiver\n");

#if ENABLE_OBOE
       shutdown_audio();
#endif

#if ENABLE_CLOUDXR_CONTROLLERS
        remove_controllers();
#endif

        cxrDestroyReceiver(cxr_receiver_);
        cxr_receiver_ = nullptr;

#if USE_CLOUDXR_POSE_ID
        poseID_ = 0;
#endif

        update_cxr_state(cxrClientState_Disconnected, cxrError_Success);
    }

    void OKCloudSession::compute_ipd()
    {
        if (!shellParams().xr_app_ptr_)
        {
            return;
        }

        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;

        const XrPosef& left_eye_pose = xr_app.views_[LEFT].pose;
        const XrPosef& right_eye_pose = xr_app.views_[RIGHT].pose;

        const XrVector3f delta = {(right_eye_pose.position.x - left_eye_pose.position.x),
                                          (right_eye_pose.position.y - left_eye_pose.position.y),
                                          (right_eye_pose.position.z - left_eye_pose.position.z)};

        float ipd = sqrtf((delta.x * delta.x) + (delta.y * delta.y) + (delta.z * delta.z));
        ipd_meters_ = roundf(ipd * 10000.0f) / 10000.0f;

        //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::compute_ipd IPP =  %.7f meters (%.03f mm)\n", ipd_meters_, ipd_meters_ * MILLIMETERS_PER_METER);
    }

#if ENABLE_CLOUDXR_CONTROLLERS
    bool OKCloudSession::add_controllers()
    {
        if (!is_connected())
        {
            return false;
        }

        if (controllers_initialized_)
        {
            return true;
        }

        for (int controller_id = LEFT; controller_id < CXR_NUM_CONTROLLERS; controller_id++)
        {
            //assert(cxr_controller_handles_[controller_id] == nullptr);

            //if (cxr_controller_handles_[controller_id] == nullptr)
            {
                cxrControllerDesc cxr_controller_desc = {};
                cxr_controller_desc.id = controller_id;
                cxr_controller_desc.role = (controller_id == LEFT) ? "cxr://input/hand/left" : "cxr://input/hand/right";
                cxr_controller_desc.controllerName = "Oculus Touch";
                cxr_controller_desc.inputCount = ARRAY_SIZE(BVR::cxr_input_paths);
                cxr_controller_desc.inputPaths = BVR::cxr_input_paths;
                cxr_controller_desc.inputValueTypes = BVR::cxr_input_value_types;

                cxrError add_controller_error = cxrAddController(cxr_receiver_,
                                                                 &cxr_controller_desc,
                                                                 &cxr_controller_handles_[controller_id]);

                if (add_controller_error)
                {
                    IGLLog(IGLLogLevel::LOG_ERROR, "cxrAddController error = %s\n",
                           cxrErrorString(add_controller_error));

                    return false;
                }
            }
        }

        controllers_initialized_ = true;
        return true;
    }

    void OKCloudSession::remove_controllers()
    {
        if (!controllers_initialized_)
        {
            return;
        }

        for (int controller_id = LEFT; controller_id < CXR_NUM_CONTROLLERS; controller_id++)
        {
            //assert(cxr_controller_handles_[controller_id] != nullptr);

            if (cxr_controller_handles_[controller_id] != nullptr)
            {
                if (is_connected())
                {
                    cxrError remove_controller_error = cxrRemoveController(cxr_receiver_,
                                                                           cxr_controller_handles_[controller_id]);

                    if (remove_controller_error)
                    {
                        IGLLog(IGLLogLevel::LOG_ERROR, "cxrRemoveController error = %s\n",
                               cxrErrorString(remove_controller_error));
                    }
                }

                cxr_controller_handles_[controller_id] = nullptr;
            }
        }

        controllers_initialized_ = false;
    }
#endif

#if ENABLE_CLOUDXR_FRAME_LATCH
    bool OKCloudSession::latch_frame(int view_id)
    {
        if (!is_cxr_initialized_ || !is_connected() || is_latched_)
        {
            return false;
        }

        uint32_t timeoutMS = DEFAULT_CLOUDXR_LATCH_TIMEOUT_MS;

        uint32_t frame_mask = cxrFrameMask_All;

        if (view_id == INVALID_INDEX)
        {
            frame_mask = cxrFrameMask_All;
        }
        else
        {
            frame_mask = (view_id == LEFT) ? cxrFrameMask_Left : cxrFrameMask_Right;
        }

        cxrError error = cxrLatchFrame(cxr_receiver_, &latched_frames_, frame_mask, timeoutMS);

        if (error)
        {
            //is_latched_[view_id] = false;

            const bool is_real_error = (error != cxrError_Frame_Not_Ready);

            if (is_real_error)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "cxrLatchFrame error = %s\n", cxrErrorString(error));
            }

            return false;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::latch_frame LATCHED\n");
        is_latched_ = true;

        return true;
    }

    void OKCloudSession::release_frame()
    {
        if (!is_cxr_initialized_ || !is_connected() || !is_latched_)
        {
            return;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::release_frame\n");
        cxrReleaseFrame(cxr_receiver_, &latched_frames_);
        is_latched_ = false;
    }
#endif

    void OKCloudSession::get_tracking_state(cxrVRTrackingState* cxr_tracking_state_ptr)
    {
        if (!is_cxr_initialized_ || !is_connected() || !cxr_tracking_state_ptr || !shellParams().xr_app_ptr_)
        {
            return;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::get_tracking_state\n");

        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;

        const uint64_t predicted_display_time_ns = xr_app.get_predicted_display_time_ns() + DEFAULT_CLOUDXR_PREDICTION_OFFSET_NS;
        openxr::XrInputState& xr_inputs = xr_app.xr_inputs_;

        cxrVRTrackingState& cxr_tracking_state = *cxr_tracking_state_ptr;
        memset(cxr_tracking_state_ptr, 0, sizeof(*cxr_tracking_state_ptr));

        cxr_tracking_state.poseTimeOffset = DEFAULT_CLOUDXR_POSE_TIME_OFFSET_SECONDS;

#if ENABLE_CLOUDXR_CONTROLLERS
        add_controllers();

        if (controllers_initialized_)
        {
            // Poll from async thread, possibly much higher Hz (up to 1 Khz) than main render thread (to reduce latency)
            xr_app.pollActions(false);

            for (int controller_id = LEFT; controller_id < CXR_NUM_CONTROLLERS; controller_id++)
            {
                XrActionStateGetInfo action_info = {XR_TYPE_ACTION_STATE_GET_INFO};
                XrActionStatePose pose_state = {XR_TYPE_ACTION_STATE_POSE};

                action_info.subactionPath = xr_inputs.handSubactionPath[controller_id];
                action_info.action = xr_inputs.aimPoseAction;

                XrResult result = xrGetActionStatePose(xr_app.session_, &action_info,  &pose_state);

                cxrControllerTrackingState& cxr_controller = cxr_tracking_state.controller[controller_id];
                cxr_controller = {};

                BVR::OKController& ok_controller = ok_player_state_.controllers_[controller_id];
                ok_controller.pose_.is_valid_ = false;

                if (XR_UNQUALIFIED_SUCCESS(result) && pose_state.isActive)
                {
                    XrSpaceVelocity controller_velocity = {XR_TYPE_SPACE_VELOCITY};
                    XrSpaceLocation controller_location = {XR_TYPE_SPACE_LOCATION,
                                                           &controller_velocity};

                    XrResult controller_result =
                            xrLocateSpace(xr_inputs.aimSpace[controller_id], xr_app.currentSpace_,
                                          predicted_display_time_ns, &controller_location);

                    if (controller_result == XR_SUCCESS)
                    {
                        cxr_controller.clientTimeNS = predicted_display_time_ns;
                        ok_controller.pose_ = BVR::convert_to_glm_pose(controller_location.pose);

#if ENABLE_CLOUDXR_CONTROLLER_FIX
                        BVR::GLMPose cloudxr_controller_offset;

                        const float x_offset = (controller_id == LEFT) ? -CLOUDXR_CONTROLLER_OFFSET_X : CLOUDXR_CONTROLLER_OFFSET_X;

                        cloudxr_controller_offset.translation_ =
                                glm::vec3(x_offset, CLOUDXR_CONTROLLER_OFFSET_Y,
                                          CLOUDXR_CONTROLLER_OFFSET_Z);

                        cloudxr_controller_offset.euler_angles_degrees_ =
                                glm::vec3(CLOUDXR_CONTROLLER_ROTATION_EULER_X, CLOUDXR_CONTROLLER_ROTATION_EULER_Y,
                                          CLOUDXR_CONTROLLER_ROTATION_EULER_Z);

                        cloudxr_controller_offset.update_rotation_from_euler();

                        const glm::vec3 offset_ws = ok_controller.pose_.rotation_ * cloudxr_controller_offset.translation_;
                        ok_controller.pose_.translation_ += offset_ws;
                        ok_controller.pose_.rotation_ = glm::normalize(ok_controller.pose_.rotation_ * cloudxr_controller_offset.rotation_);

                        cxr_controller.pose = convert_glm_to_cxr_pose(ok_controller.pose_);
#endif

                        cxr_controller.pose = convert_glm_to_cxr_pose(ok_controller.pose_);

                        update_controller_digital_buttons(controller_id);
                        update_controller_analog_axes(controller_id);

                        //send_controller_poses(cxr_controller, controller_id, predicted_display_time_ns);

                        fire_controller_events(controller_id, predicted_display_time_ns);
                    }
                }

#if SQUEEZE_LEFT_GRIP_TO_ENABLE_SHARPENING
                if (controller_id == LEFT)
                {
                    XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                    getInfo.action = xr_inputs.grabAction;
                    getInfo.subactionPath = xr_inputs.handSubactionPath[controller_id];

                    XrActionStateFloat grabValue{XR_TYPE_ACTION_STATE_FLOAT};
                    XR_CHECK(xrGetActionStateFloat(xr_app.session_, &getInfo, &grabValue));

                    const bool enable_sharpening = ((grabValue.isActive == XR_TRUE) && (grabValue.currentState > 0.9f));
                    xr_app.setSharpeningEnabled(enable_sharpening);
                }
#endif
            }
        }
#endif

#if ENABLE_CLOUDXR_HMD
        {
            cxr_tracking_state.hmd.flags = 0;

#if RECOMPUTE_IPD_EVERY_FRAME
            compute_ipd();
            cxr_tracking_state.hmd.flags |= cxrHmdTrackingFlags_HasIPD;
            cxr_tracking_state.hmd.ipd = ipd_meters_;
#endif

#if USE_CLOUDXR_POSE_ID
            cxr_tracking_state.hmd.flags |= cxrHmdTrackingFlags_HasPoseID;
            cxr_tracking_state.hmd.poseID = poseID_++;
#endif

            cxrTrackedDevicePose& cxr_hmd_pose = cxr_tracking_state.hmd.pose;
            cxr_hmd_pose = {};

            XrSpaceVelocity hmd_velocity = {XR_TYPE_SPACE_VELOCITY};
            XrSpaceLocation hmd_location = {XR_TYPE_SPACE_LOCATION, &hmd_velocity};

            XrResult hmd_result = xrLocateSpace(xr_app.headSpace_, xr_app.currentSpace_, predicted_display_time_ns, &hmd_location);

            if (XR_UNQUALIFIED_SUCCESS(hmd_result))
            {
                cxr_hmd_pose = BVR::convert_xr_to_cxr_pose(hmd_location);
                cxr_tracking_state.hmd.clientTimeNS = predicted_display_time_ns;
                cxr_tracking_state.hmd.activityLevel = cxrDeviceActivityLevel_UserInteraction;
            }
        }
#endif

    }

#if ENABLE_CLOUDXR_CONTROLLERS
    void OKCloudSession::send_controller_poses(cxrControllerTrackingState& cxr_controller, const int controller_id, const uint64_t predicted_display_time_ns)
    {
        if (!is_cxr_initialized_ || !is_connected() || !controllers_initialized_ || !shellParams().xr_app_ptr_)
        {
            return;
        }

        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::send_controller_poses\n");

        {
            const uint32_t pose_count = 1;
            const cxrControllerTrackingState* controller_ptr = &cxr_controller;
            const cxrControllerTrackingState ** controller_states_ptr = &controller_ptr;

            cxrError send_controller_pose_result = cxrSendControllerPoses(cxr_receiver_, pose_count, &cxr_controller_handles_[controller_id], controller_states_ptr);

            if (send_controller_pose_result)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "cxrSendControllerPoses error = %s\n",
                       cxrErrorString(send_controller_pose_result));
            }
        }
    }

    void OKCloudSession::fire_controller_events(const int controller_id, const uint64_t predicted_display_time_ns)
    {
        if (!is_cxr_initialized_ || !is_connected() || !controllers_initialized_ || !shellParams().xr_app_ptr_)
        {
            return;
        }

        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::fire_controller_events\n");

        cxrControllerEvent cxr_events[MAX_CLOUDXR_CONTROLLER_EVENTS] = {};
        uint32_t cxr_event_count = 0;

        const BVR::OKController& ok_controller = ok_player_state_.controllers_[controller_id];

        {
            const uint32_t num_analog_axis_maps = ARRAY_SIZE(BVR::analog_axis_maps);

            for (uint32_t map_id = 0; map_id < num_analog_axis_maps; map_id++)
            {
                const BVR::AnalogAxisToCloudXRMap &analog_axis_map = BVR::analog_axis_maps[map_id];

                if (analog_axis_map.cloudxr_path_id_ == INVALID_INDEX)
                {
                    continue;
                }

                const BVR::OKAnalogAxis& ok_analog_axis = ok_controller.analog_axes_[analog_axis_map.analog_axis_id_];
                const bool was_changed = send_all_analog_controller_values_ || ok_analog_axis.was_value_changed();

                if (was_changed)
                {
                    const float analog_axis_value = ok_analog_axis.get_current_value();

                    cxrControllerEvent &event = cxr_events[cxr_event_count++];
                    event.clientTimeNS = predicted_display_time_ns;
                    event.clientInputIndex = analog_axis_map.cloudxr_path_id_;
                    event.inputValue.valueType = cxrInputValueType_float32;
                    event.inputValue.vF32 = analog_axis_value;
                }
            }
        }

        {
            const uint32_t num_digital_button_maps = ARRAY_SIZE(BVR::digital_button_maps);

            for (uint32_t map_id = 0; map_id < num_digital_button_maps; map_id++)
            {
                const BVR::DigitalButtonToCloudXR_Map& digital_button_map = BVR::digital_button_maps[map_id][controller_id];

                if (digital_button_map.cloudxr_path_id_ == INVALID_INDEX)
                {
                    continue;
                }

                const BVR::OKDigitalButton& ok_digital_button = ok_controller.digital_buttons_[digital_button_map.digital_button_id_];

                const bool is_down = ok_digital_button.is_down();
                const bool was_changed = send_all_digital_controller_values_ || ok_digital_button.was_changed();

                if (was_changed)
                {
                    cxrControllerEvent& event = cxr_events[cxr_event_count++];
                    event.clientTimeNS = predicted_display_time_ns;
                    event.clientInputIndex = digital_button_map.cloudxr_path_id_;
                    event.inputValue.valueType = cxrInputValueType_boolean;
                    event.inputValue.vBool = is_down;
                }
            }
        }

        if (cxr_event_count > 0)
        {
            cxrError fire_controller_events_result = cxrFireControllerEvents(cxr_receiver_, cxr_controller_handles_[controller_id], cxr_events, cxr_event_count);

            if (fire_controller_events_result)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "cxrFireControllerEvents error = %s\n",
                       cxrErrorString(fire_controller_events_result));
            }
        }
    }

    void OKCloudSession::update_controller_digital_buttons(const int controller_id)
    {
        if (!is_cxr_initialized_ || !is_connected() || !shellParams().xr_app_ptr_)
        {
            return;
        }

        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
        openxr::XrInputState& xr_inputs = xr_app.xr_inputs_;

        struct XRActionToDigitalButtonID_Mapping
        {
            XrAction action;
            BVR::DigitalButtonID digital_button_id;
        };

        XRActionToDigitalButtonID_Mapping xr_to_ok_button_mappings[] =
        {
                {xr_inputs.menuClickAction, BVR::DigitalButton_ApplicationMenu},
                {xr_inputs.triggerTouchAction, BVR::DigitalButton_Trigger_Touch},
                {xr_inputs.triggerClickAction, BVR::DigitalButton_Trigger_Click},
                {xr_inputs.squeezeTouchAction, BVR::DigitalButton_Grip_Touch},
                {xr_inputs.squeezeClickAction, BVR::DigitalButton_Grip_Click},
                {xr_inputs.thumbstickTouchAction, BVR::DigitalButton_Joystick_Touch},
                {xr_inputs.thumbstickClickAction, BVR::DigitalButton_Joystick_Click},
                //{xr_inputs.thumbRestTouchAction, BVR::DigitalButton_Touchpad_Touch},
                //{xr_inputs.thumbRestClickAction, BVR::DigitalButton_Touchpad_Click},
                {xr_inputs.buttonAXTouchAction, BVR::DigitalButton_A_Touch},
                {xr_inputs.buttonAXClickAction, BVR::DigitalButton_A_Click},
                {xr_inputs.buttonBYTouchAction, BVR::DigitalButton_B_Touch},
                {xr_inputs.buttonBYClickAction, BVR::DigitalButton_B_Click}
        };

        BVR::OKController& ok_controller = ok_player_state_.controllers_[controller_id];

        XrActionStateGetInfo action_info = {XR_TYPE_ACTION_STATE_GET_INFO};
        action_info.subactionPath = xr_inputs.handSubactionPath[controller_id];

        XrActionStateBoolean button_state = {XR_TYPE_ACTION_STATE_BOOLEAN};

        for (uint32_t j = 0; j < ARRAY_SIZE(xr_to_ok_button_mappings); j++)
        {
            action_info.action = xr_to_ok_button_mappings[j].action;
            XR_CHECK(xrGetActionStateBoolean(xr_app.session_, &action_info, &button_state));

            if (!button_state.isActive)
            {
                continue;
            }

            BVR::OKDigitalButton& ok_digital_button = ok_controller.digital_buttons_[xr_to_ok_button_mappings[j].digital_button_id];
            ok_digital_button.set_state(button_state.currentState);
        }
    }

    void OKCloudSession::update_controller_analog_axes(const int controller_id)
    {
        if (!is_cxr_initialized_ || !is_connected() || !shellParams().xr_app_ptr_)
        {
            return;
        }

        openxr::XrApp& xr_app = *shellParams().xr_app_ptr_;
        openxr::XrInputState& xr_inputs = xr_app.xr_inputs_;

        struct XRActionToAnalogAxisID_Mapping
        {
            XrAction action;
            BVR::AnalogAxisID analog_axis_id;
        };

        XRActionToAnalogAxisID_Mapping xr_to_ok_analog_mappings[] =
        {
                {xr_inputs.triggerValueAction, BVR::AnalogAxis_Trigger},
                {xr_inputs.squeezeValueAction, BVR::AnalogAxis_Grip},
                {xr_inputs.thumbstickXAction, BVR::AnalogAxis_JoystickX},
                {xr_inputs.thumbstickYAction, BVR::AnalogAxis_JoystickY},
                {xr_inputs.thumbProximityAction, BVR::AnalogAxis_Proximity},
                {xr_inputs.thumbRestForceAction, BVR::AnalogAxis_Grip_Force},
                //{xr_inputs.trackpadXAction, BVR::AnalogAxis_JoystickX},
                //{xr_inputs.trackpadYAction, BVR::AnalogAxis_JoystickY}
        };

        BVR::OKController& ok_controller = ok_player_state_.controllers_[controller_id];

        XrActionStateGetInfo action_info = {XR_TYPE_ACTION_STATE_GET_INFO};
        action_info.subactionPath = xr_inputs.handSubactionPath[controller_id];
        XrActionStateFloat axis_state = {XR_TYPE_ACTION_STATE_FLOAT};

        for (uint32_t j = 0; j < ARRAY_SIZE(xr_to_ok_analog_mappings); j++)
        {
            action_info.action = xr_to_ok_analog_mappings[j].action;
            XR_CHECK(xrGetActionStateFloat(xr_app.session_, &action_info, &axis_state));

            if (!axis_state.isActive)
            {
                continue;
            }

            BVR::OKAnalogAxis& ok_analog_axis = ok_controller.analog_axes_[xr_to_ok_analog_mappings[j].analog_axis_id];
            ok_analog_axis.set_value(axis_state.currentState);
        }
    }

#endif

#if ENABLE_HAPTICS
    void OKCloudSession::trigger_haptics(const cxrHapticFeedback* haptics)
    {
        if (!is_cxr_initialized_ || !is_connected() || !haptics)
        {
            return;
        }

        const int controller_id = haptics->deviceID;
        const float duration_ms = haptics->seconds * 1e9;

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::trigger_haptics\n");

        //apply_haptics(controller_id, haptics->amplitude, duration_ms, haptics->frequency);
    }
#endif

#if ENABLE_OBOE
    bool OKCloudSession::init_audio()
    {
        if (!enable_audio_playback_ && !enable_audio_recording_)
        {
            return false;
        }

        if (is_audio_initialized_)
        {
            return true;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::init_audio\n");

        if (enable_audio_playback_)
        {
            // Playback Stream
            oboe::AudioStreamBuilder audio_output_stream_builder = {};
            audio_output_stream_builder.setDirection(oboe::Direction::Output);

            audio_output_stream_builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
            //audio_output_stream_builder.setPerformanceMode(oboe::PerformanceMode::None);

            audio_output_stream_builder.setSharingMode(oboe::SharingMode::Exclusive);
            audio_output_stream_builder.setFormat(oboe::AudioFormat::I16);
            audio_output_stream_builder.setChannelCount(oboe::ChannelCount::Stereo);
            audio_output_stream_builder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);

            oboe::Result playback_stream_result = audio_output_stream_builder.openStream(audio_playback_stream_);

            if (playback_stream_result != oboe::Result::OK)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "openStream playback error = %s\n", oboe::convertToText(playback_stream_result));
                return false;
            }

            int buffer_size = audio_playback_stream_->getFramesPerBurst() * CXR_AUDIO_CHANNEL_COUNT;
            oboe::Result set_buffer_size_result = audio_playback_stream_->setBufferSizeInFrames(buffer_size);

            if (set_buffer_size_result != oboe::Result::OK)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "setBufferSizeInFrames playback error = %s\n", oboe::convertToText(set_buffer_size_result));
                return false;
            }

            oboe::Result start_playback_result = audio_playback_stream_->start();

            if (start_playback_result != oboe::Result::OK)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "start audio playback error = %s\n", oboe::convertToText(start_playback_result));
                return false;
            }
        }

        if (enable_audio_recording_)
        {
            // Capture Stream
            oboe::AudioStreamBuilder audio_capture_stream_builder = {};
            audio_capture_stream_builder.setDirection(oboe::Direction::Input);

            //audio_capture_stream_builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
            audio_capture_stream_builder.setPerformanceMode(oboe::PerformanceMode::None);

            audio_capture_stream_builder.setSharingMode(oboe::SharingMode::Exclusive);
            audio_capture_stream_builder.setFormat(oboe::AudioFormat::I16);
            audio_capture_stream_builder.setChannelCount(oboe::ChannelCount::Stereo);
            audio_capture_stream_builder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);

            audio_capture_stream_builder.setDirection(oboe::Direction::Input);
            audio_capture_stream_builder.setPerformanceMode(oboe::PerformanceMode::None);
            audio_capture_stream_builder.setSharingMode(oboe::SharingMode::Exclusive);
            audio_capture_stream_builder.setFormat(oboe::AudioFormat::I16);
            audio_capture_stream_builder.setChannelCount(oboe::ChannelCount::Stereo);
            audio_capture_stream_builder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);
            audio_capture_stream_builder.setInputPreset(oboe::InputPreset::VoiceCommunication);
            audio_capture_stream_builder.setDataCallback(this);

            oboe::Result capture_stream_result = audio_capture_stream_builder.openStream(audio_record_stream_);

            if (capture_stream_result != oboe::Result::OK)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "openStream record error = %s\n", oboe::convertToText(capture_stream_result));
                return false;
            }

            int buffer_size = audio_playback_stream_->getFramesPerBurst() * 2;
            oboe::Result set_buffer_size_result = audio_playback_stream_->setBufferSizeInFrames(buffer_size);

            if (set_buffer_size_result != oboe::Result::OK)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "setBufferSizeInFrames record error = %s\n", oboe::convertToText(set_buffer_size_result));
                return false;
            }

            oboe::Result start_record_result = audio_record_stream_->start();

            if (start_record_result != oboe::Result::OK)
            {
                IGLLog(IGLLogLevel::LOG_ERROR, "start audio record error = %s\n", oboe::convertToText(start_record_result));
                return false;
            }
        }

        is_audio_initialized_ = true;
        return true;
    }

    void OKCloudSession::shutdown_audio()
    {
        if (!is_audio_initialized_)
        {
            return;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::shutdown_audio\n");

        if (audio_playback_stream_)
        {
            audio_playback_stream_->close();
        }

        if (audio_record_stream_)
        {
            audio_record_stream_->close();
        }

        is_audio_initialized_ = false;
    }

    cxrBool OKCloudSession::render_audio(const cxrAudioFrame* audio_frame)
    {
        if (!audio_frame || !is_audio_initialized_ || !enable_audio_playback_ || !audio_playback_stream_)
        {
            return cxrFalse;
        }

        uint32_t timeout = audio_frame->streamSizeBytes / CXR_AUDIO_BYTES_PER_MS;
        uint32_t frame_count = timeout * CXR_AUDIO_SAMPLING_RATE / 1000;

        oboe::ResultWithValue write_result =
                audio_playback_stream_->write(audio_frame->streamBuffer, frame_count, timeout * oboe::kNanosPerMillisecond);

        if (!write_result)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "Error rendering audio: %s", oboe::convertToText(write_result.error()));

            if (write_result.error() == oboe::Result::ErrorDisconnected)
            {
                shutdown_audio();
                init_audio();
            }
        }

        return cxrTrue;
    }

    oboe::DataCallbackResult OKCloudSession::onAudioReady(oboe::AudioStream* audio_stream, void *data, int32_t frame_count)
    {
        if (is_connected())
        {
            cxrAudioFrame audio_frame = {};
            audio_frame.streamBuffer = (int16_t *)data;
            audio_frame.streamSizeBytes = frame_count * CXR_AUDIO_CHANNEL_COUNT * CXR_AUDIO_SAMPLE_SIZE;
            cxrSendAudio(cxr_receiver_, &audio_frame);
        }

        return oboe::DataCallbackResult::Continue;
    }
#endif

} // namespace BVR


#endif // ENABLE_CLOUDXR

