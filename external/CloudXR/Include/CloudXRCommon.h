/*
 * Copyright (c) 2019-2022, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifndef __CLOUDXR_COMMON_H__
#define __CLOUDXR_COMMON_H__

// note, cuda compiler may pull in this header,
// but does not need nor want windows.h...
#ifndef __CUDACC__
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

 // Win32 API
#include <Windows.h>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif
#endif // __CUDACC__

#include <string.h>

#define _TOSTR(x, y, z) #x "." #y "." #z
#define TOSTR(x, y, z) _TOSTR(x, y, z)

#define CLOUDXR_VERSION_MAJOR 4
#define CLOUDXR_VERSION_MINOR 0
#define CLOUDXR_VERSION_PATCH 0
#define CLOUDXR_VERSION TOSTR(CLOUDXR_VERSION_MAJOR, CLOUDXR_VERSION_MINOR, CLOUDXR_VERSION_PATCH)
#define CLOUDXR_VERSION_DWORD ((uint32_t) ((CLOUDXR_VERSION_MAJOR << 16) | (CLOUDXR_VERSION_MINOR << 8) | (CLOUDXR_VERSION_PATCH)))

#include <stdint.h>

#if !defined(CLOUDXR_PUBLIC_API)
#if defined(_WIN32)

#ifdef CXR_LIBRARY_BUILD
#define CLOUDXR_PUBLIC_API extern "C" __declspec(dllexport)

#else
#define CLOUDXR_PUBLIC_API extern "C" __declspec(dllimport)
#endif

#elif defined(__linux__)

#ifdef CXR_LIBRARY_BUILD
#define CLOUDXR_PUBLIC_API extern "C" __attribute__((visibility("default")))
#else
#define CLOUDXR_PUBLIC_API extern "C"
#endif

#elif defined(__APPLE__)
#define CLOUDXR_PUBLIC_API 

#else
// as sanity fallback, define blank
#define CLOUDXR_PUBLIC_API

#endif
#endif //!defined(CLOUDXR_PUBLIC_API)

#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup groupStruct Core CloudXR Structures
    @{
 */

typedef uint8_t cxrBool;
#define cxrFalse 0
#define cxrTrue 1

const uint32_t cxrFrameMask_Mono = 0x01;                    ///< Maps to index/stream 0 for mono mode
const uint32_t cxrFrameMask_Left = 0x01;                    ///< Maps to index/stream 0 for stereo mode
const uint32_t cxrFrameMask_Right = 0x02;                   ///< Maps to index/stream 1 for stereo mode
const uint32_t cxrFrameMask_Mono_With_Alpha = 0x01 | 0x02;  ///< Maps to index/stream 0 for mono mode with alpha
const uint32_t cxrFrameMask_Left_With_Alpha = 0x01 | 0x02;  ///< Maps to index/stream 0 for stereo mode with alpha
const uint32_t cxrFrameMask_Right_With_Alpha = 0x04 | 0x08; ///< Maps to index/stream 1 for stereo mode with alpha
const uint32_t cxrFrameMask_All = 0xFFFFFFFF;               ///< Requests the system map to all available frames


/// CloudXR uses a right-handed system, where:
/// +y is up
/// +x is to the right
/// -z is forward
/// Distance units are in meters.
typedef struct cxrMatrix34
{
    float m[3][4]; ///< Matrix components
} cxrMatrix34;

typedef struct cxrVector2
{
    float v[2]; ///< x, y values of the vector.
} cxrVector2;

typedef struct cxrVector3
{
    float v[3]; ///< x, y, z values of the vector.
} cxrVector3;

typedef struct cxrQuaternion
{
    float w; ///< w value.
    float x; ///< x value.
    float y; ///< y value.
    float z; ///< z value.
} cxrQuaternion;

/// Keys for CloudXR specific vr::ETrackedDeviceProperty values
typedef enum
{
    Prop_CloudXRServerState_Int32 = 10100,  ///< State of the CXR server driver as @ref cxrServerState
    Prop_ArLightColor_Vector3,              ///< Primary light color sent by client API @ref cxrSendLightProperties
    Prop_ArLightDirection_Vector3,          ///< Primary light direction sent by client API @ref cxrSendLightProperties
    Prop_ArAmbientLightSh0_Vector3          ///< First spherical harmonic component followed by a total of CXR_MAX_AMBIENT_LIGHT_SH sent by client API @ref cxrSendLightProperties
} cxrTrackedDeviceProperty;

