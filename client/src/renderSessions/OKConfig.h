//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CONFIG_H
#define OK_CONFIG_H

#include <string>
#include "GLMPose.h"
#include "ok_defines.h"

namespace BVR 
{

class OKConfig 
{
public:
    OKConfig();

    void reset();

	bool load();
	bool save();

    std::string server_ip_address_ = DEFAULT_SERVER_IP_ADDRESS;

    bool enable_auto_connect_ = AUTO_CONNECT_TO_CLOUDXR;

    uint32_t per_eye_width_ = DEFAULT_CLOUDXR_PER_EYE_WIDTH;
    uint32_t per_eye_height_ = DEFAULT_CLOUDXR_PER_EYE_HEIGHT;

    uint32_t desired_refresh_rate_ = DEFAULT_CLOUDXR_FRAMERATE;
    uint32_t polling_rate_mult_ = DEFAULT_CLOUDXR_POSE_POLL_FREQUENCY_MULT;

    uint32_t foveation_ = DEFAULT_CLOUDXR_FOVEATION;
    bool enable_sharpening_ = ENABLE_CLOUDXR_LINK_SHARPENING;

    float max_res_factor_ = DEFAULT_CLOUDXR_MAX_RES_FACTOR;
    uint32_t max_bitrate_kbps_ = DEFAULT_CLOUDXR_MAX_BITRATE_KBPS;

    float prediction_offset_ns_ = DEFAULT_CLOUDXR_PREDICTION_OFFSET_NS;
    float pose_time_offset_s_ = DEFAULT_CLOUDXR_POSE_TIME_OFFSET_SECONDS;
    uint32_t latch_timeout_ms_ = DEFAULT_CLOUDXR_LATCH_TIMEOUT_MS;

    bool enable_audio_playback_ = ENABLE_CLOUDXR_AUDIO_PLAYBACK;
    bool enable_audio_recording_ = ENABLE_CLOUDXR_AUDIO_RECORDING;

    bool enable_eye_tracking_ = ENABLE_EYE_TRACKING;
    bool enable_face_tracking_ = ENABLE_FACE_TRACKING;
    bool enable_hand_tracking_ = ENABLE_HAND_TRACKING;
    bool enable_body_tracking_ = ENABLE_BODY_TRACKING;

    bool enable_waist_loco_ = ENABLE_WAIST_LOCO;
    bool enable_swap_thumbsticks_ = ENABLE_SWAP_THUMBSTICKS;

    bool enable_remote_controller_offset_ = ENABLE_CLOUDXR_CONTROLLER_FIX;
    GLMPose remote_controller_offset_ = {{CLOUDXR_CONTROLLER_OFFSET_X, CLOUDXR_CONTROLLER_OFFSET_Y, CLOUDXR_CONTROLLER_OFFSET_Z},
                                         {CLOUDXR_CONTROLLER_ROTATION_EULER_X, CLOUDXR_CONTROLLER_ROTATION_EULER_Y, CLOUDXR_CONTROLLER_ROTATION_EULER_Z}};

    std::string app_directory_ = OK_CLOUD_STREAMER_APP_DIRECTORY;
    std::string json_filename_ = OK_CLOUD_STREAMER_CONFIG_FILENAME;
};

} // namespace BVR

#endif // OK_CONFIG_H

