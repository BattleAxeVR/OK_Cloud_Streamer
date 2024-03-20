//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CLOUD_CLIENT_H
#define OK_CLOUD_CLIENT_H

#include "defines.h"

#if ENABLE_CLOUDXR

#include "OKConfig.h"
#include "OKPlayerState.h"

#include <CloudXRClient.h>
#include <CloudXRMatrixHelpers.h>
#include <CloudXRClientOptions.h>
#include <CloudXRController.h>

#if ENABLE_OBOE
#include <oboe/Oboe.h>
#endif


// Android / GL ES Only
#include <EGL/egl.h>

namespace igl::shell::openxr
{
    struct XrInputState;
}

namespace BVR 
{

class XRInterface
{
public:
    virtual XrInstance get_instance() = 0;
    virtual XrSession get_session() = 0;

    virtual igl::shell::openxr::XrInputState* get_input_state() = 0;
    virtual XrTime get_predicted_display_time_ns() = 0;

    virtual float get_current_refresh_rate() = 0;
    virtual void query_refresh_rates() = 0;
    virtual bool set_refresh_rate(const float refresh_rate) = 0;

#if ENABLE_CLOUDXR_LINK_SHARPENING
    virtual void set_sharpening_enabled(const bool enabled) = 0;
#endif

    virtual void handle_stream_connected() = 0;
    virtual void handle_stream_disconnected() = 0;

    virtual const XrView get_view(const int view_id) = 0;

    virtual void poll_actions(const bool main_thread) = 0;
    virtual XrSpace get_base_space() = 0;
    virtual XrSpace get_head_space() = 0;
};

class OKCloudClient
#if ENABLE_OBOE
    : public oboe::AudioStreamDataCallback
#endif
{
public:
    OKCloudClient();
    virtual ~OKCloudClient();

    bool init_android_gles(XRInterface* xr_interface, EGLDisplay egl_display, EGLContext egl_context);

    void update_cxr_state(cxrClientState state, cxrError error);
    void shutdown_cxr();

    bool connect();
    void disconnect();

    bool latch_frame(const int view_id);
    bool blit_frame(const int view_id, GLMPose& eye_pose);
    void release_frame();

    bool is_cxr_initialized() const
    {
        return is_cxr_initialized_;
    }

    bool is_ready_to_connect() const
    {
        return (cxr_client_state_ == cxrClientState::cxrClientState_ReadyToConnect);
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
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audio_stream, void* data, int32_t frame_count) override;
#endif

private:
    XRInterface* xr_interface_ = nullptr;

    OKConfig ok_config_;
    OKPlayerState ok_player_state_;

    bool is_cxr_initialized_ = false;

    bool create_receiver();
    void destroy_receiver();

    void compute_ipd();

    void get_tracking_state(cxrVRTrackingState *cxr_tracking_state_ptr);

#if ENABLE_CLOUDXR_CONTROLLERS
    bool controllers_initialized_ = false;
    cxrControllerHandle cxr_controller_handles_[CXR_NUM_CONTROLLERS] = {nullptr, nullptr};
    bool add_controllers();
    void remove_controllers();
    void send_controller_poses(cxrControllerTrackingState& cxr_controller, const int controller_id, const uint64_t predicted_display_time_ns);
    void fire_controller_events(const int controller_id, const uint64_t predicted_display_time_ns);

    void update_controller_digital_buttons(const int controller_id);
    void update_controller_analog_axes(const int controller_id);

    bool send_all_analog_controller_values_ = true;
    bool send_all_digital_controller_values_ = true;
    bool combine_grip_force_with_grip_ = true;
    bool simulate_grip_touch_ = true;
    bool simulate_thumb_rest_ = false; // doesn't work, appears "/input/thumb_rest/touch" doesn't work on CXR side
#endif

#if ENABLE_HAPTICS
    void trigger_haptics(const cxrHapticFeedback *haptics);
#endif
    CloudXR::ClientOptions cxr_options_ = {};
    cxrGraphicsContext graphics_context_ = {};
    cxrReceiverHandle cxr_receiver_ = nullptr;
    cxrClientState cxr_client_state_ = cxrClientState_ReadyToConnect;
    cxrFramesLatched latched_frames_ = {};
    bool is_latched_ = false;

    float ipd_meters_ = DEFAULT_CLOUDXR_IPD_M;

#if USE_CLOUDXR_POSE_ID
    uint64_t poseID_ = 0;
#endif

#if ENABLE_OBOE
    bool init_audio();
    void shutdown_audio();
    cxrBool render_audio(const cxrAudioFrame* audio_frame);

    bool is_audio_initialized_ = false;
    bool enable_audio_playback_ = ENABLE_CLOUDXR_AUDIO_PLAYBACK;
    bool enable_audio_recording_ = ENABLE_CLOUDXR_AUDIO_RECORDING;
    std::shared_ptr<oboe::AudioStream> audio_playback_stream_;
    std::shared_ptr<oboe::AudioStream> audio_record_stream_;
#endif

};

}  // namespace BVR

#endif // ENABLE_CLOUDXR

#endif  // OK_CLOUD_CLIENT_H

