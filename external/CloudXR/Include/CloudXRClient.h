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

#ifndef __CLOUDXR_CLIENT_H__
#define __CLOUDXR_CLIENT_H__

#include "CloudXRCommon.h"
#include "CloudXRInputEvents.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*
    A typical main loop might be something like this:
    @code
        set up clientDesc
        set up clientCallbacks
            // >> if connecting asynchronously, implement UpdateClientState
        set up receiverDesc
        @ref cxrCreateReceiver()
        @ref cxrConnect()
        while (!quitting)
            if (state==connected)
                @ref cxrLatchFrame()
                @ref cxrBlitFrame() // Called per eye, Android only.
                @ref cxrReleaseFrame()
            else if (state==connecting)
                // >> render connection progress
            else if (state==failed)
                // >> handle notifying user of connection error
                quitting = true
        @ref cxrDestroyReceiver()
    @endcode
*/

/** @defgroup groupStruct Core CloudXR Structures
    @{
 */

/**
	@brief CloudXR SDK client interface

    The *Receiver* is the primary interface handle for a client of the CloudXR SDK to
     connect to a server, describe client device properties, send position and input
     updates, stream live video/audio, and acquire video frames to render out locally.
*/
typedef struct cxrReceiver* cxrReceiverHandle;

/**
    Callback functions for the *Receiver* to interact with client code.

    These functions are used to either request something from the client or to provide
     data or notifications to the client. 
*/
typedef struct cxrClientCallbacks
{
    /// Return HMD and controller states and poses. Called at a fixed frequency by the Receiver,
    /// as specified by cxrDeviceDesc#posePollFreq.  This callback must be implemented for the
    /// server to render changing position and orientation of client device and controller(s).
    void (*GetTrackingState)(void* context, cxrVRTrackingState* trackingState);

    /// Notify the client it should trigger controller vibration
    void (*TriggerHaptic)(void* context, const cxrHapticFeedback* haptic);

    /// Notify the client to play out the passed audio buffer.
    /// @return cxrTrue if data consumed/played, cxrFalse if cannot play or otherwise discarded
    cxrBool (*RenderAudio)(void* context, const cxrAudioFrame* audioFrame);

    /// ``Reserved for Future Use``
    void (*ReceiveUserData)(void* context, const void* data, uint32_t size);

    /// Notify the client of underlying state changes and why the changed occurred,
    /// for both abnormal and expected cases. This callback must be implemented if you
    /// call cxrConnect() with the `async` flag set in @ref cxrConnectionDesc in order
    /// to be notified of connection state changes in the background thread.
    void (*UpdateClientState)(void* context, cxrClientState state, cxrError error);

    /// Notify the client of message strings to be logged, via whatever log facility
    /// the client has implemented.
    /// The typedef for LogMessage is in CloudXRCommon.h:
    /// void (*func)(void* context, cxrLogLevel level, cxrMessageCategory category, void* extra, const char* tag, const char* const messageText);
    cxrMessageCallbackFunc LogMessage;

    /// This is an optional app-specific pointer that is sent back to client in all the callback functions
    /// as the first parameter (void* context).  It would typically be a pointer to a core object/struct
    /// necessary to access/implement functionality inside of the callback.
    void* clientContext;                    
} cxrClientCallbacks;

/**
    This contains details of the client device and overall session characteristics.  It is
     passed to @ref cxrCreateReceiver for construction of the @ref cxrReceiverHandle instance
     and connection to server for streaming.
*/
typedef struct cxrReceiverDesc
{
    uint32_t requestedVersion;              ///< Must set to CLOUDXR_VERSION_DWORD

    cxrDeviceDesc deviceDesc;               ///< Describes the client device hardware
    cxrClientCallbacks clientCallbacks;     ///< Pointers to client callback functions (NULL for unsupported callbacks)
    const cxrGraphicsContext* shareContext; ///< The API-specific client graphics context to be shared with CloudXR.
                                            ///< @note On certain graphics APIs, such as DirectX, the share context may be NULL.
                                            ///< However, the share context cannot be NULL on OpenGL ES on Android devices.

    uint32_t debugFlags;                    ///< Features to aid debugging (see @ref cxrDebugFlags)
    // TODO, these items, and other 'logging state' will move or go away as we shift to log callback in the clients.
    int32_t logMaxSizeKB;                   ///< Maximum size for the client log in KB. -1 for default, 0 to disable size limit.
    int32_t logMaxAgeDays;                  ///< Delete log and diagnostic files older than this. -1 for default, 0 to disable.

    // no better location for this for the moment to parallel server side closely
    char appOutputPath[CXR_MAX_PATH];       ///< The client subdirectory where the library can output logs, captures, traces, etc.
} cxrReceiverDesc;

