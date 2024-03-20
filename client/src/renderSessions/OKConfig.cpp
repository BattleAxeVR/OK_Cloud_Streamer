//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"

#include "OKConfig.h"

namespace BVR 
{

OKConfig::OKConfig()
{
}

void OKConfig::reset()
{
    ip_address_ = DEFAULT_IP_ADDRESS;
    app_directory_ = OK_CLOUD_STREAMER_APP_DIRECTORY;
    enable_auto_connect_ = AUTO_CONNECT_TO_CLOUDXR;

    desired_refresh_rate_ = DEFAULT_CLOUDXR_FRAMERATE;
    polling_rate_mult_ = DEFAULT_CLOUDXR_POSE_POLL_FREQUENCY_MULT;

    max_res_factor_ = DEFAULT_CLOUDXR_MAX_RES_FACTOR;
    max_bitrate_ = DEFAULT_CLOUDXR_MAX_BITRATE;

    per_eye_width_ = DEFAULT_CLOUDXR_PER_EYE_WIDTH;
    per_eye_height_ = DEFAULT_CLOUDXR_PER_EYE_HEIGHT;
    foveation_ = DEFAULT_CLOUDXR_FOVEATION;

    prediction_offset_ns_ = DEFAULT_CLOUDXR_PREDICTION_OFFSET_NS;
    pose_time_offset_s_ = DEFAULT_CLOUDXR_POSE_TIME_OFFSET_SECONDS;

    latch_timeout_ms_ = DEFAULT_CLOUDXR_LATCH_TIMEOUT_MS;

    remote_controller_offset_ = {{CLOUDXR_CONTROLLER_OFFSET_X, CLOUDXR_CONTROLLER_OFFSET_Y, CLOUDXR_CONTROLLER_OFFSET_Z},
                                         {CLOUDXR_CONTROLLER_ROTATION_EULER_X, CLOUDXR_CONTROLLER_ROTATION_EULER_Y, CLOUDXR_CONTROLLER_ROTATION_EULER_Z}};

    enable_remote_controller_offset_ = ENABLE_CLOUDXR_CONTROLLER_FIX;

    enable_sharpening_ = ENABLE_CLOUDXR_LINK_SHARPENING;
    enable_audio_playback_ = ENABLE_CLOUDXR_AUDIO_PLAYBACK;
    enable_audio_recording_ = ENABLE_CLOUDXR_AUDIO_RECORDING;

    enable_eye_tracking_ = ENABLE_EYE_TRACKING;
    enable_face_tracking_ = ENABLE_FACE_TRACKING;
    enable_hand_tracking_ = ENABLE_HAND_TRACKING;
    enable_body_tracking_ = ENABLE_BODY_TRACKING;
    enable_waist_loco_ = ENABLE_WAIST_LOCO;

    enable_swap_thumbsticks_ = ENABLE_SWAP_THUMBSTICKS;
}

bool OKConfig::load()
{
	return true;
}

bool OKConfig::save()
{
	return true;
}


} // namespace BVR