/// Maximum number of spherical harmonic components for ambient lighting information
const unsigned int CXR_MAX_AMBIENT_LIGHT_SH = 9; 

typedef struct cxrLightProperties
{
    cxrVector3 primaryLightColor;                           ///< Primary light color
    cxrVector3 primaryLightDirection;                       ///< Primary light direction
    cxrVector3 ambientLightSh[CXR_MAX_AMBIENT_LIGHT_SH];    ///< Ambient light spherical harmonic components upto @ref CXR_MAX_AMBIENT_LIGHT_SH
} cxrLightProperties;

typedef enum
{
    cxrDeviceActivityLevel_Unknown = -1,                ///< Unknown state.
    cxrDeviceActivityLevel_Idle = 0,                    ///< No activity for the last 10 seconds
    cxrDeviceActivityLevel_UserInteraction = 1,         ///< Activity is happening now (movement or prox sensor)
    cxrDeviceActivityLevel_UserInteraction_Timeout = 2, ///< No activity for the last 0.5 seconds
    cxrDeviceActivityLevel_Standby = 3,                 ///< Idle for at least 5 seconds (configurable in Settings : Power Management)
} cxrDeviceActivityLevel;

typedef enum
{
    cxrTrackingResult_Uninitialized          = 1,       ///< Tracking hasn't been initialized yet.

    cxrTrackingResult_Calibrating_InProgress = 100,     ///< Calibration in progress.
    cxrTrackingResult_Calibrating_OutOfRange = 101,     ///< Devices are outside of the calibration range.

    cxrTrackingResult_Running_OK             = 200,     ///< Tracking is running OK.
    cxrTrackingResult_Running_OutOfRange     = 201,     ///< Devices are out of the tracking range.

    cxrTrackingResult_Fallback_RotationOnly  = 300,     ///< Tracking is limited to rotation-only, usually IMU-based, as a fallback.
} cxrTrackingResult;

typedef enum
{
    cxrUniverseOrigin_Seated,   ///< Indicates the user is in a seated position
    cxrUniverseOrigin_Standing  ///< Indicates the user is in a standing position
} cxrUniverseOrigin;

typedef struct cxrChaperone
{
    cxrUniverseOrigin universe;     ///< Whether the user is seated or standing
    cxrMatrix34 origin;             ///< The origin of the chaperone boundary
    cxrVector2 playArea;            ///< The extents of the chaperone boundary
} cxrChaperone;

typedef struct cxrTrackedDevicePose
{
    cxrVector3 position;                ///< Device 3D position.
    cxrQuaternion rotation;             ///< Normalized quaternion representing the orientation.
    cxrVector3 velocity;                ///< Device velocity in tracker space in m/s
    cxrVector3 angularVelocity;         ///< Device angular velocity in radians/s
    cxrVector3 acceleration;            ///< Device acceleration in tracker space in m/s^2
    cxrVector3 angularAcceleration;     ///< Device angular acceleration in radians/s^2
    cxrTrackingResult trackingResult;   ///< Device's tracking result.
    cxrBool poseIsValid;                ///< Whether this pose is valid or not.
    cxrBool deviceIsConnected;          ///< Whether the device is physically connected or not.
} cxrTrackedDevicePose;

const int CXR_NUM_CONTROLLERS = 2;  ///< The number of controllers tracking states that can be sent with the hmd tracking state

typedef enum
{
    cxrHmdTrackingFlags_HasProjection = 0x00000001, ///< Tracking state has updated projection parameters.
    cxrHmdTrackingFlags_HasIPD        = 0x00000002, ///< Tracking state has updated IPD.
    cxrHmdTrackingFlags_HasRefresh    = 0x00000004, ///< Tracking state has updated refresh rate.
    cxrHmdTrackingFlags_HasPoseID     = 0x00000008, ///< Tracking state contains a 64-bit pose ID.
    cxrHmdTrackingFlags_HasScaling    = 0x00000010, ///< Tracking state contains a float scale.
} cxrHmdTrackingFlags;

