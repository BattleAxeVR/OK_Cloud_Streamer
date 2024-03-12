//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"

#if ENABLE_CLOUDXR

#include <../../../external/igl/IGLU/managedUniformBuffer/ManagedUniformBuffer.h>

#include <algorithm>
#include <cmath>
#include <glm/detail/qualifier.hpp>
#include <igl/NameHandle.h>
#include <igl/ShaderCreator.h>
#include <igl/opengl/Device.h>
#include <igl/opengl/GLIncludes.h>
#include <igl/opengl/RenderCommandEncoder.h>
#include <igl/Log.h>

#include <igl/opengl/egl/Context.h>

#include "OKCloudSession.h"
#include <../../../external/igl/shell/shared/renderSession/ShellParams.h>

extern "C" void dispatchLogMsg(cxrLogLevel level, cxrMessageCategory category, void *extra, const char *tag, const char *fmt, ...)
{
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

    std::unique_ptr<IShaderStages> getShaderStagesForBackend(igl::IDevice& device)
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
        renderPass_.colorAttachments[0].clearColor = {0.0, 0.0, 1.0, 1.0f};
#endif
        renderPass_.depthAttachment.loadAction = LoadAction::Clear;
        renderPass_.depthAttachment.clearDepth = 1.0;

#if 1
        const bool init_cxr_ok = init_cxr();
        if (!init_cxr_ok)
        {
            return;
        }
#endif
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

    void OKCloudSession::update(igl::SurfaceTextures surfaceTextures) noexcept
    {
        auto& device = getPlatform().getDevice();

        if (!isDeviceCompatible(device))
        {
            return;
        }

        // cube animation
        setVertexParams();

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

        commandQueue_->submit(*buffer); // Guarantees ordering between command buffers
    }

    bool OKCloudSession::init_cxr()
    {
        if (is_cxr_initialized_)
        {
            return true;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::init_cxr\n");

#if ENABLE_OBOE
        const bool audio_ok = init_audio();

        if (!audio_ok)
        {
            return false;
        }
#endif

        const bool init_ok = create_receiver();

        is_cxr_initialized_ = init_ok;
        return init_ok;

    }

    void OKCloudSession::update_cxr_state(cxrClientState state, cxrError error)
    {
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
                cxr_client_state_ == cxrClientState_ReadyToConnect;
                return;
            case cxrClientState_Exiting:
                IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_Exiting, setting back to cxrClientState_ReadyToConnect");
                cxr_client_state_ == cxrClientState_ReadyToConnect;
                return;
        }

        cxr_client_state_ = state;
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

        if (cxr_receiver_)
        {
            destroy_receiver();
        }

        if (!cxr_receiver_)
        {
            create_receiver();
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
        if (cxr_receiver_)
        {
            return false;
        }

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

        cxrDeviceDesc& device_desc = receiver_desc.deviceDesc;
        device_desc.maxResFactor = DEFAULT_CLOUDXR_MAX_RES_FACTOR;

        float ipd_m = DEFAULT_CLOUDXR_IPD_M;
        device_desc.ipd = ipd_m;
        device_desc.foveationModeCaps = cxrFoveation_PiecewiseQuadratic;

        const uint32_t number_of_streams = 2;
        device_desc.numVideoStreamDescs = number_of_streams;

        uint32_t max_bitrate = DEFAULT_CLOUDXR_MAX_BITRATE;
        float foveation = DEFAULT_CLOUDXR_FOVEATION;

        uint32_t per_eye_width = DEFAULT_CLOUDXR_PER_EYE_WIDTH;
        uint32_t per_eye_height = DEFAULT_CLOUDXR_PER_EYE_HEIGHT;
        float fps = DEFAULT_CLOUDXR_FRAMERATE;

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
        device_desc.predOffset = DEFAULT_CLOUDXR_PREDICTION_OFFSET;

#if ENABLE_OBOE
        device_desc.receiveAudio = enable_audio_playback_;
        device_desc.sendAudio = enable_audio_recording_;
#else
        device_desc.receiveAudio = false;
        device_desc.sendAudio = false;
#endif
        device_desc.disablePosePrediction = false;
        device_desc.angularVelocityInDeviceSpace = false;

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

        cxrDestroyReceiver(cxr_receiver_);
        cxr_receiver_ = nullptr;

        update_cxr_state(cxrClientState_Disconnected, cxrError_Success);
    }

    bool OKCloudSession::latch_frame()
    {
        if (!is_cxr_initialized_)
        {
            return false;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::latch_frame\n");

        return true;
    }

    void OKCloudSession::release_frame()
    {
        if (!is_cxr_initialized_)
        {
            return;
        }

        IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::release_frame\n");
    }

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

    cxrBool OKCloudSession::RenderAudio(const cxrAudioFrame* audio_frame)
    {
        if (!audio_frame|| is_audio_initialized_ || !enable_audio_playback_ || !audio_playback_stream_)
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