typedef enum
{
    cxrConnectionQuality_Unstable = 0,   ///< Initial value while estimating quality. Thereafter, expect disconnects. Details in @ref cxrConnectionQualityReason.
    cxrConnectionQuality_Bad = 1,        ///< Expect very low bitrate and/or high latency. Details in @ref cxrConnectionQualityReason.
    cxrConnectionQuality_Poor = 2,       ///< Expect low bitrate and/or high latency. Details in @ref cxrConnectionQualityReason.
    cxrConnectionQuality_Fair = 3,       ///< Expect frequent impact on bitrate or latency. Details in @ref cxrConnectionQualityReason.
    cxrConnectionQuality_Good = 4,       ///< Expect ocassional impacts on bitrate or latency
    cxrConnectionQuality_Excellent = 5   ///< Expect infrequent impacts on bitrate or latency
} cxrConnectionQuality;

typedef enum
{
    cxrConnectionQualityReason_EstimatingQuality = 0x0,   ///< Inital value while estimating quality.
    cxrConnectionQualityReason_LowBandwidth = 0x1,        ///< The percentage of unutilized bandwidth is too low to maintain bitrate
    cxrConnectionQualityReason_HighLatency = 0x2,         ///< The round trip time is too high to maintain low latency
    cxrConnectionQualityReason_HighPacketLoss = 0x4       ///< The packet loss is too high to overcome without re-transmission
} cxrConnectionQualityReason;

/**
    This structure is passed into @ref cxrGetConnectionStats. There are three categories of statistics:
     
     The frame stats relate to the timing of video frames as they move from server to client and
     are prefixed with frame. 
     
     The network stats relate to the performance of the connection between server and client and
     are prefixed with bandwidth, roundTrip, jitter and totalPackets.

     The quality stats relate to the overall health of the connection and are prefixed with
     quality.
*/
typedef struct cxrConnectionStats
{
    float framesPerSecond;                  ///< The number of frames per second
    float frameDeliveryTimeMs;              ///< The average time a frame takes to reach the client (for XR streaming this includes pose latency)
    float frameQueueTimeMs;                 ///< The average time a frame spends queued on the client
    float frameLatchTimeMs;                 ///< The time the client application spent waiting for a frame to be latched

    uint32_t bandwidthAvailableKbps;        ///< The estimated available bandwidth from server to client.
    uint32_t bandwidthUtilizationKbps;      ///< The average video streaming rate from server to client.
    uint32_t bandwidthUtilizationPercent;   ///< The estimated bandwidth utilization percent from server to client.
    uint32_t roundTripDelayMs;              ///< The estimated network round trip delay between server and client in milliseconds.
    uint32_t jitterUs;                      ///< The estimated jitter from server to client in microseconds.
    uint32_t totalPacketsReceived;          ///< The cumulative number of video packets received on the client.
    uint32_t totalPacketsLost;              ///< The cumulative number of video packets lost in transit from server to client.
    uint32_t totalPacketsDropped;           ///< The cumulative number of video packets dropped without getting displayed on the client.

    cxrConnectionQuality quality;           ///< The quality of the connection as one of the five values in @ref cxrConnectionQuality.
                                            ///< @note If the quality is @ref cxrConnectionQuality_Fair the qualityReasons flags will be populated.
    uint32_t qualityReasons;                ///< The reasons for connection quality as one or more @ref cxrConnectionQualityReason flags.
} cxrConnectionStats;