typedef struct cxrHmdTrackingState
{
    uint64_t clientTimeNS; ///< Client-side time when this event happened. The client side clock must be monotonically increasing, but no epoch constraints exist.
    uint64_t flags;        ///< A bitwise-mask of values from `cxrHmdTrackingFlags`
    float proj[2][4];      ///< If cxrHmdTrackingFlags_HasProjection is set, allows you to update the projection parameters specified in the device description at a per-pose granularity.
    float ipd;             ///< If cxrHmdTrackingFlags_HasIPD is set, allows you to update the headset interpupilary distance parameters specified in the device description at a per-pose granularity.
    float displayRefresh;  ///< If cxrHmdTrackingFlags_HasRefresh is set, overrides the headset display target refresh/fps specified in the device description (up to per-pose granularity).
    uint64_t poseID;       ///< If cxrHmdTrackingFlags_HasPoseID is set, this contains a user-specified 64-bit value associated with this pose. The same value will be delivered to the client in the poseID of the cxrFramesLatched structure associated with this pose.
    float scaling;         ///< If cxrHmdTrackingFlags_HasScaling is set, allows you to specify an all-axis scale, defaults to 1.0.
    cxrTrackedDevicePose pose; ///< The headset's 3D pose.
    cxrDeviceActivityLevel activityLevel; ///< The headset's activity level.
} cxrHmdTrackingState;

/// Max number of video streams supported
#define CXR_MAX_NUM_VIDEO_STREAMS 4

// Max width for each video stream
#define CXR_MAX_VIDEO_STREAM_WIDTH 4096

// Max height for each video stream
#define CXR_MAX_VIDEO_STREAM_HEIGHT 4096

// Max fps for each video stream
#define CXR_MAX_VIDEO_STREAM_FPS 144.0f

// Max fps for each video stream
#define CXR_DEFAULT_VIDEO_STREAM_FPS 90.0f

// Max bitrate for each video stream
#define CXR_MAX_VIDEO_STREAM_BITRATE 100000

// Max bitrate for each video stream
#define CXR_DEFAULT_VIDEO_STREAM_BITRATE 50000

/// In Receiver_XR mode number of streams is always 2
#define CXR_NUM_VIDEO_STREAMS_XR 2

/// Max length for path inputs
#define CXR_MAX_PATH 4096

/// Max length for controller role
#define CXR_MAX_CONTROLLER_ROLE 63

/// Max length for controller name
#define CXR_MAX_CONTROLLER_NAME 127

/// Max number of controller input paths
#define CXR_MAX_CONTROLLER_INPUT_COUNT 128

/// Max length of controller input path
#define CXR_MAX_INPUT_PATH_LENGTH  127

/// Max number of bytes that can be sent as blob input
#define CXR_MAX_BLOB_BYTES_PER_INPUT 1024

typedef enum
{
    cxrInputValueType_blob = 1,     ///< Freeform input data, cannot exceed @ref CXR_MAX_BLOB_BYTES_PER_INPUT in size
    cxrInputValueType_boolean = 2,  ///< Boolean input data
    cxrInputValueType_int32 = 3,    ///< Int32 input data
    cxrInputValueType_float32 = 4,  ///< Float32 input data
} cxrInputValueType;

typedef struct cxrBlob
{
    uint64_t size;  ///< Blob data size in bytes which cannot exceed @ref CXR_MAX_BLOB_BYTES_PER_INPUT
    void* ptr;      ///< Blob data contents which are copied when @ref cxrFireControllerEvents is called
} cxrBlob;

typedef struct cxrControllerInputValue
{
    cxrInputValueType valueType;    ///< Value type for this input event based on which one of the below properties should be set
    union
    {
        cxrBlob vBlob;              ///< Blob data for this input event
        cxrBool vBool;              ///< Boolean data for this input event
        uint32_t vI32;              ///< Int32 data for this input event
        float vF32;                 ///< Float32 data for this input event
    };
} cxrControllerInputValue;

typedef struct cxrControllerEvent
{
    uint64_t clientTimeNS;              ///< Time this event happened - no epoch is specified, but durations must reflect wall time in nanoseconds.
    uint16_t clientInputIndex;          ///< Index of this input event in the corresponding controller description provided to cxrAddController().
    cxrControllerInputValue inputValue; ///< Value of the input event.
} cxrControllerEvent;

