//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CLOUD_SESSION_H
#define OK_CLOUD_SESSION_H

#include "ok_defines.h"

#if ENABLE_CLOUDXR
#include "OKCloudClient.h"
#include "OKController.h"
#endif

#include <shell/shared/renderSession/RenderSession.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <igl/IGL.h>
#include <shell/shared/platform/Platform.h>

namespace igl::shell
{
    struct VertexFormat
    {
        glm::mat4 modelMatrix = glm::mat4(1.0);
        glm::mat4 viewProjectionMatrix[2]{};
        float scaleZ{};
    };

    inline float sign(float val)
    {
        return (val < 0.0f) ? -1.0f : 1.0f;
    }

    class OKCloudSession : public RenderSession
#if ENABLE_CLOUDXR
    , BVR::OKOpenXRInterface
#endif
    {
    public:
        OKCloudSession(std::shared_ptr<Platform> platform);

        void initialize() noexcept override;

        bool pre_update() noexcept override;
        void update(igl::SurfaceTextures surfaceTextures) noexcept override;
        bool post_update() noexcept override;

#if ENABLE_CLOUDXR
        virtual XrInstance get_instance() override;
        virtual XrSession get_session() override;

        virtual BVR::OKOpenXRControllerActions& get_actions() override;
        virtual const BVR::OKOpenXRControllerActions& get_actions() const override;

        virtual XrTime get_predicted_display_time_ns() override;

        virtual float get_current_refresh_rate() override;
        virtual void query_refresh_rates() override;
        virtual bool set_refresh_rate(const float refresh_rate) override;

#if ENABLE_CLOUDXR_LINK_SHARPENING
        virtual void set_sharpening_enabled(const bool enabled) override;
#endif

        virtual void handle_stream_connected() override;
        virtual void handle_stream_disconnected() override;

        virtual const XrView get_view(const int view_id) override;

        virtual void poll_actions(const bool main_thread) override;
        virtual XrSpace get_base_space() override;
        virtual XrSpace get_head_space() override;
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

        void createSamplerAndTextures(const IDevice & /*device*/);
        void setVertexParams();

#if ENABLE_CLOUDXR
        BVR::OKCloudClient ok_client_;
        BVR::OKOpenXRControllerActions ok_inputs_;
#endif

    };

} // namespace igl::shell


#endif // OK_CLOUD_SESSION_H