typedef struct cxrConnectionDesc
{
    cxrBool async;                          ///< Calls to @ref cxrConnect will spawn a background thread to attempt the connection and return control.
                                            ///< to calling thread immediately without blocking until a connection is established or fails.
    cxrBool useL4S;                         ///< On networks where available, enable L4S congestion handling.
    cxrNetworkInterface clientNetwork;      ///< Network adapter type used by the client to connect to the server.
    cxrNetworkTopology topology;            ///< Topology of the connecton between Client and Server.
} cxrConnectionDesc;

/** @} */ // end of groupStruct

/** @defgroup groupFunc CloudXR Client API Functions
    @{
 */


/**
    @brief Initialize the CloudXR client and create a Receiver handle to be used by all other API calls.

    @param[in] description      A filled-out client receiver descriptor struct (see @ref cxrReceiverDesc).
    @param[out] receiver        A pointer to return back to the client an opaque handle representing the Receiver interface.

    @note                       The returned @ref cxrReceiverHandle must be passed as the first parameter to all other CloudXR client API calls.

    @retval cxrError_Required_Parameter         One of the required parameters was missing
    @retval cxrError_Invalid_API_Version        The requested version of the client library was invalid
    @retval cxrError_Invalid_Number_Of_Streams      The requested number of streams was invalid for the requested mode
    @retval cxrError_Invalid_Device_Descriptor  The provided device descriptor is invalid
    @retval cxrError_Invalid_Graphics_Context   The graphics context is not valid for the OS/platform
    @retval cxrError_Decoder_Setup_Failed       The platform specific decoder could not be instantiated
*/
CLOUDXR_PUBLIC_API cxrError cxrCreateReceiver(const cxrReceiverDesc* description, cxrReceiverHandle* receiver);

/**
    @brief Establish a connection to a CloudXR server.

    @param[in] receiver          cxrReceiverHandle handle that was obtained from cxrCreateReceiver().
    @param[in] serverAddr        IP address of the CloudXR server.
    @param[in] description       Optional settings that control connection behavior (see @ref cxrConnectionDesc).

    @retval cxrError_Required_Parameter         One of the required parameters was missing
    @retval cxrError_Not_Connected              The receiver has initiated an asynchronous connection which has not completed yet
    @retval cxrError_Module_Load_Failed         The streamer library could not be loaded
    @retval cxrError_Client_Setup_Failed        The streamer library could not be initialized
    @retval cxrError_Client_Version_Old         The client version is too old for the server
    @retval cxrError_Client_Unauthorized        The client authorization header was rejected
    @retval cxrError_Server_Handshake_Failed    The client could not complete the handshake with the server (This could be due to an incorrect address, a blocked RTSP port or a temporary network interruption)
    @retval cxrError_Server_Version_Old         The server version is too old for the client
    @retval cxrError_Server_Feature_Disabled_*  The server disabled the requested feature (HEVC, VVSync, Pose Prediction, Send Audio or Receive Audio)
 */
CLOUDXR_PUBLIC_API cxrError cxrConnect(cxrReceiverHandle receiver, const char* serverAddr, cxrConnectionDesc* description);

/**
    @brief Terminate a streaming session and free the associated CloudXR resources.

    @param[in] receiver   cxrReceiverHandle handle from cxrCreateReceiver()

    @note The Receiver handle is invalid upon return from this call.
*/
CLOUDXR_PUBLIC_API void cxrDestroyReceiver(cxrReceiverHandle receiver);

/**
    @brief Acquire the next available decoded video frames from network stream

    This call attempts to acquire the next available decoded video frames for all streams
     in `streamMask`, together in lock-step.  It can be called repeatedly with different
     masks in order to acquire frames without concurrent locking.

    @param[in] receiver   cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in,out]        framesLatched  pointer to a cxrFramesLatched struct with a lifetime until cxrReleaseFrame.
    @param[in] frameMask  a bitfield mask set to 1<<[frame number] for generic frames, or use special values like @ref cxrFrameMask_All.
    @param[in] timeoutMs  number of milliseconds to wait for frame(s) before returning in error.

    @retval cxrError_Required_Parameter         One of the required parameters was missing
    @retval cxrError_Not_Connected              The receiver has not connected yet
    @retval cxrError_Not_Streaming              The server is not streaming yet
    @retval cxrError_Frames_Not_Released        There are previously latched frames that have not yet been released
    @retval cxrError_Frames_Not_Ready           A frame could not be latched within the requested timeout
    @retval cxrError_Frame_Not_Latched          There was a platform specific decoder issue
    @retval cxrError_Decoder_No_Texture         The decoder could not acquire a texture to decode into
    @retval cxrError_Decoder_Frame_Not_Ready    The decoded frame was not ready to be latched
*/
CLOUDXR_PUBLIC_API cxrError cxrLatchFrame(cxrReceiverHandle receiver, cxrFramesLatched* framesLatched, uint32_t frameMask, uint32_t timeoutMs);