typedef struct cxrControllerDesc
{
    uint64_t id;                                ///< A unique ID for this controller. This ID should persist through a disconnect/reconnect
    const char* role;                           ///< A unique role for the controller. This is used to indicate handedness etc...
    const char* controllerName;                 ///< A unique name for the controller. This is used to determine the server rendering of the controller
    uint32_t inputCount;                        ///< The number of inputs paths supported by this controller
    const char** inputPaths;                    ///< An array of input paths with inputCount elements
    const cxrInputValueType* inputValueTypes;   ///< An array of input types with inputCount elements corresponding to the inputPaths
} cxrControllerDesc;

typedef void* cxrControllerHandle;

typedef struct cxrControllerTrackingState
{
    uint64_t clientTimeNS;      ///< Time this event happened - no epoch is specified, but durations must reflect wall time in nanoseconds.
    cxrTrackedDevicePose pose;  ///< The controller's 3D pose.
} cxrControllerTrackingState;

/// This structure is used to send the hmd tracking state and can also be used to send the poses of a left and right controller.
/// and additional controller poses should be sent using @ref cxrSendControllerPose
typedef struct cxrVRTrackingState
{
    cxrHmdTrackingState hmd;    ///< Tracking state for the headset.
    cxrControllerTrackingState controller[CXR_NUM_CONTROLLERS]; ///< Tracking state for left / right hand controller, index 0 is the left hand.
    float poseTimeOffset;       ///< Offset in seconds from the actual time of the pose that affects the amount of pose extrapolation. Default is 0. 
} cxrVRTrackingState;

typedef enum
{
    cxrFoveation_PiecewiseQuadratic = 0,
} cxrFoveationMode;

typedef enum
{
    cxrClientSurfaceFormat_RGB,  ///< 32bpp packed R8B8G8X8, sRGB colorspace, alpha channel is not transmitted
    cxrClientSurfaceFormat_RGBA, ///< 32bpp packed R8B8G8A8, sRGB colorspace
    cxrClientSurfaceFormat_NV12  ///< 12bpp packed YUV 4:2:0 planar with interleaved chroma, sRGB colorspace
} cxrClientSurfaceFormat;

typedef struct
{
    cxrClientSurfaceFormat  format;

    uint32_t                width;          ///< Requested render width, typically this is display width.
    uint32_t                height;         ///< Requested render height, typically this is display height.
                                            ///< @note Width and height should be **even** values, ideally a multiple of 32.

    float                   fps;            ///< Device display refresh rate, client needs server to provide stream at this rate.

    uint32_t                maxBitrate;     ///< Bitrate for all video traffic is capped at this value. A value of 0 means unlimited.
} cxrClientVideoStreamDesc;

typedef struct cxrDeviceDesc
{
    uint32_t                    numVideoStreamDescs;
    cxrClientVideoStreamDesc    videoStreamDescs[CXR_MAX_NUM_VIDEO_STREAMS];

    float             maxResFactor;                 ///< Suggested maximum oversampling size on server.  Server downsamples larger res to this.  Valid range 0.5-2.0, with 1.2 as a common value.
    float             ipd;                          ///< Inter-pupillary distance
    float             proj[2][4];                   ///< Per eye raw projection extents: tangents of the half-angles from the center view axis. 0|1 for left|right eye, then 0-3 for left, right, top, bottom edges.
    float             predOffset;                   ///< A time offset in seconds to adjust the server time prediction model to account for device-specific delay rendering stream to screen.
    float             maxClientQueueSize;           ///< Maximum size of the decoded frame FIFO queue on the client(unit is frames)
    cxrBool           stereoDisplay;                ///< This device can display stereo output so the server needs to render in stereo
    cxrBool           receiveAudio;                 ///< This device can receive and play audio FROM the server (e.g. game/app sounds)
    cxrBool           sendAudio;                    ///< This device can send captured audio TO the server (e.g. microphone)
    cxrBool           embedInfoInVideo;             ///< Request the server embed debug information in each frame prior to encoding.
    cxrBool           disablePosePrediction;        ///< Use the raw client-provided pose without any pose extrapolation
    cxrBool           angularVelocityInDeviceSpace; ///< True if angular velocity in device space, false if in world space
    cxrBool           disableVVSync;                ///< Disables VVSync and instead runs server at fixed frame rate based on client fps
    uint32_t          foveatedScaleFactor;          ///< Request server foveate rendered frames at % of display res.  Values [25-100], zero to disable.  50 is reasonable for most uses.
    uint32_t          posePollFreq;                 ///< Frequency to poll pose updates.  Set 0 for default, or rate per second to invoke polling.  Default is 250, maximum is 1000.  Recommend at least 2x display refresh.
    uint32_t          foveationModeCaps;            ///< A bitmask of supported foveation modes (1 bit per cxrFoveationMode enum value)
    cxrChaperone      chaperone;                    ///< SteamVR-equivalent chaperone or 'play space'
} cxrDeviceDesc;

