//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef DEFINES_H
#define DEFINES_H

#ifndef ENABLE_CLOUDXR
#define ENABLE_CLOUDXR 0
#endif

#ifndef ENABLE_OBOE
#define ENABLE_OBOE 0
#endif

#ifndef LEFT_EYE
#define LEFT_EYE 0
#endif

#ifndef RIGHT_EYE
#define RIGHT_EYE 1
#endif

#ifndef NUM_EYES
#define NUM_EYES 2
#endif

#define DEFAULT_SERVER_IP_ADDRESS "192.168.2.38"

//#define OK_CLOUD_STREAMER_APP_DIRECTORY "/sdcard/Android/data/com.battleaxevr.okcloudstreamer.gles/files/"

#define OK_CLOUD_STREAMER_CONFIG_FILENAME "ok_cloud_streamer_config.json"
//#define OK_CLOUD_STREAMER_CONFIG_FULLPATH "/sdcard/Android/data/com.battleaxevr.okcloudstreamer.gles/files/ok_cloud_streamer_config.json"
#define OK_CLOUD_STREAMER_CONFIG_FULLPATH "/sdcard/Android/data/com.battleaxevr.ok_cloud_streamer.opengles/files/ok_cloud_streamer_config.json"

//#define OK_CLOUD_STREAMER_LOG_DIR "/sdcard/Android/data/com.battleaxevr.okcloudstreamer.gles/files/logs/"
#define OK_CLOUD_STREAMER_LOG_DIR "/sdcard/Android/data/com.battleaxevr.ok_cloud_streamer.opengles/files/logs/"

#define DEFAULT_CLOUDXR_MAX_RES_FACTOR 1.0f
#define DEFAULT_CLOUDXR_MAX_BITRATE_KBPS 50000 // 0 = unlimited

#define DEFAULT_CLOUDXR_FOVEATION 0 // 0=100, 0=OFF. 25-50 is ok.

#define DEFAULT_CLOUDXR_PREDICTION_OFFSET_NS 0.0f
#define USE_FRAME_PERIOD_AS_POSE_PREDICTION_OFFSET 0

#define DEFAULT_CLOUDXR_POSE_POLL_FREQUENCY_MULT 0
#define MIN_CLOUDXR_POSE_POLLING_HZ 0
#define MAX_CLOUDXR_POSE_POLLING_HZ 1000

#define DEFAULT_CLOUDXR_POSE_TIME_OFFSET_SECONDS 0.0f//(0.02f)
#define DEFAULT_CLOUDXR_LATCH_TIMEOUT_MS 500

#define DEFAULT_CLOUDXR_PER_EYE_WIDTH 1920
#define DEFAULT_CLOUDXR_PER_EYE_HEIGHT 1920
#define DEFAULT_CLOUDXR_FRAMERATE 72.0f

#define RECOMPUTE_IPD_EVERY_FRAME 1

#define ENABLE_CLOUDXR_POSE_PREDICTION 1 // disable this at your own peril! bleh
#define ANGULAR_VELOCITY_IN_DEVICE_SPACE 0

#define METERS_PER_MILLIMETER 0.001f
#define MILLIMETERS_PER_METER 1000.0f

#define NS_TO_SEC (1.0f / 1000000000.0f)

#define DEFAULT_CLOUDXR_IPD_MM 67.0f
#define DEFAULT_CLOUDXR_IPD_M (DEFAULT_CLOUDXR_IPD_MM * METERS_PER_MILLIMETER)

#define AUTO_CONNECT_TO_CLOUDXR 1
#define USE_CLOUDXR_POSE_ID 1
#define ENABLE_CLOUDXR_LOGGING 0
#define CLOUDXR_LOG_FRAME_NOT_READY_EVENT 0

#define ENABLE_CLOUDXR_HMD 1
#define ENABLE_CLOUDXR_CONTROLLERS (ENABLE_CLOUDXR_HMD && 1)
#define ENABLE_HAPTICS (ENABLE_CLOUDXR_CONTROLLERS && 1)

#define INVALID_INDEX -1
#define MAX_CLOUDXR_CONTROLLER_EVENTS 256
#define SEND_ALL_DIGITAL_EVENTS_EVERY_FRAME 1
#define SEND_ALL_ANALOG_EVENTS_EVERY_FRAME 1

#define COMBINE_GRIP_FORCE_WITH_GRIP 1
#define SIMULATE_GRIP_TOUCH 1
#define SIMULATE_THUMB_REST 0

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define ENABLE_CLOUDXR_AUDIO_PLAYBACK (ENABLE_OBOE && 1)
#define ENABLE_CLOUDXR_AUDIO_RECORDING (ENABLE_OBOE && 0)

#define ENABLE_CLOUDXR_CONTROLLER_FIX (ENABLE_CLOUDXR_CONTROLLERS && 1)

#define CLOUDXR_CONTROLLER_OFFSET_X -0.007f
#define CLOUDXR_CONTROLLER_OFFSET_Y 0.042f
#define CLOUDXR_CONTROLLER_OFFSET_Z -0.014f

#define CLOUDXR_CONTROLLER_ROTATION_EULER_X 40.0f
#define CLOUDXR_CONTROLLER_ROTATION_EULER_Y 0.0f
#define CLOUDXR_CONTROLLER_ROTATION_EULER_Z 0.0f

#define DRAW_CUBE_UNTIL_CONNECTED (!ENABLE_CLOUDXR || 1)

#define ENABLE_CLOUDXR_LINK_SHARPENING 0

#define ENABLE_EYE_TRACKING 0
#define ENABLE_FACE_TRACKING 0
#define ENABLE_HAND_TRACKING 0
#define ENABLE_BODY_TRACKING 0
#define ENABLE_WAIST_LOCO (ENABLE_BODY_TRACKING && 0)

#define ENABLE_SWAP_THUMBSTICKS 0

#define USE_EGL_HELPER 0

#endif // DEFINES_H