#if defined(ANDROID)
/**
    @brief `ANDROID-ONLY` Render a masked set of the latched video frame(s) to the currently bound target surface.

    @param[in] receiver   cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in,out]        framesLatched  pointer to a cxrFramesLatched struct with a lifetime until cxrReleaseFrame.
    @param[in] frameMask  a bitfield mask set to 1<<[frame number] for generic frames, or use special values like @ref cxrFrameMask_Mono_With_Alpha.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Invalid_Number_Of_Streams  The frames latched count or the frame mask contained invalid values
*/
CLOUDXR_PUBLIC_API cxrError cxrBlitFrame(cxrReceiverHandle receiver, cxrFramesLatched* framesLatched, uint32_t frameMask);
#endif

/**
    @brief Release a previously latched set of video frames from cxrLatchFrame.

    @param[in] receiver   cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in,out]        framesLatched  pointer to a cxrFramesLatched struct that was filled by cxrLatchFrame.

    @note The cxrFramesLatched data is invalid upon return from this call.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Frames_Not_Latched     There are no latched frames to release
*/
CLOUDXR_PUBLIC_API cxrError cxrReleaseFrame(cxrReceiverHandle receiver, cxrFramesLatched* framesLatched);

/**
    @brief Adds a controller.

    @param[in] receiver         cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in,out] outHandle    pointer to a cxrControllerHandle that receives a handle for the newly added controller.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Controller_Id_In_Use   The id field in controller description has already been used for another controller
    @retval cxrError_Role_Too_Long          Description's role is longer than CXR_MAX_CONTROLLER_ROLE
    @retval cxrError_Name_Too_Long          Description's name is longer than CXR_MAX_CONTROLLER_NAME, or an input path is longer than CXR_MAX_INPUT_PATH_LENGTH
    @retval cxrError_Too_Many_Inputs        Description's input count is more than CXR_MAX_CONTROLLER_INPUT_COUNT
*/
CLOUDXR_PUBLIC_API cxrError cxrAddController(cxrReceiverHandle receiver, const cxrControllerDesc* desc, cxrControllerHandle* outHandle);

/**
    @brief Sends a number of poses for one or more controllers.

    @param[in] receiver             cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in] poseCount            number of poses to send.
    @param[in] controllerHandles    array containing poseCount controller handles.
    @param[in] states               array containing poseCount controller tracking states.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
*/
CLOUDXR_PUBLIC_API cxrError cxrSendControllerPoses(cxrReceiverHandle receiver, uint32_t poseCount, const cxrControllerHandle* controllerHandles, const cxrControllerTrackingState* const* states);

/**
    @brief Sends events for a controller.

    @param[in] receiver     cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in] controller   handle of the controller.
    @param[in] events       array containing eventCount controller events.
    @param[in] eventCount   number of events to send.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Data_Too_Large         A blob value in an action excceds CXR_MAX_BLOB_BYTES_PER_INPUT bytes
*/
CLOUDXR_PUBLIC_API cxrError cxrFireControllerEvents(cxrReceiverHandle receiver, cxrControllerHandle controller, const cxrControllerEvent* events, uint32_t eventCount);

/**
    @brief Removes a controller.

    @param[in] receiver     cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in] handle       handle of the controller to remove.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
*/
CLOUDXR_PUBLIC_API cxrError cxrRemoveController(cxrReceiverHandle receiver, cxrControllerHandle handle);