typedef struct cxrHapticFeedback
{
    // revised from 'index' to 'ID', and int to uint64, to match revised input system.
    // note device isn't necessarily a controller, AND there may be single devices with multiple haptics (gamepad L/R).
    uint64_t    deviceID;
    float       amplitude;
    float       seconds;
    float       frequency;
} cxrHapticFeedback;

typedef void* cxrTextureHandle;

typedef enum
{
    cxrGraphicsContext_D3D11,
    cxrGraphicsContext_D3D12,
    cxrGraphicsContext_GL,
    cxrGraphicsContext_GLES,
    cxrGraphicsContext_Vulkan,
    cxrGraphicsContext_Metal,
    cxrGraphicsContext_Cuda,
    cxrGraphicsContext_NvMedia,
    // force to a qword
    cxr_GraphicsContext_Max = ~0,
} cxrGraphicsContextType;
 
// A low-level graphics context information.
typedef struct cxrGraphicsContext
{
    cxrGraphicsContextType type;

    union
    {
#if defined(__linux__)
        struct
        {
            void* display;
            void* context;
        } egl;
#endif

#if defined(_WIN32)
        void* wglContext;

        struct ID3D11DeviceContext* d3d11Context;

        struct
        {
            struct ID3D12Device* device;
            struct ID3D12CommandQueue* queue;
        } d3d12;

#endif

        struct
        {
            int device;
            void* ctx;
        } cuda;

#if defined(WITH_VULKAN)
        struct
        {
            void* instance;
            void* physicalDevice;
            void* device;
            void* queue;
        } vk;
#endif

#if defined(__APPLE__) && defined(WITH_METAL)
        struct
        {
            id<MTLDevice> device;
            id<MTLCommandQueue> queue;
        } metal;
#endif

        uint8_t _[32];
    };
} cxrGraphicsContext;

typedef struct cxrVideoFrame
{
    cxrTextureHandle texture;   ///< This frame's texture handle -- can be cast to a platform texture when needed.

    uint32_t width;             ///< Width of frame in texels
    uint32_t height;            ///< Height of frame in texels
    uint32_t pitch;             ///< Width of frame in bytes

    uint32_t widthFinal;        ///< Final width of frame *after* internal transforms, such as de-foveation.
    uint32_t heightFinal;       ///< Final height of frame *after* internal transforms, such as de-foveation.

    uint32_t streamIdx;         ///< Index of video stream this frame comes from.
    uint64_t timeStamp;         ///< Timestamp when the frame was captured on the server.
} cxrVideoFrame;

/**
    This structure is passed into cxrLatchFrame(), and if the latch succeeds it will contain
    data for the latched frames.  The structure's data must be freed by passing to cxrReleaseFrame()
    when done using it.
 */
typedef struct cxrFramesLatched
{
    uint32_t count;                                     ///< The number of frames latched/acquired
    cxrVideoFrame frames[CXR_MAX_NUM_VIDEO_STREAMS];    ///< The video frame data for each frame latched
    cxrMatrix34 poseMatrix;                             ///< The device position at that moment in time
    uint64_t poseID;                                    ///< The user-specified 64-bit value that was passed in the cxrHmdTrackingState structure and that is associated with the headset pose used for rendering this frame.
} cxrFramesLatched;

/**
    These bitfield masks denote various debugging flags handled by either the Receiver
     or by the Server.  Note that many of these may have some significant performance impact
     either constantly or at some interval.
 */
