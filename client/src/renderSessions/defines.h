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
#define DEFAULT_CLOUDXR_MAX_BITRATE 0
#define DEFAULT_CLOUDXR_FOVEATION 0
#define DEFAULT_CLOUDXR_PREDICTION_OFFSET 0.04f
#define DEFAULT_CLOUDXR_POSE_POLL_FREQUENCY 0

#define DEFAULT_CLOUDXR_IPD_MM 67.0f
#define DEFAULT_CLOUDXR_IPD_M (DEFAULT_CLOUDXR_IPD_MM * 0.001f)

#define DEFAULT_CLOUDXR_PER_EYE_WIDTH 2064
#define DEFAULT_CLOUDXR_PER_EYE_HEIGHT 2064
#define DEFAULT_CLOUDXR_FRAMERATE 72.0f

#define ENABLE_HAPTICS 0

#define LEFT 0
#define RIGHT 1
#define NUM_SIDES 2

#define AUTO_CONNECT_TO_CLOUDXR 1
#define USE_MAIN_SURFACE_RENDER_RES_AS_CLOUDXR_RES 0

#endif // DEFINES_H