/**
    @brief Provide estimated world lighting properties to the server.

    This is used to send ARCore or ARKit lighting estimates to the server, for live integration into
     3D rendered scene lighting.  It supports a primary light source as well as ambient spherical harmonics.
     On the server side, apps can query this data using:

            IVRSystem::GetArrayTrackedDeviceProperty(cxrTrackedDeviceProperty)

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Not_Connected          The receiver has not connected yet
*/
CLOUDXR_PUBLIC_API cxrError cxrSendLightProperties(cxrReceiverHandle receiver, const cxrLightProperties* lightProps);

/**
    @brief Send non-VR-controller input events to the server for app-specific handling.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Not_Connected          The receiver has not connected yet
*/
CLOUDXR_PUBLIC_API cxrError cxrSendInputEvent(cxrReceiverHandle receiver, const cxrInputEvent* inputEvent);

/**
    @brief Add an event to the trace timeline.
 
    This function allows for the insertion of custom events into the trace timeline of CloudXR. Note that to log
     an event this function must be called twice.
 
     Example of logging an event:
         void foo()
         {
            cxrTraceEvent("ButtonPressed", 0x23, cxrTrue);
            ... do some work here ...
            cxrTraceEvent("ButtonPressed", 0x23, cxrFalse);
         }

    @param[in] name      name of the event to add to the timeline
    @param[in] eventId   a custom Id value that can be used to identify the event in the timeline
    @param[in] begin     true indicates the beginning of an event, false indicates the end of the event.
*/
CLOUDXR_PUBLIC_API void cxrTraceEvent(char* name, uint32_t eventId, cxrBool begin);

/**
    @brief Send client input audio (i.e. microphone) to the server.

    @param[in] receiver        cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in] audioFrame      A filled-out struct containing the audio data to send (see @ref cxrAudioFrame).

    @note To use this API set the sendAudio boolean (see @ref cxrDeviceDesc).

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Not_Connected          The receiver has not connected yet
    @retval cxrError_Data_Too_Large         The audio frame data was larger than the max size of 64kb
*/
CLOUDXR_PUBLIC_API cxrError cxrSendAudio(cxrReceiverHandle receiver, const cxrAudioFrame* audioFrame);

/**
	@brief Set an authorization header that is validated by the server.

    This api allows a client to specify credentials via a header that is exchanged prior
     to the connection to the server over HTTPS. If used, this API must be called after
     cxrCreateReceiver and before cxrConnect.

    @param[in] receiver        cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in] header          The value of the authorization header to send the server during cxrConnect().
 
    @note In order to send credentials the client OS must trust the server certificate.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
*/
CLOUDXR_PUBLIC_API cxrError cxrSetAuthorizationHeader(cxrReceiverHandle receiver, const char* header);

/**
    @brief Explicitly push XR tracking state.

    @param[in] receiver        cxrReceiverHandle handle from cxrCreateReceiver()
    @param[in] trackingState   A filled-out struct containing the tracking state to send (see @ref cxrVRTrackingState).

    @retval cxrError_Required_Parameter         One of the required parameters was missing
    @retval cxrError_Not_Connected              The receiver has not connected yet
    @retval cxrError_Pose_Callback_Provided     The receiver was configured to poll for poses via callback rather than have the client send them
*/

CLOUDXR_PUBLIC_API cxrError cxrSendPose(cxrReceiverHandle receiver, const cxrVRTrackingState* trackingState);

/**
    @brief Get Connection statistics during the lifetime of the session.

    @param[in] receiver        cxrReceiverHandle handle from cxrCreateReceiver()
    @param[out] stats          A struct to fill-in with the current connection statistics (see @ref cxrConnectionStats).

    @note For best results it is reccommended to poll this API no more than once per second.

    @retval cxrError_Required_Parameter     One of the required parameters was missing
    @retval cxrError_Not_Connected          The receiver has not connected yet
*/

CLOUDXR_PUBLIC_API cxrError cxrGetConnectionStats(cxrReceiverHandle receiver, cxrConnectionStats* stats);

/** @} */ // end of groupFunc

#ifdef __cplusplus
}
#endif

#endif