typedef enum
{
    cxrDebugFlags_LogVerbose                = 0x00000001, ///< record very verbose output
    cxrDebugFlags_LogQuiet                  = 0x00000002, ///< do not open normal log files at all
    cxrDebugFlags_LogPrivacyDisabled        = 0x00000010, ///< disable any privacy filtering in output
    cxrDebugFlags_TraceLocalEvents          = 0x00000100, ///< record trace of local event timing
    cxrDebugFlags_TraceStreamEvents         = 0x00000200, ///< record trace of c/s stream event timing
    cxrDebugFlags_TraceQosStats             = 0x00000400, ///< record QoS statistics
    cxrDebugFlags_DumpImages                = 0x00001000, ///< Dump images from stream
    cxrDebugFlags_DumpAudio                 = 0x00002000, ///< Dump audio from stream
    cxrDebugFlags_EmbedServerInfo           = 0x00004000, ///< server embeds stats into frame buffers for streaming
    cxrDebugFlags_EmbedClientInfo           = 0x00008000, ///< client embeds stats into frame buffers or overlays
    cxrDebugFlags_CaptureServerBitstream    = 0x00010000, ///< record the server-sent video bitstream
    cxrDebugFlags_CaptureClientBitstream    = 0x00020000, ///< record the client-received video bitstream
    cxrDebugFlags_FallbackDecoder           = 0x00100000, ///< try to use a fallback decoder for platform
    cxrDebugFlags_EnableAImageReaderDecoder = 0x00400000, ///< `[ANDROID only]` enable the AImageReader decoder
    cxrDebugFlags_OutputLinearRGBColor      = 0x00800000, ///< `[ANDROID only]` cxrBlitFrame output will be linear instead of sRGB
    cxrDebugFlags_DisableMultistreamQoS     = 0x01000000, ///< QoS will be computed on each stream independently
} cxrDebugFlags;


const uint32_t CXR_AUDIO_CHANNEL_COUNT = 2;             ///< Audio is currently always stereo
const uint32_t CXR_AUDIO_SAMPLE_SIZE = sizeof(int16_t); ///< Audio is currently singed 16-bit samples (little-endian)
const uint32_t CXR_AUDIO_SAMPLING_RATE = 48000;         ///< Audio is currently always 48khz
const uint32_t CXR_AUDIO_FRAME_LENGTH_MS = 5;           ///< Sent audio has a 5 ms default frame length.  Received audio has 5 or 10 ms frame length, depending on the configuration.
const uint32_t CXR_AUDIO_BYTES_PER_MS = CXR_AUDIO_CHANNEL_COUNT * CXR_AUDIO_SAMPLE_SIZE * CXR_AUDIO_SAMPLING_RATE / 1000; ///< Total bytes of audio per ms


typedef struct cxrAudioFrame
{
    int16_t* streamBuffer;
    uint32_t streamSizeBytes;
} cxrAudioFrame;


typedef enum
{
    cxrServerState_NotRunning = 0,      ///< Initial state, not yet running
    cxrServerState_Connecting = 1,      ///< Server is connecting to client, finished the RTSP handshake
    cxrServerState_Running = 2,         ///< Server is connected and ready to accept video and audio input for streaming     
    cxrServerState_HMD_Active = 3,      ///< `[VR only]` HMD is active
    cxrServerState_HMD_Idle = 4,        ///< `[VR only]` HMD is idle
    cxrServerState_Disconnected = 5,    ///< Server is disconnected from the client
    cxrServerState_Error = 6            ///< Server in an error state
} cxrServerState;

typedef enum
{
    cxrClientState_ReadyToConnect = 0,                  ///< Initial state, no connection attempt pending
    cxrClientState_ConnectionAttemptInProgress = 1,     ///< Attempting to connect to server
    cxrClientState_ConnectionAttemptFailed = 2,         ///< Error occurred during connection attempt
    cxrClientState_StreamingSessionInProgress = 3,      ///< Connected, streaming session in progress
    cxrClientState_Disconnected = 4,                    ///< Streaming session has ended
    cxrClientState_Exiting = 5                          ///< Client is shutting down.
} cxrClientState;

