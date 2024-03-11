//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CLOUD_SESSION_H
#define OK_CLOUD_SESSION_H

#include "defines.h"

#if ENABLE_CLOUDXR
#include <shell/shared/renderSession/RenderSession.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <igl/IGL.h>
#include <shell/shared/platform/Platform.h>

#include <CloudXRClient.h>
#include <CloudXRMatrixHelpers.h>
#include <CloudXRClientOptions.h>
#include <CloudXRController.h>

#if ENABLE_OBOE
#include <oboe/Oboe.h>
#endif

namespace igl::shell
{
    struct VertexFormat
    {
        glm::mat4 modelMatrix = glm::mat4(1.0);
        glm::mat4 viewProjectionMatrix[2]{};
        float scaleZ{};
    };


    class OKCloudSession : public RenderSession
#if ENABLE_OBOE
    , oboe::AudioStreamDataCallback
#endif
    {
    public:
        OKCloudSession(std::shared_ptr<Platform> platform);

        void initialize() noexcept override;
        void update(igl::SurfaceTextures surfaceTextures) noexcept override;

        // CloudXR stuff
        bool init_cxr();
        void update_cxr_state(cxrClientState state, cxrError error);
        void shutdown_cxr();

        bool connect();
        void disconnect();

        void set_ip_address(const std::string& ip_address)
        {
            ip_address_ = ip_address;
        }

        bool is_cxr_initialized() const
        {
            return is_cxr_initialized_;
        }

        bool is_connected() const
        {
            return (cxr_client_state_ == cxrClientState::cxrClientState_StreamingSessionInProgress);
        }

        bool is_connecting() const
        {
            return (cxr_client_state_ == cxrClientState::cxrClientState_ConnectionAttemptInProgress);
        }

        bool failed_to_connect() const
        {
            return (cxr_client_state_ == cxrClientState::cxrClientState_ConnectionAttemptFailed);
        }

#if ENABLE_OBOE
        bool init_audio();
        void shutdown_audio();
        cxrBool RenderAudio(const cxrAudioFrame* audio_frame);
        oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audio_stream, void *data, int32_t frame_count);
#endif

    private:
        std::shared_ptr<ICommandQueue> commandQueue_;
        RenderPassDesc renderPass_;
        std::shared_ptr<IRenderPipelineState> pipelineState_;
        std::shared_ptr<IVertexInputState> vertexInput0_;
        std::shared_ptr<IShaderStages> shaderStages_;
        std::shared_ptr<IBuffer> vb0_, ib0_; // Buffers for vertices and indices (or constants)
        std::shared_ptr<ITexture> tex0_;
        std::shared_ptr<ISamplerState> samp0_;
        std::shared_ptr<IFramebuffer> framebuffer_;

        VertexFormat vertexParameters_;

        void createSamplerAndTextures(const IDevice& /*device*/);
        void setVertexParams();

        bool is_cxr_initialized_ = false;

#if ENABLE_OBOE
        bool is_audio_initialized_ = false;
        bool enable_audio_playback_ = true;
        bool enable_audio_recording_ = true;
        std::shared_ptr<oboe::AudioStream> audio_playback_stream_;
        std::shared_ptr<oboe::AudioStream> audio_record_stream_;
#endif

        std::string ip_address_ = DEFAULT_IP_ADDRESS;

        bool create_receiver();
        void destroy_receiver();

        bool latch_frame();
        void release_frame();

        CloudXR::ClientOptions cxr_options_ = {};
        cxrGraphicsContext graphics_context_ = {};
        cxrReceiverHandle cxr_receiver_ = nullptr;
        cxrClientState cxr_client_state_ = cxrClientState_Disconnected;
        cxrFramesLatched latched_frames_ = {};
        bool is_latched_ = false;

    };

} // namespace igl::shell

#endif // ENABLE_CLOUDXR

#endif // OK_CLOUD_SESSION_H


