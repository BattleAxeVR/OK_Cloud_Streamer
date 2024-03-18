//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef DEFINES_H
#define DEFINES_H


#ifndef ENABLE_CLOUDXR
#define ENABLE_CLOUDXR 1
#endif

#ifndef ENABLE_OBOE
#define ENABLE_OBOE 1
#endif

#define DEFAULT_IP_ADDRESS "192.168.2.38"

#define DEFAULT_CLOUDXR_MAX_RES_FACTOR 1.0f
#define DEFAULT_CLOUDXR_MAX_BITRATE 50000
#define DEFAULT_CLOUDXR_FOVEATION 0
#define DEFAULT_CLOUDXR_PREDICTION_OFFSET_NS 0.0f//4000.0f
#define DEFAULT_CLOUDXR_POSE_POLL_FREQUENCY_MULT 0
#define DEFAULT_CLOUDXR_POSE_TIME_OFFSET_SECONDS 0.0f//(0.02f)
#define DEFAULT_CLOUDXR_LATCH_TIMEOUT_MS 500

#define DEFAULT_CLOUDXR_PER_EYE_WIDTH 1920
#define DEFAULT_CLOUDXR_PER_EYE_HEIGHT 1920
#define DEFAULT_CLOUDXR_FRAMERATE 90.0f

#define RECOMPUTE_IPD_EVERY_FRAME 1

#define METERS_PER_MILLIMETER 0.001f
#define MILLIMETERS_PER_METER 1000.0f

#define NS_TO_SEC (1.0f / 1000000000.0f)

#define DEFAULT_CLOUDXR_IPD_MM 67.0f
#define DEFAULT_CLOUDXR_IPD_M (DEFAULT_CLOUDXR_IPD_MM * METERS_PER_MILLIMETER)

#define AUTO_CONNECT_TO_CLOUDXR 1
#define USE_CLOUDXR_POSE_ID 1

#define ENABLE_CLOUDXR_FRAME_LATCH 1
#define ENABLE_CLOUDXR_FRAME_BLIT (ENABLE_CLOUDXR_FRAME_LATCH && 1)

#define ENABLE_CLOUDXR_HMD 1
#define ENABLE_CLOUDXR_CONTROLLERS (ENABLE_CLOUDXR_HMD && 1)
#define ENABLE_HAPTICS (ENABLE_CLOUDXR_CONTROLLERS && 1)

#define INVALID_INDEX -1
#define MAX_CLOUDXR_CONTROLLER_EVENTS 128

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define ENABLE_CLOUDXR_AUDIO_PLAYBACK (ENABLE_OBOE && 1)
#define ENABLE_CLOUDXR_AUDIO_RECORDING (ENABLE_OBOE && 0)

#define ENABLE_CLOUDXR_CONTROLLER_FIX 1

#define CLOUDXR_CONTROLLER_OFFSET_X -0.007f
#define CLOUDXR_CONTROLLER_OFFSET_Y 0.042f
#define CLOUDXR_CONTROLLER_OFFSET_Z -0.014f

#define CLOUDXR_CONTROLLER_ROTATION_EULER_X 40.0f
#define CLOUDXR_CONTROLLER_ROTATION_EULER_Y 0.0f
#define CLOUDXR_CONTROLLER_ROTATION_EULER_Z 0.0f

#endif // DEFINES_H