typedef enum
{
    cxrNetworkInterface_Unknown = 0,     ///< Network interface is unknown.
    cxrNetworkInterface_Ethernet = 1,    ///< Network interface is Ethernet.
    cxrNetworkInterface_WiFi5Ghz = 2,    ///< Network interface is 5GHz Wi-Fi.
    cxrNetworkInterface_WiFi24Ghz = 3,   ///< Network interface is 2.4GHz Wi-Fi.
    cxrNetworkInterface_MobileLTE = 4,   ///< Network interface is mobile LTE.
    cxrNetworkInterface_Mobile5G = 5,    ///< Network interface is mobile 5G.
} cxrNetworkInterface;

typedef enum
{
    cxrNetworkTopology_Unknown = 0,   ///< Client Server connection unknown
    cxrNetworkTopology_LAN = 1,       ///< Client and Server are connected over LAN
    cxrNetworkTopology_WAN = 2        ///< Client and Server are connected over WAN
} cxrNetworkTopology;

typedef enum
{
    cxrError_Success = 0x0,                                     ///< The operation succeeded

    cxrError_Group_General = 0x100,                             ///< **Errors which can be returned from most API functions**

    cxrError_Failed = 0x101,                                    ///< The operation failed
    cxrError_Timeout = 0x102,                                   ///< The operation timed out
    cxrError_Not_Connected = 0x103,                             ///< The client is not connected
    cxrError_Not_Streaming = 0x104,                             ///< The client is not streaming
    cxrError_Not_Implemented = 0x105,                           ///< The method is not implemened
    cxrError_Required_Parameter = 0x106,                        ///< A required parameter was missing
    cxrError_Module_Load_Failed = 0x107,                        ///< A library could not be loaded

    cxrError_Group_Validation = 0x200,                          ///< **Errors validating API input parmeters** 

    cxrError_Invalid_API_Version = 0x201,                       ///< The requested API version is invalid
    cxrError_Invalid_Number_Of_Streams = 0x202,                 ///< The number of streams is invalid
    cxrError_Invalid_Device_Descriptor = 0x203,                 ///< The device descriptor is invalid
    cxrError_Invalid_Graphics_Context = 0x204,                  ///< The graphics context is invalid
    cxrError_Invalid_Video_Format = 0x205,                      ///< The video format is invalid
    cxrError_Invalid_Video_Width = 0x206,                       ///< The video width is invalid
    cxrError_Invalid_Video_Height = 0x207,                      ///< The video height is invalid
    cxrError_Invalid_Video_Fps = 0x208,                         ///< The video fps is invalid
    cxrError_Invalid_Video_Max_Bitrate = 0x209,                 ///< The video max bitrate is invalid
    cxrError_Invalid_Frame_Mask = 0x20A,                        ///< The frame mask is invalid

    cxrError_Group_Server = 0x400,                              ///< **Errors originating from the server**

    cxrError_Server_Setup_Failed = 0x401,                       ///< The setup of the server failed

    cxrError_Group_Server_Connection = 0x410,                   ///< **Errors connecting caused by issues on the server**

    cxrError_Server_Version_Old = 0x411,                        ///< The server version is too old for the client
    cxrError_Server_Untrusted_Certificate = 0x412,              ///< The server SSL certificate was not trusted by the client
    cxrError_Server_Initiated_Disconnect = 0x413,               ///< The server initiated a disconnect
    cxrError_Server_Handshake_Failed = 0x414,                   ///< The handshake with the server failed
    cxrError_Server_Stream_Configuration_Failed = 0x415,        ///< The server could not be setup to offer the requested streams
    cxrError_Server_Firewall_Configuration_Failed = 0x416,      ///< The server firewall could not be traveresed by hole-punching

    cxrError_Group_Server_Feature_Disabled = 0x420,             ///< **Errors due to features being disabled by the server**

    cxrError_Server_Feature_Disabled_HEVC = 0x421,              ///< The server disabled the feature HEVC
    cxrError_Server_Feature_Disabled_VVSync = 0x422,            ///< The server disabled the feature VVSync
    cxrError_Server_Feature_Disabled_Pose_Prediction = 0x423,   ///< The server disabled the feature Pose Prediction
    cxrError_Server_Feature_Disabled_Send_Audio = 0x424,        ///< The server disabled the feature Send Audio
    cxrError_Server_Feature_Disabled_Receive_Audio = 0x425,     ///< The server disabled the feature Receive Audio

    cxrError_Group_Client = 0x800,                              ///< **Errors originating from the client**

    cxrError_Client_Setup_Failed = 0x801,                       ///< The client setup failed

    cxrError_Group_Client_Connection = 0x810,                   ///< **Errors connecting caused by issues on the client**

    cxrError_Client_Version_Old = 0x811,                        ///< The client version is too old for the server
    cxrError_Client_Unauthorized = 0x812,                       ///< The client was not authorized

    cxrError_Group_Frame = 0x1000,                              ///< **Errors in the frame lifecycle**

    cxrError_Frame_Not_Released = 0x1001,                       ///< The frame was not released
    cxrError_Frame_Not_Latched = 0x1002,                        ///< The frame was not latched
    cxrError_Frame_Not_Ready = 0x1003,                          ///< The frame was not ready

    cxrError_Group_Decoder = 0x2000,                            ///< **Errors in the decoder lifecycle** 

    cxrError_Decoder_Setup_Failed = 0x2001,                     ///< The decoder setup failed
    cxrError_Decoder_No_Texture = 0x2002,                       ///< The decoder could not acquire a texture
    cxrError_Decoder_Frame_Not_Ready = 0x2003,                  ///< The decoder was not ready with a frame

    cxrError_Group_Input = 0x4000,                              ///< **Errors in sending pose or input to the server**

    cxrError_Pose_Callback_Provided = 0x4001,                   ///< The pose could not be sent because a callback was provided
    cxrError_Name_Too_Long = 0x4002,                            ///< Name string exceeds max size
    cxrError_Too_Many_Inputs = 0x4003,                          ///< Too many inputs provided for single controller
    cxrError_Controller_Id_In_Use = 0x4004,                     ///< Controller ID already in use
    cxrError_Role_Too_Long = 0x4005,                            ///< Controller role too long

    cxrError_Group_Data = 0x8000,                               ///< **Errors in sending other forms of data to the server**

    cxrError_Data_Too_Large = 0x8001,                           ///< The data was too large to send
} cxrError;

CLOUDXR_PUBLIC_API const char* cxrErrorString(cxrError E);

/// Constant for logging systems to use their default settings for maximum size, age, and similar properties.
#define CLOUDXR_LOG_MAX_DEFAULT -1

/// This enum tries to define logging verbosity in a way similar
/// to various platforms, initially Android, but also influenced
/// by iOS and others.
typedef enum
{
    cxrLL_Verbose = 0,
    cxrLL_Debug = 1,
    cxrLL_Info = 2,
    cxrLL_Warning = 3,
    cxrLL_Error = 4,
    cxrLL_Critical = 5,
    cxrLL_Silence = 6, ///< The Silence level means no logging at all, no log file created, we print one line just to note this mode.
} cxrLogLevel;

// for the moment, we just return single character, similar to android logcat.
// TODO could potentially do a small array lookup, since it's zero-based enum -> single char.
// TBD <static>.  Swift compiler needs it, yet doesn't fully like it.  Need to test on
// other compilers to ensure they don't throw errors on the odd syntax mix.
/// This helper converts a cxrLogLevel enum value into a single ASCII char to easily display the priority of a given message.
static inline char cxrLLToChar(cxrLogLevel ll)
{
    switch (ll)
    {
        case cxrLL_Silence: return 'S';
        case cxrLL_Critical: return 'C';
        case cxrLL_Error: return 'E';
        case cxrLL_Warning: return 'W';
        case cxrLL_Info: return 'I';
        case cxrLL_Debug: return 'D';
        default: return 'V';
    }
}

typedef enum
{
    cxrMC_Correctness = 0,
    cxrMC_Performance = 1,
} cxrMessageCategory;

/// This is the definition for a generic message/logging callback function, and is declared
/// here to be identically structured for both client and server implementations.
typedef void (*cxrMessageCallbackFunc)(void* context, cxrLogLevel level, cxrMessageCategory category, void* extra, const char* tag, const char* const messageText);
/// The maximum size for a given log message C-string
#define MAX_LOG_LINE_LEN    4096
/// The maximum size for a given log tag C-string
#define MAX_TAG_LEN         256


const char* const cxrUserDataFileName = "CloudXR User Data";
const char* const cxrUserDataMutexName = "CloudXR User Data Mutex";
const uint32_t cxrUserDataMaxSize = 8 * 1024;

/** @} */ // end of groupStruct

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __CLOUDXR_COMMON_H__
