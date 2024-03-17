/*
 * Copyright (c) 2019-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "main.h"

#include <android/window.h>
#include <android/native_window_jni.h>
#include <android_native_app_glue.h>
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON

#include <linux/prctl.h>
#include <sys/prctl.h>
#include <sys/system_properties.h>

#include "CloudXRClientOptions.h"
#include "CloudXRMatrixHelpers.h"

#define LOG_TAG "OVR Client"
#include "CloudXRLog.h"

#include "CloudXRFileLogger.h"

#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <map>
#include <vector>

static struct android_app* GAndroidApp = NULL;
static CloudXR::ClientOptions GOptions;
static std::mutex GJniMutex;
static OKCloudStreamer *gClientHandle = NULL;

// TODO: these values are heavily dependent on app workload.
static const int CPU_LEVEL = 1;
static const int GPU_LEVEL = 1;

// macro to switch between CloudXRFileLogger vs just direct android_log_print.
#define LOG_TO_FILE 1

//-----------------------------------------------------------------------------
cxrClientCallbacks OKCloudStreamer::s_clientProxy = { 0 };
extern "C" void dispatchLogMsg(cxrLogLevel level, cxrMessageCategory category, void *extra, const char *tag, const char *fmt, ...)
{
    va_list aptr;
    va_start(aptr, fmt);
#if !LOG_TO_FILE
    const int bufsize = 8192;
    char buffer[bufsize];
    vsnprintf(buffer, bufsize, fmt, aptr);
    // throw to logcat.
    __android_log_print(cxrLLToAndroidPriority(level), tag, "%s", buffer);
#else
    // throw to the log file
    g_logFile.logva(level, tag, fmt, aptr);
#endif
    va_end(aptr);
}


static double GetTimeInSeconds() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

static uint64_t GetTimeInNS() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return ((uint64_t)(now.tv_sec * 1e9) + now.tv_nsec);
}

#define CASE(x) \
case x:     \
return #x

const char* ClientStateEnumToString(cxrClientState state)
{
    switch (state)
    {
        CASE(cxrClientState_ReadyToConnect);
        CASE(cxrClientState_ConnectionAttemptInProgress);
        CASE(cxrClientState_ConnectionAttemptFailed);
        CASE(cxrClientState_StreamingSessionInProgress);
        CASE(cxrClientState_Disconnected);
        CASE(cxrClientState_Exiting);
        default:
            return "";
    }
}
#undef CASE

//==============================================================
//
//==============================================================
void android_handle_cmd(struct android_app* app, int32_t cmd) {
    OKCloudStreamer* cxrc = (OKCloudStreamer*)app->userData;
    if (cxrc == nullptr) {
        // TODO: shouldn't hit this case, but if we do we need to likely
        //  log and exit. TBD.
        CXR_LOGE("android_handle_cmd called with null userData");
        return;
    }

    switch (cmd) {
        // TODO: handle gained/lost focus events
        case APP_CMD_START:
            CXR_LOGI("APP_CMD_START");
            break;
        case APP_CMD_RESUME:
            CXR_LOGI("APP_CMD_RESUME");
            cxrc->SetPaused(false);
            break;
        case APP_CMD_PAUSE:
            CXR_LOGI("APP_CMD_PAUSE");
            cxrc->SetPaused(true);
            break;
        case APP_CMD_STOP:
            // TODO - may need to handle this
            CXR_LOGI("APP_CMD_STOP");
            break;
        case APP_CMD_DESTROY:
            // TODO - may need to do more here
            CXR_LOGI("APP_CMD_DESTROY");
            cxrc->SetWindow(nullptr);
            break;
        case APP_CMD_INIT_WINDOW:
            CXR_LOGI("APP_CMD_INIT_WINDOW");
            cxrc->SetWindow(app->window);
            break;
        case APP_CMD_TERM_WINDOW:
            CXR_LOGI("APP_TERM_WINDOW");
            cxrc->SetWindow(nullptr);
            break;
    }
}

//==============================================================
//
//==============================================================
int32_t android_handle_input(struct android_app* app, AInputEvent* event) {
    OKCloudStreamer* cxrc = (OKCloudStreamer*)app->userData;
    if (app == nullptr) {
        CXR_LOGE("android_handle_input called with null userData");
        return 0;
    }

    // if needed, handle android keyboard/etc events here.
    // HMD+controller already handled via VrApi calls.

    return 0;
}

namespace
{

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static inline cxrMatrix34 cxrConvert(const ovrMatrix4f& m)
{
    cxrMatrix34 out{};
    // The matrices are compatible so doing a memcpy() here
    //  noting that we are a [3][4] and ovr uses [4][4]
    memcpy(&out, &m, sizeof(out));
    return out;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static inline cxrVector3 cxrConvert(const ovrVector3f& v)
{
    return {{v.x, v.y, v.z}};
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static ovrQuatf cxrToQuaternion(const cxrMatrix34& m)
{
    ovrQuatf q;
    const float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];

    if (trace > 0.f)
    {
        float s = 0.5f / sqrtf(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (m.m[2][1] - m.m[1][2]) * s;
        q.y = (m.m[0][2] - m.m[2][0]) * s;
        q.z = (m.m[1][0] - m.m[0][1]) * s;
    }
    else
    {
        if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2])
        {
            float s = 2.0f * sqrtf(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]);
            q.w = (m.m[2][1] - m.m[1][2]) / s;
            q.x = 0.25f * s;
            q.y = (m.m[0][1] + m.m[1][0]) / s;
            q.z = (m.m[0][2] + m.m[2][0]) / s;
        }
        else if (m.m[1][1] > m.m[2][2])
        {
            float s = 2.0f * sqrtf(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]);
            q.w = (m.m[0][2] - m.m[2][0]) / s;
            q.x = (m.m[0][1] + m.m[1][0]) / s;
            q.y = 0.25f * s;
            q.z = (m.m[1][2] + m.m[2][1]) / s;
        }
        else
        {
            float s = 2.0f * sqrtf(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]);
            q.w = (m.m[1][0] - m.m[0][1]) / s;
            q.x = (m.m[0][2] + m.m[2][0]) / s;
            q.y = (m.m[1][2] + m.m[2][1]) / s;
            q.z = 0.25f * s;
        }
    }

    return q;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static ovrVector3f cxrGetTranslation(const cxrMatrix34& m)
{
    return {m.m[0][3], m.m[1][3], m.m[2][3]};
}

}  // namespace


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
OKCloudStreamer::OKCloudStreamer(struct android_app* app)
{
    mRenderState = RenderState_Loading;
    mHeadsetOnHead = true; // assume it is until we detect otherwise.

    // clear things we need to clear...
    mNativeWindow = NULL;
    mOvrSession = NULL;
    mFrameCounter = 0;

    // set paused state.
    mIsPaused = mWasPaused = true;

    mAndroidApp = app; // cache and hang onto it.
    // we want the EXTERNAL path, as that maps to /sdcard, and is what shows on PC.
    mAppBasePath = mAndroidApp->activity->externalDataPath;
    mAppOutputPath = mAppBasePath + "/logs/";
    CXR_LOGI("Android external data path is %s", mAppBasePath.c_str());

    mJavaCtx.Vm = app->activity->vm;
    mJavaCtx.Vm->AttachCurrentThread(&mJavaCtx.Env, NULL);
    mJavaCtx.ActivityObject = app->activity->clazz;

    // AttachCurrentThread reset the thread name, set it to something meaningful here.
    prctl(PR_SET_NAME, (long)"OKCloudStreamer", 0, 0, 0);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrError OKCloudStreamer::Initialize()
{
    CXR_LOGV("OKCloudStreamer::Initialize START");

    int32_t result = VRAPI_INITIALIZE_SUCCESS;
    ovrInitParms localInitParms = vrapi_DefaultInitParms(&mJavaCtx);
    CXR_LOGV("Initialize VrApi");
    result = vrapi_Initialize(&localInitParms);
    if (result != VRAPI_INITIALIZE_SUCCESS)
    {
        CXR_LOGE("Init - failed to Initialize the VRAPI localInitParms=%p", &localInitParms);
        return cxrError_Module_Load_Failed;
    }

    CXR_LOGV("Initialize EGL");
    if (!mEglHelper.Initialize())
    {
        CXR_LOGE("Init - failed to initialize EglHelper");
        return cxrError_Failed;
    }

    mAndroidApp->userData = this;
    mAndroidApp->onAppCmd = android_handle_cmd;
    mAndroidApp->onInputEvent = android_handle_input;

    // demonstrate logging some device specific data...
    char propStr[PROP_VALUE_MAX];
    int proplen;
    proplen = __system_property_get("ro.ovr.os.api.version", propStr);
    CXR_LOGI("OVR API version is %s", propStr);

    CXR_LOGV("Initialize END");

    return cxrError_Success;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrError OKCloudStreamer::Release()
{
    mEglHelper.Release();

    // if we somehow still have session, release it now so we don't block display.
    if (mOvrSession != NULL)
    {
        CXR_LOGI("CALLING vrapi_LeaveVrMode()");
        vrapi_LeaveVrMode(mOvrSession);
        mOvrSession = NULL;
    }
    CXR_LOGI("CALLING vrapi_Shutdown()");
    vrapi_Shutdown();

    mJavaCtx.Vm->DetachCurrentThread();
    mJavaCtx.Vm = NULL;
    mJavaCtx.Env = NULL;
    mJavaCtx.ActivityObject = NULL;

    return cxrError_Success;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::RequestExit()
{
    CXR_LOGI("Requesting application exit.");
    mClientState = cxrClientState_Exiting;
    mRenderState = RenderState_Exiting;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::UpdateClientState()
{
    if (mClientState==cxrClientState_Exiting)
        return; // early return if we're ALREADY in exiting state.

    if (mConnectionDesc.async)
    {
        // all logging done in the callback, just do 'reactions' here.
        switch (mClientState)
        {
            case cxrClientState_ConnectionAttemptInProgress:
            { // status indication via log
                static int32_t attemptCount = 0;
                if (++attemptCount % 60 == 0) CXR_LOGI("Waiting for server...");
                break;
            }

            case cxrClientState_StreamingSessionInProgress:
                mRenderState = RenderState_Running;
                break;

            case cxrClientState_ConnectionAttemptFailed:
            case cxrClientState_Disconnected:
                CXR_LOGE("Exiting due to connection failure.");
                // fall through to below common handling...
                break;

            default:
                break;
        }
    }

    // handle client error state in one place so we don't duplicate this code.
    if (mClientState==cxrClientState_Disconnected ||
        mClientState==cxrClientState_ConnectionAttemptFailed)
    {
        RequestExit();
    }
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrError OKCloudStreamer::MainLoop()
{
    const double startTime = GetTimeInSeconds();

    while (mAndroidApp->destroyRequested == 0 && mClientState != cxrClientState_Exiting) {
        // Read all pending events.
        for (;;) {
            int events;
            struct android_poll_source* source;
            const int timeoutMilliseconds = (mOvrSession == NULL || !mIsFocused) ? 250 : 1;
            if (ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void**)&source) < 0)
                break;

            // Process this event.
            if (source != NULL)
                source->process(mAndroidApp, source);

        }

        // check and update client state changes from callback
        UpdateClientState();

        // we check state and handle vr enter/leave changes
        HandleVrModeChanges();

        // We must read from the event queue with regular frequency.
        HandleVrApiEvents();

        // if not yet entered vr mode, just continue as not yet set up to render...
        if (mOvrSession == NULL) continue;

        // TODO: is this check now implicitly handled in client state changes?
        //if (Receiver && !cxrIsRunning(Receiver) && mRenderState != RenderState_Exiting)
        //    mRenderState = RenderState_Exiting;

        if (mRenderState == RenderState_Loading)
            RenderLoadScreen();
        else if (mRenderState == RenderState_Exiting)
            RenderExitScreen();
        else
            Render();
    }

    return cxrError_Success;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrError OKCloudStreamer::CreateReceiver()
{
    if (Receiver)
        return cxrError_Success;

    if (GOptions.mServerIP.empty())
    {
        CXR_LOGE("No CloudXR server address specified.");
        return cxrError_Required_Parameter;
    }

    if (mOvrSession == nullptr)
    {
        CXR_LOGE("OVR session is null, cannot continue.");
        return cxrError_Failed; // false..
    }

    if (mDeviceDesc.receiveAudio)
    {
        // Initialize audio playback
        oboe::AudioStreamBuilder playbackStreamBuilder;
        playbackStreamBuilder.setDirection(oboe::Direction::Output);
        playbackStreamBuilder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        playbackStreamBuilder.setSharingMode(oboe::SharingMode::Exclusive);
        playbackStreamBuilder.setFormat(oboe::AudioFormat::I16);
        playbackStreamBuilder.setChannelCount(oboe::ChannelCount::Stereo);
        playbackStreamBuilder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);

        oboe::Result r = playbackStreamBuilder.openStream(playbackStream);
        if (r != oboe::Result::OK) {
            CXR_LOGE("Failed to open playback stream. Error: %s", oboe::convertToText(r));
            return cxrError_Failed;
        }

        int bufferSizeFrames = playbackStream->getFramesPerBurst() * 2;
        r = playbackStream->setBufferSizeInFrames(bufferSizeFrames);
        if (r != oboe::Result::OK) {
            CXR_LOGE("Failed to set playback stream buffer size to: %d. Error: %s",
                    bufferSizeFrames, oboe::convertToText(r));
            return cxrError_Failed;
        }

        r = playbackStream->start();
        if (r != oboe::Result::OK) {
            CXR_LOGE("Failed to start playback stream. Error: %s", oboe::convertToText(r));
            return cxrError_Failed;
        }
    }

    if (mDeviceDesc.sendAudio)
    {
        // Initialize audio recording
        oboe::AudioStreamBuilder recordingStreamBuilder;
        recordingStreamBuilder.setDirection(oboe::Direction::Input);
        recordingStreamBuilder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        recordingStreamBuilder.setSharingMode(oboe::SharingMode::Exclusive);
        recordingStreamBuilder.setFormat(oboe::AudioFormat::I16);
        recordingStreamBuilder.setChannelCount(oboe::ChannelCount::Stereo);
        recordingStreamBuilder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);
        recordingStreamBuilder.setInputPreset(oboe::InputPreset::VoiceCommunication);
        recordingStreamBuilder.setDataCallback(this);

        oboe::Result r = recordingStreamBuilder.openStream(recordingStream);
        if (r != oboe::Result::OK) {
            CXR_LOGE("Failed to open recording stream. Error: %s", oboe::convertToText(r));
            return cxrError_Failed;
        }

        r = recordingStream->start();
        if (r != oboe::Result::OK) {
            CXR_LOGE("Failed to start recording stream. Error: %s", oboe::convertToText(r));
            return cxrError_Failed;
        }
    }

    CXR_LOGI("Trying to create Receiver at %s.", GOptions.mServerIP.c_str());
    cxrGraphicsContext context{cxrGraphicsContext_GLES};
    context.egl.display = eglGetCurrentDisplay();
    context.egl.context = eglGetCurrentContext();

    if(context.egl.context == nullptr)
    {
        CXR_LOGE("Error, null EGL graphics context");
    }

    s_clientProxy.GetTrackingState = [](void* context, cxrVRTrackingState* trackingState)
    {
        return reinterpret_cast<OKCloudStreamer*>(context)->GetTrackingState(trackingState);
    };
    s_clientProxy.TriggerHaptic = [](void* context, const cxrHapticFeedback* haptic)
    {
        return reinterpret_cast<OKCloudStreamer*>(context)->TriggerHaptic(haptic);
    };
    s_clientProxy.RenderAudio = [](void* context, const cxrAudioFrame *audioFrame)
    {
        return reinterpret_cast<OKCloudStreamer*>(context)->RenderAudio(audioFrame);
    };

    // the client_lib calls into here when the async connection status changes
    s_clientProxy.UpdateClientState = [](void* context, cxrClientState state, cxrError error)
    {
        switch (state)
        {
            case cxrClientState_ConnectionAttemptInProgress:
                CXR_LOGI("Connection attempt in progress.");
                break;
            case cxrClientState_StreamingSessionInProgress:
                CXR_LOGI("Connection attempt succeeded.");
                break;
            case cxrClientState_ConnectionAttemptFailed:
                CXR_LOGE("Connection attempt failed with error: %s", cxrErrorString(error));
                break;
            case cxrClientState_Disconnected:
                CXR_LOGE("Server disconnected with error: %s", cxrErrorString(error));
                break;
            default:
                CXR_LOGI("Client state updated: %s, error: %s", ClientStateEnumToString(state), cxrErrorString(error));
                break;
        }

        // update the state of the app, don't perform any actions here
        // the client state change will be handled in the render thread (UpdateClientState())
        OKCloudStreamer *client = reinterpret_cast<OKCloudStreamer*>(context);
        client->mClientState = state;
        client->mClientError = error;
    };

    s_clientProxy.LogMessage = [](void* context, cxrLogLevel level, cxrMessageCategory category, void* extra, const char* tag, const char* const messageText)
    {
        // Here we call our helper fn to output same way as the log macros will.
        // note that at the moment, we don't need/use the client context.
        dispatchLogMsg(level, category, extra, tag, messageText);
    };

    // context is now IN the callback struct.
    s_clientProxy.clientContext = this;

    cxrReceiverDesc desc = { 0 };
    desc.requestedVersion = CLOUDXR_VERSION_DWORD;
    desc.deviceDesc = mDeviceDesc;
    desc.clientCallbacks = s_clientProxy;
    desc.shareContext = &context;
    desc.debugFlags = cxrDebugFlags_EnableAImageReaderDecoder;//GOptions.mDebugFlags;
    desc.logMaxSizeKB = CLOUDXR_LOG_MAX_DEFAULT;
    desc.logMaxAgeDays = CLOUDXR_LOG_MAX_DEFAULT;
    strncpy(desc.appOutputPath, mAppOutputPath.c_str(), CXR_MAX_PATH - 1);
    desc.appOutputPath[CXR_MAX_PATH-1] = 0; // ensure null terminated if string was too long.

    cxrError err = cxrCreateReceiver(&desc, &Receiver);
    if (err != cxrError_Success)
    {
        CXR_LOGE("Failed to create CloudXR receiver. Error %d, %s.", err, cxrErrorString(err));
        return err;
    }

    // else, good to go.
    CXR_LOGI("Receiver created!");

    mConnectionDesc.async = cxrTrue;
    mConnectionDesc.useL4S = GOptions.mUseL4S;
    mConnectionDesc.clientNetwork = GOptions.mClientNetwork;
    mConnectionDesc.topology = GOptions.mTopology;
    err = cxrConnect(Receiver, GOptions.mServerIP.c_str(), &mConnectionDesc);
    if (!mConnectionDesc.async)
    {
        if (err != cxrError_Success)
        {
            CXR_LOGE("Failed to connect to CloudXR server at %s. Error %d, %s.",
                  GOptions.mServerIP.c_str(), (int)err, cxrErrorString(err));
            TeardownReceiver();
            return err;
        }
        else {
            mClientState = cxrClientState_StreamingSessionInProgress;
            mRenderState = RenderState_Running;
            CXR_LOGI("Receiver created for server: %s", GOptions.mServerIP.c_str());
        }
    }

    return cxrError_Success; //true
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::TeardownReceiver() {
    if (playbackStream)
    {
        playbackStream->close();
    }

    if (recordingStream)
    {
        recordingStream->close();
    }

    if (Receiver) {
        cxrDestroyReceiver(Receiver);
    }

    Receiver = nullptr;
    s_clientProxy = {0};
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrError OKCloudStreamer::QueryChaperone(cxrDeviceDesc* deviceDesc) const
{
    if (mOvrSession == nullptr)
    {
        CXR_LOGE("OVR session is null, cannot continue.");
        return cxrError_Failed; // false..
    }

    // Set arena boundary
    ovrPosef pose = {0};
    ovrVector3f scale = {0};
    ovrResult result;
    // This call fails (returns '1' not '0') when guardian is in stationary mode...
    result = vrapi_GetBoundaryOrientedBoundingBox(mOvrSession, &pose, &scale);
    if (result == ovrSuccess)
    {
        // we fall through to chaperone init/send below.
    }
    else
    {
        CXR_LOGI("Cannot get play bounds, creating default.");
        // should clear pose in case we use in the future.
        // for now, we fill in scale with fake/default values.
        scale.x = scale.z = 1.5f * 0.5f; // use 1.5m for now -- oculus stationary bounds are tight.
    }

    // cxrChaperone chap;
    deviceDesc->chaperone.universe = cxrUniverseOrigin_Standing;
    deviceDesc->chaperone.origin.m[0][0] = deviceDesc->chaperone.origin.m[1][1] = deviceDesc->chaperone.origin.m[2][2] = 1;
    deviceDesc->chaperone.origin.m[0][1] = deviceDesc->chaperone.origin.m[0][2] = deviceDesc->chaperone.origin.m[0][3] = 0;
    deviceDesc->chaperone.origin.m[1][0] = deviceDesc->chaperone.origin.m[1][2] = deviceDesc->chaperone.origin.m[1][3] = 0;
    deviceDesc->chaperone.origin.m[2][0] = deviceDesc->chaperone.origin.m[2][1] = deviceDesc->chaperone.origin.m[2][3] = 0;
    deviceDesc->chaperone.playArea.v[0] = 2.f * scale.x;
    deviceDesc->chaperone.playArea.v[1] = 2.f * scale.z;
    CXR_LOGI("Setting play area to %0.2f x %0.2f", deviceDesc->chaperone.playArea.v[0], deviceDesc->chaperone.playArea.v[1]);

    return cxrError_Success;
}

//-----------------------------------------------------------------------------
// Note: here we try to detect controllers up-front.  We may need to do this
// post-connect, if we don't detect any, or don't detect two.  Also note that
// this code has removed all support for non Touch controllers and old devices.
//-----------------------------------------------------------------------------
void OKCloudStreamer::DetectControllers()
{
    // determine class of Oculus device
    mControllersFound = 0;
    uint32_t deviceIndex = 0;
    ovrInputCapabilityHeader capsHeader;
    ovrResult result;

    while (true)
    {
        result = vrapi_EnumerateInputDevices(mOvrSession, deviceIndex, &capsHeader);
        if (result != ovrSuccess)
        {
            CXR_LOGE("Failed to enumerate device %d, error = %d", deviceIndex, result);
            break; // we're done if this call fails.
        }
        else
        {
            CXR_LOGE("Found device %d, type = 0x%0x", deviceIndex, capsHeader.Type);
        }
        ++deviceIndex;
        if (capsHeader.Type == ovrControllerType_TrackedRemote)
        {
            ovrInputTrackedRemoteCapabilities remoteCaps;
            remoteCaps.Header = capsHeader;
            vrapi_GetInputDeviceCapabilities(mOvrSession, &remoteCaps.Header);
            if (remoteCaps.ControllerCapabilities & ovrControllerCaps_ModelOculusTouch)
            {
                mControllersFound++;
                // TODO do we set up our internal state tracking of controllers here???
            }
        }
    }

    if (0==mControllersFound)
    {
        CXR_LOGE("No controllers identified!");
    }
    else
    {
        CXR_LOGI("Found %d controllers", mControllersFound);
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool OKCloudStreamer::SetupFramebuffer(GLuint colorTexture, uint32_t eye)
{
    if(Framebuffers[eye] == 0)
    {
        GLuint framebuffer;

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, colorTexture, 0);

        GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            CXR_LOGI("Incomplete frame buffer object!");
            return false;
        }

        Framebuffers[eye] = framebuffer;

        CXR_LOGI("Created FBO %d for eye%d texture %d.",
            framebuffer, eye, colorTexture);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Framebuffers[eye]);
    }
    else
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Framebuffers[eye]);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, colorTexture, 0);
    }

    glViewport(0, 0, EyeWidth[eye], EyeHeight[eye]);

    return true;
}

//-----------------------------------------------------------------------------
void OKCloudStreamer::FillBackground()
{
    float cr = ((mBGColor & 0x00FF0000) >> 16) / 255.0f;
    float cg = ((mBGColor & 0x0000FF00) >> 8) / 255.0f;
    float cb = ((mBGColor & 0x000000FF)) / 255.0f;
    float ca = ((mBGColor & 0xFF000000) >> 24) / 255.0f;
    glClearColor(cr, cg, cb, ca);
    glClear(GL_COLOR_BUFFER_BIT);
}


//-----------------------------------------------------------------------------
static constexpr int inputCountQuest = 21;

static const char* inputPathsQuest[inputCountQuest] =
{
    "/input/system/click",
    "/input/application_menu/click", // this is carried over from old system and might be remove, it's not a button binding, more action.
    "/input/trigger/click",
    "/input/trigger/touch",
    "/input/trigger/value",
    "/input/grip/click",
    "/input/grip/touch",
    "/input/grip/value",
    "/input/joystick/click",
    "/input/joystick/touch",
    "/input/joystick/x",
    "/input/joystick/y",
    "/input/a/click",
    "/input/b/click",
    "/input/x/click", // Touch has X/Y on L controller, so we'll map the raw strings.
    "/input/y/click",
    "/input/a/touch",
    "/input/b/touch",
    "/input/x/touch",
    "/input/y/touch",
    "/input/thumb_rest/touch",
};

cxrInputValueType inputValueTypesQuest[inputCountQuest] =
{
    cxrInputValueType_boolean, //input/system/click
    cxrInputValueType_boolean, //input/application_menu/click
    cxrInputValueType_boolean, //input/trigger/click
    cxrInputValueType_boolean, //input/trigger/touch
    cxrInputValueType_float32, //input/trigger/value
    cxrInputValueType_boolean, //input/grip/click
    cxrInputValueType_boolean, //input/grip/touch
    cxrInputValueType_float32, //input/grip/value
    cxrInputValueType_boolean, //input/joystick/click
    cxrInputValueType_boolean, //input/joystick/touch
    cxrInputValueType_float32, //input/joystick/x
    cxrInputValueType_float32, //input/joystick/y
    cxrInputValueType_boolean, //input/a/click
    cxrInputValueType_boolean, //input/b/click
    cxrInputValueType_boolean, //input/x/click
    cxrInputValueType_boolean, //input/y/click
    cxrInputValueType_boolean, //input/a/touch
    cxrInputValueType_boolean, //input/b/touch
    cxrInputValueType_boolean, //input/x/touch
    cxrInputValueType_boolean, //input/y/touch
    cxrInputValueType_boolean, //input/thumb_rest/touch
};


// we need a map of ovr button bit shift index (1<<n) to the
// index into client input list, for quick conversions from
// ovr -> cxr events.  Given the OVR Api is deprecated, these
// items are constants, we can precompute the values.
// since there's only 32b, an array[32] is fastest lookup by far.
const int ovrBitsToInput[32] = { // ovr bit index -> client input index
    12, // A
    13, // B
    -1, // not mapped
    -1, // not mapped

    -1, -1, -1, -1, //unused block

    14, // X
    15, // Y
    -1, // not mapped
    -1, // not mapped

    -1, -1, -1, -1, //unused block

    -1, // not mapped // up
    -1, // not mapped // down
    -1, // not mapped // left
    -1, // not mapped // right

    0, // ENTER, left controller menu button, => /input/system/click
    -1, // not mapped
    -1, // not mapped
    -1, // n/a

    -1, // n/a
    -1, // n/a
    5, // grip trigger
    -1, // n/a

    -1, // n/a
    2, // index trigger
    -1, // n/a
    8, // joystick click
};

const int ovrTouchToInput[16] = { // ovr touch bit index -> client input index
    16, // A
    17, // B
    18, // X
    19, // Y

    -1, // not mapped - trackpad
    9, // stick (generic?)
    3, // index trigger
    -1, // n/a

    -1, // thumb up, not near ABXY/Stick
    -1, // index up, far enough from trigger to not be in proximity
    -1, // left joystick
    -1, // right joystick

    -1, // thumb rest (generic?)
    -1, // left thumb rest
    -1, // left thumb rest
    -1, // n/a
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::ProcessControllers(float predictedTimeS)
{
    if (mClientState != cxrClientState_StreamingSessionInProgress)
    {
        // there might be a reason a given app wants to process the controllers regardless.
        // such as if displaying local UI pre-connection.
        // in our case, we're either connected, or we're not, if we're not we need no input.
        return;
    }

    // 64 should be more than large enough. 2x32b masks that are < half used, plus scalars.
    cxrControllerEvent events[MAX_CONTROLLERS][64] = {};
    uint32_t eventCount[MAX_CONTROLLERS] = {};

    uint32_t deviceIndex = 0, controllerIndex = 0;
    ovrInputCapabilityHeader capsHeader;
    while (vrapi_EnumerateInputDevices(mOvrSession, deviceIndex, &capsHeader) >= 0)
    {
        ++deviceIndex;
        if (capsHeader.Type != ovrControllerType_TrackedRemote)
            continue; // quick loop, rather than indenting all the rest of the fn.

        // FIRST check capabilities, detect hand index
        ovrInputTrackedRemoteCapabilities remoteCaps;
        remoteCaps.Header = capsHeader;
        vrapi_GetInputDeviceCapabilities(mOvrSession, &remoteCaps.Header);
        const int32_t handIndex = (remoteCaps.ControllerCapabilities&ovrControllerCaps_RightHand)
                                  ? 1 : 0;

        // for the moment, we're hacking in the controller ADD here, first time we
        // detect controller N available.  It's not a horrible solution, where devices
        // can wake/sleep, and their API doesn't seem to have events/status for that.
        // TODO: ensure we have some ID->index to keep linked when sleep.
        if (!m_newControllers[handIndex]) // null, so open to create+add
        {
            cxrControllerDesc desc = {};
            //desc.id = capsHeader.DeviceID; // turns out this is NOT UNIQUE.  it's a fixed starting number, incremented, and thus devices can 'swap' IDs.
            desc.id = handIndex; // so for now, we're going to just use handIndex, as we're guaranteed left+right will remain 0+1 always.
            desc.role = handIndex?"cxr://input/hand/right":"cxr://input/hand/left";
            desc.controllerName = "Oculus Touch";
            desc.inputCount = inputCountQuest;
            desc.inputPaths = inputPathsQuest;
            desc.inputValueTypes = inputValueTypesQuest;
            CXR_LOGI("Adding controller index %u, ID %llu, role %s", handIndex, desc.id, desc.role);
            CXR_LOGI("Controller caps bits = 0x%08x", capsHeader.DeviceID, remoteCaps.ControllerCapabilities);
            cxrError e = cxrAddController(Receiver, &desc, &m_newControllers[handIndex]);
            if (e!=cxrError_Success)
            {
                CXR_LOGE("Error adding controller: %s", cxrErrorString(e));
                // TODO!!! proper example for client to handle client-call errors, fatal vs 'notice'.
                continue;
            }
        }

        // SECOND handle pose/tracking, to get it out of the way of input events...
        // Must use predicted time or tracking will not be filtered and will jitter/jump
        ovrTracking tracking;
        if (vrapi_GetInputTrackingState(
                mOvrSession, capsHeader.DeviceID, predictedTimeS, &tracking) < 0)
        {
            CXR_LOGE("vrapi_GetInputTrackingState failed, index %u", deviceIndex-1);
            // TODO: maybe mark this as remove controller, or controller-sleep?
            //  may need to review error codes....
            continue;
        }

        auto& controller = TrackingState.controller[handIndex];

        // Rotate the orientation of the controller to match the Quest pose with the Touch SteamVR model
        const float QUEST_TO_TOUCH_ROT = 0.45f; // radians
        controller.pose = ConvertPose(tracking.HeadPose, QUEST_TO_TOUCH_ROT);

        // TODO tracking.status has a bunch of flags to inform active state.
        controller.pose.deviceIsConnected = cxrTrue;
        controller.pose.trackingResult = cxrTrackingResult_Running_OK;
        controller.pose.poseIsValid = cxrTrue;
        // TODO trackingState->hmd.activityLevel = cxrDeviceActivityLevel_UserInteraction;

        // Done with tracking/pose.

        // THIRD, we grab the current input state, and then we'll compare against
        // prior state and generate any events we need to pass to server.
        ovrInputStateTrackedRemote input;
        input.Header.ControllerType = capsHeader.Type;
        if (vrapi_GetCurrentInputState(
                mOvrSession, capsHeader.DeviceID, &input.Header) < 0)
        {
            CXR_LOGE("vrapi_GetCurrentInputState failed, index %u", deviceIndex-1);
            // TODO: maybe mark this as remove controller, or controller-sleep?
            //  may need to review error codes....
            continue;
        }

        const uint64_t inputTimeNS = GetTimeInNS();

        // Let's deal with the scalars up front, since we know what they are.
        // TODO: we could use a compare fn here to filter out tiny changes,
        //  but then we'd need to track value last time we sent events...
        if (input.IndexTrigger != mLastInputState[handIndex].IndexTrigger)
        {
            cxrControllerEvent& e = events[handIndex][eventCount[handIndex]++];
            e.clientTimeNS = inputTimeNS;
            e.clientInputIndex = 4;
            e.inputValue.valueType = cxrInputValueType_float32;
            e.inputValue.vF32 = input.IndexTrigger;
        }

        if (input.GripTrigger != mLastInputState[handIndex].GripTrigger)
        {
            cxrControllerEvent& e = events[handIndex][eventCount[handIndex]++];
            e.clientTimeNS = inputTimeNS;
            e.clientInputIndex = 7;
            e.inputValue.valueType = cxrInputValueType_float32;
            e.inputValue.vF32 = input.GripTrigger;
        }

        if (input.Joystick.x != mLastInputState[handIndex].Joystick.x)
        {
            cxrControllerEvent& e = events[handIndex][eventCount[handIndex]++];
            e.clientTimeNS = inputTimeNS;
            e.clientInputIndex = 10;
            e.inputValue.valueType = cxrInputValueType_float32;
            e.inputValue.vF32 = input.Joystick.x;
        }

        if (input.Joystick.y != mLastInputState[handIndex].Joystick.y)
        {
            cxrControllerEvent& e = events[handIndex][eventCount[handIndex]++];
            e.clientTimeNS = inputTimeNS;
            e.clientInputIndex = 11;
            e.inputValue.valueType = cxrInputValueType_float32;
            e.inputValue.vF32 = input.Joystick.y;
        }

        // okay, now the 'hard' part.  we need to loop through our static arrays
        // of button and touch 'bindings', if value not -1 then test mask against
        // 1<<i (position in array is position in mask...), and if that bit is set,
        // value in array position is client input index.
        // the button mask is 32 bits, the touches mask really is only 16 bits.

        if (input.Buttons != mLastInputState[handIndex].Buttons)
        { // quick check to see if any changes in mask since last time...
            for (uint32_t i = 0; i < 32; i++)
            {
                const int32_t inputId = ovrBitsToInput[i];
                if (inputId < 0) // means we don't bind that input
                    continue;
                // else we do bind that bit.  check current and prior value
                bool bitset = (input.Buttons & (1 << i)) != 0;
                bool oldbit = (mLastInputState[handIndex].Buttons & (1 << i)) != 0;
                if (bitset != oldbit)
                { // value changed, post an event.
                    // prepare an event.
                    //CXR_LOGV("Hand %d Btn %d Path %s", handIndex, i, inputPathsQuest[inputId]);
                    cxrControllerEvent &e = events[handIndex][eventCount[handIndex]++];
                    e.clientTimeNS = inputTimeNS;
                    e.clientInputIndex = inputId;
                    e.inputValue.valueType = cxrInputValueType_boolean;
                    e.inputValue.vBool = bitset ? cxrTrue : cxrFalse;
                }
            }
        }

        // duplicate for touches.  we could make this a function/closure.
        if (input.Touches != mLastInputState[handIndex].Touches)
        { // quick check to see if any changes in mask since last time...
            for (uint32_t i = 0; i < 16; i++) // !!! note change to 16.
            {
                if (ovrTouchToInput[i] < 0) // means we don't bind that touch
                    continue;
                // else we do bind that touch.  check current and prior value
                bool bitset = (input.Touches & (1 << i)) != 0;
                bool oldbit = (mLastInputState[handIndex].Touches & (1 << i)) != 0;
                if (bitset != oldbit)
                { // value changed, post an event.
                    // prepare an event.
                    cxrControllerEvent &e = events[handIndex][eventCount[handIndex]++];
                    e.clientTimeNS = inputTimeNS;
                    e.clientInputIndex = ovrTouchToInput[i];
                    e.inputValue.valueType = cxrInputValueType_boolean;
                    e.inputValue.vBool = bitset ? cxrTrue : cxrFalse;
                }
            }
        }

        if (eventCount[handIndex])
        {
            cxrError err = cxrFireControllerEvents(Receiver, m_newControllers[handIndex], events[handIndex], eventCount[handIndex]);
            if (err != cxrError_Success)
            {
                CXR_LOGE("cxrFireControllerEvents failed: %s", cxrErrorString(err));
                // TODO: how to handle UNUSUAL API errors? might just return up.
                throw("Error firing events"); // just to do something fatal until we can propagate and 'handle' it.
            }
            // save input state for easy comparison next time, ONLY if we sent the events...
            mLastInputState[handIndex] = input;
        }

        // clear event count.
        eventCount[handIndex] = 0;
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrTrackedDevicePose OKCloudStreamer::ConvertPose(
        const ovrRigidBodyPosef& inPose, float rotationX)
{
    ovrMatrix4f transform = vrapi_GetTransformFromPose(&inPose.Pose);

    if(rotationX)
    {
        const ovrMatrix4f rotation = ovrMatrix4f_CreateRotation( rotationX, 0, 0 );
        transform = ovrMatrix4f_Multiply( &transform, &rotation );
    }

    cxrTrackedDevicePose pose{};
    cxrMatrix34 m = cxrConvert(transform);
    cxrMatrixToVecQuat(&m, &pose.position, &pose.rotation);
    pose.velocity = cxrConvert(inPose.LinearVelocity);
    pose.angularVelocity = cxrConvert(inPose.AngularVelocity);
    pose.acceleration = cxrConvert(inPose.LinearAcceleration);
    pose.angularAcceleration = cxrConvert(inPose.AngularAcceleration);

    pose.poseIsValid = cxrTrue;

    return pose;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::DoTracking(double predictedTimeS)
{
    ProcessControllers(predictedTimeS);

    ovrTracking2_ tracking = vrapi_GetPredictedTracking2(mOvrSession, predictedTimeS);

    TrackingState.poseTimeOffset = ClientPredictionOffset;

    TrackingState.hmd.ipd = vrapi_GetInterpupillaryDistance(&tracking);
    // the quest2 ipd sensor reports infinitesimal changes every frame, even when the user has not adjusted the headset IPD
    // so we truncate the value to 5 decimal places (sub-millimeter precision)
    TrackingState.hmd.ipd = truncf(TrackingState.hmd.ipd * 10000.0f) / 10000.0f;
    TrackingState.hmd.flags = 0; // reset dynamic flags every frame
    TrackingState.hmd.flags |= cxrHmdTrackingFlags_HasIPD; // TODO: consider tracking local IPD value and only flag when it actually changes...

    if (mRefreshChanged)
    {
        TrackingState.hmd.displayRefresh = std::fminf(mTargetDisplayRefresh, 90.0f);
        TrackingState.hmd.flags |= cxrHmdTrackingFlags_HasRefresh;
        // TODO: should we have this mutex protected so no race condition on it?
        mRefreshChanged = false;
    }

    mLastHeadPose = tracking.HeadPose;
    TrackingState.hmd.pose = ConvertPose(tracking.HeadPose);
    TrackingState.hmd.pose.poseIsValid = ((tracking.Status & VRAPI_TRACKING_STATUS_ORIENTATION_VALID) > 0) ? cxrTrue : cxrFalse;
    TrackingState.hmd.pose.deviceIsConnected = ((tracking.Status & VRAPI_TRACKING_STATUS_HMD_CONNECTED) > 0) ? cxrTrue : cxrFalse;
    TrackingState.hmd.pose.trackingResult = cxrTrackingResult_Running_OK;
    TrackingState.hmd.activityLevel = cxrDeviceActivityLevel_UserInteraction;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::GetTrackingState(cxrVRTrackingState* trackingState)
{
    // TODO TBD!!! we used to use null to do the ovr api calls on loading/exiting screens
    //  but that generates events and state changes the system isn't expecting.  so return for now.
    if (nullptr==trackingState) return; // TODO see if any issues not processing tracking.

    // Unless the predicted time is used, tracking state will not be
    // filtered and as a result view will be jumping all over the place.
    const double predictedTimeS = ClientPredictionOffset == 0.0 ? 0.0 : GetTimeInSeconds() + ClientPredictionOffset;
    // TODO: look into replacing this with mNextDisplayTime so tracking closer to real scanout

    DoTracking(predictedTimeS);
    if (trackingState != nullptr)
        *trackingState = TrackingState;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrDeviceDesc OKCloudStreamer::GetDeviceDesc(float fovX, float fovY)
{
    cxrDeviceDesc desc = {};
    if (mJavaCtx.Vm == nullptr)
    {
        CXR_LOGE("Java context is null.");
        return desc;
    }

    int texW = vrapi_GetSystemPropertyInt( &mJavaCtx, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH );
    int texH = vrapi_GetSystemPropertyInt( &mJavaCtx, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT );
    int dispW = vrapi_GetSystemPropertyInt( &mJavaCtx, VRAPI_SYS_PROP_DISPLAY_PIXELS_WIDE );
    int dispH = vrapi_GetSystemPropertyInt( &mJavaCtx, VRAPI_SYS_PROP_DISPLAY_PIXELS_HIGH );

    // get the rate the system is running at right now.
    // TODO: should this be a Float property??
    const int currDisplayRefresh = vrapi_GetSystemPropertyInt( &mJavaCtx, VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE );
    CXR_LOGI("System property says current display refresh set to %d", currDisplayRefresh);

    // TODO: atm we ignore current rate, and use a hardcoded default (72hz)
    // TODO: we may want to switch this to assign currDisplayRefresh instead, as that might be 90 at some point...
    mTargetDisplayRefresh = cDefaultDisplayRefresh; // this will be our fallback value.

    if (GOptions.mRequestedRefreshRate <= 0.0f)
    { // leave as default for now
        CXR_LOGI("Override for display refresh not specified, so defaulting to %0.2f", mTargetDisplayRefresh);
        // TODO: we may down the line want to use this case to clamp to something other than mRequested/cDefault.
        //  for example, if no override given, maybe we should try to set to current?
    }
    else
    { // some refresh was requested.
        // query how many different display rates are supported by the system, then get an array of them.
        const int numRates = vrapi_GetSystemPropertyInt(&mJavaCtx,
                                                  VRAPI_SYS_PROP_NUM_SUPPORTED_DISPLAY_REFRESH_RATES);
        std::vector<float> supportedRates(numRates);
        int retNumRates = vrapi_GetSystemPropertyFloatArray(&mJavaCtx,
                                                            VRAPI_SYS_PROP_SUPPORTED_DISPLAY_REFRESH_RATES,
                                                            supportedRates.data(), numRates);

        // if we find more rates available, we look to match requested rate as close as possible.
        // NOTE: this does allow for 120hz mode if enabled on a system and is the requested rate.

        // TODO: Note that CloudXR does NOT currently, actively support 120hz.  You can request it, but no guarantee it works smoothly...

        // TODO: We may want the option and checks in float, as often display rates are not perfect integers.
        //  For example, display mode might might be 59.97, and request is 60, so we will round and match, but setting
        //  might not work.  Might need to stash the floating point value we 'matched' for future reference at minimum.
        if (numRates >= 1) // try to find a matching rate to launch option
        {
            CXR_LOGE("Launch options requested display refreh of %u, checking list of %d available rates",
                  (GOptions.mRequestedRefreshRate), numRates);

            // for debugging, we'll print the list of supportedRates
            CXR_LOGI("Display support rates of:");
            for (auto rate : supportedRates)
            {
                CXR_LOGI("Refresh = %0.2f hz", rate);
            }

            // then we loop through the rate list to find something 'close enough' (less than 1hz from request).
            for (auto rate : supportedRates)
            {
                // try for closest int value for now (within 1hz seems fine), can update to floats all over if needed
                if (abs(rate - (float)(GOptions.mRequestedRefreshRate)) < 1.0f)
                { // found what we wanted -- update variable.
                    mTargetDisplayRefresh = rate;
                    CXR_LOGE("Choosing closest display rate of %0.2f", rate);
                    break;
                }
            }
        }
    }

    if (mOvrSession == nullptr)
    {
        CXR_LOGE("OVR session is null, cannot continue.");
        return {}; // false..
    }

    CXR_LOGI("Setting display rate to %0.2f hz.", mTargetDisplayRefresh);
    ovrResult result = vrapi_SetDisplayRefreshRate(mOvrSession, mTargetDisplayRefresh);
    if (result != ovrSuccess)
    {
        // there are two known cases called out by the api header.
        if (result == ovrError_InvalidParameter) // rate not supported
        {
            CXR_LOGE("Unable to set display rate to 0.2f, unsupported rate.");
        }
        else
        if (result == ovrError_InvalidOperation) // rate can't be set right now -- like low power mode
        {
            CXR_LOGE("Unable to set display rate to 0.2f at this time (may be in low power mode?)");
        }

        // I think the right thing here is to reset the member to default value...
        // TODO: reminder if we change to assigning current refresh, this line will want to match init code.
        mTargetDisplayRefresh = cDefaultDisplayRefresh;
    }
    CXR_LOGI("vrapi HMD Props, texture = %d x %d, display = %d x %d @ %0.2f", texW, texH, dispW, dispH, mTargetDisplayRefresh);

//#define DISP_RES_OCULUS_SUGGESTED
#ifdef DISP_RES_OCULUS_SUGGESTED
    // This is using the suggested texture size for optimal perf per Oculus sdk design
    uint32_t width = texW;
    uint32_t height = texH;
#else
    // TODO: This is trying to use display-native per eye w/h instead.
    uint32_t width = 1920;// dispW / 2;
    uint32_t height = 1920;//dispH;
#endif

    desc.numVideoStreamDescs = CXR_NUM_VIDEO_STREAMS_XR;
    for (uint32_t i = 0; i < desc.numVideoStreamDescs; i++)
    {
        desc.videoStreamDescs[i].format = cxrClientSurfaceFormat_RGB;
        desc.videoStreamDescs[i].width = width;
        desc.videoStreamDescs[i].height = height;
        desc.videoStreamDescs[i].fps = mTargetDisplayRefresh;
        desc.videoStreamDescs[i].maxBitrate = GOptions.mMaxVideoBitrate;
    }
    desc.stereoDisplay = true;

    desc.maxResFactor = 1.0f;//GOptions.mMaxResFactor;

    const int maxWidth = (int)(desc.maxResFactor * (float)width);
    const int maxHeight = (int)(desc.maxResFactor * (float)height);
    CXR_LOGI("HMD size requested as %d x %d, max %d x %d", width, height, maxWidth, maxHeight);

    // Get IPD from OVR API
    const double predictedDisplayTime = vrapi_GetPredictedDisplayTime( mOvrSession, 0 );
    const ovrTracking2 tracking = vrapi_GetPredictedTracking2( mOvrSession, predictedDisplayTime );
    desc.ipd = vrapi_GetInterpupillaryDistance(&tracking);

    desc.predOffset = ServerPredictionOffset;
    desc.receiveAudio = GOptions.mReceiveAudio;
    desc.sendAudio = GOptions.mSendAudio;
    desc.posePollFreq = 0;
    desc.disablePosePrediction = false;
    desc.angularVelocityInDeviceSpace = false;
    desc.foveatedScaleFactor = 0;//(GOptions.mFoveation < 100) ? GOptions.mFoveation : 0;

    const float halfFOVTanX = tanf(VRAPI_PI/360.f * fovX);
    const float halfFOVTanY = tanf(VRAPI_PI/360.f * fovY);

    desc.proj[0][0] = -halfFOVTanX;
    desc.proj[0][1] =  halfFOVTanX;
    desc.proj[0][2] = -halfFOVTanY;
    desc.proj[0][3] =  halfFOVTanY;
    desc.proj[1][0] = -halfFOVTanX;
    desc.proj[1][1] =  halfFOVTanX;
    desc.proj[1][2] = -halfFOVTanY;
    desc.proj[1][3] =  halfFOVTanY;
    QueryChaperone(&desc);

    return desc;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::RecreateSwapchain(uint32_t width, uint32_t height, uint32_t eye)
{
    // TODO: this log should likely be Warning level, as this is expensive and we should
    //   see clearly in log when it happens.  Don't think it needs to be Error, but will for now.
    CXR_LOGE("Recreating swapchain for eye%d: %d x %d (was %d x %d)",
          eye, width, height, EyeWidth[eye], EyeHeight[eye]);

    if (SwapChains[eye])
        vrapi_DestroyTextureSwapChain(SwapChains[eye]);

    SwapChains[eye] = vrapi_CreateTextureSwapChain2(VRAPI_TEXTURE_TYPE_2D,
                                                    VRAPI_TEXTURE_FORMAT_8888_sRGB,
                                                    width, height,
                                                    1, SwapChainLen);
    EyeWidth[eye] = width;
    EyeHeight[eye] = height;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::TriggerHaptic(
       const cxrHapticFeedback* hapticFeedback)
{
    const cxrHapticFeedback& haptic = *hapticFeedback;

    if (haptic.seconds <= 0)
        return;

    uint32_t deviceIndex = 0;
    ovrInputCapabilityHeader capsHeader;
    while (vrapi_EnumerateInputDevices(mOvrSession, deviceIndex, &capsHeader) >= 0)
    {
        ++deviceIndex;

        if (capsHeader.Type == ovrControllerType_TrackedRemote)
        {
            ovrInputTrackedRemoteCapabilities remoteCaps;
            remoteCaps.Header = capsHeader;
            vrapi_GetInputDeviceCapabilities(mOvrSession, &remoteCaps.Header);

            // now we simply compare the physical controller ID.
            if (haptic.deviceID != capsHeader.DeviceID)
                continue;

            // and of course, sanity check this device HAS haptic support...
            if (0 == (remoteCaps.ControllerCapabilities & ovrControllerCaps_HasBufferedHapticVibration))
                continue;

            ovrHapticBuffer hapticBuffer;
            hapticBuffer.BufferTime = GetTimeInSeconds() + 0.03; // TODO: use mNextDisplayTime?
            hapticBuffer.NumSamples = remoteCaps.HapticSamplesMax;
            hapticBuffer.HapticBuffer =
                    reinterpret_cast<uint8_t*>(alloca(remoteCaps.HapticSamplesMax));
            hapticBuffer.Terminated = true;

            for (uint32_t i = 0; i < hapticBuffer.NumSamples; i++)
            {
                hapticBuffer.HapticBuffer[i] =
                        static_cast<uint8_t>(haptic.amplitude*255.f);
            }

            vrapi_SetHapticVibrationBuffer(mOvrSession, capsHeader.DeviceID, &hapticBuffer);
        }
    }
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
cxrBool OKCloudStreamer::RenderAudio(const cxrAudioFrame *audioFrame)
{
    if (!playbackStream)
    {
        return cxrFalse;
    }

    const uint32_t timeout = audioFrame->streamSizeBytes / CXR_AUDIO_BYTES_PER_MS;
    const uint32_t numFrames = timeout * CXR_AUDIO_SAMPLING_RATE / 1000;
    playbackStream->write(audioFrame->streamBuffer, numFrames, timeout * oboe::kNanosPerMillisecond);

    return cxrTrue;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::SubmitLayers(const ovrLayerHeader2* layers[], int layerCount, ovrFrameFlags flags)
{
    ovrSubmitFrameDescription2 frameDesc = {0};
    frameDesc.LayerCount = layerCount;
    frameDesc.Layers = layers;
    frameDesc.Flags = flags;
    frameDesc.SwapInterval = 1;
    frameDesc.FrameIndex = mFrameCounter;
    frameDesc.DisplayTime = mNextDisplayTime;
    vrapi_SubmitFrame2(mOvrSession, &frameDesc);
}


//-----------------------------------------------------------------------------
// Here we render the loading 'spinner' while we're starting up.
//-----------------------------------------------------------------------------
void OKCloudStreamer::RenderLoadScreen()
{
    GetTrackingState(NULL);
    ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
    blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
    ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
    iconLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
    const ovrLayerHeader2* layers[] = { &blackLayer.Header, &iconLayer.Header };
    SubmitLayers(layers, 2, VRAPI_FRAME_FLAG_FLUSH);
}


//-----------------------------------------------------------------------------
// Clear to black for exit, and tell VrApi we are done submitting frames.
//-----------------------------------------------------------------------------
void OKCloudStreamer::RenderExitScreen()
{
    GetTrackingState(NULL);
    ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
    const ovrLayerHeader2* layers[] = { &layer.Header };
    SubmitLayers(layers, 1,
                 static_cast<ovrFrameFlags>(VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL));
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::Render()
{
    // This is the only place the frame index is incremented, right before
    // calling vrapi_GetPredictedDisplayTime().
    mFrameCounter++;
    mNextDisplayTime = vrapi_GetPredictedDisplayTime(mOvrSession, mFrameCounter);

    ovrLayerProjection2 worldLayer = vrapi_DefaultLayerProjection2();

    worldLayer.HeadPose = mLastHeadPose;
    worldLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

#if later
    if (GOptions.mTestLatency)
    {
        // TODO: for this non-connected test mode, or any other local UX, we currently have
        //  to manually call call GetTrackingState(null) to update input and hmd status -- as
        //  current code only updates state during client callback when connected to server.
        //  Future improvement would be to run update in a separate thread, and be able to copy
        //  out those results for the callback or here.
        if (testButtonPressed)
            mBGColor = 0xFFFFFFFF;
        else
            mBGColor = mDefaultBGColor;
    }
#endif

    // Fetch a CloudXR frame
    cxrFramesLatched framesLatched;
    const uint32_t timeoutMs = 500;
    bool frameValid = false;

    if (Receiver)
    {
        if (mClientState == cxrClientState_StreamingSessionInProgress)
        {
            cxrError frameErr = cxrLatchFrame(Receiver, &framesLatched,
                                            cxrFrameMask_All, timeoutMs);
            frameValid = (frameErr == cxrError_Success);
            if (!frameValid)
            {
                if (frameErr == cxrError_Frame_Not_Ready)
                {
                    CXR_LOGI("LatchFrame failed, frame not ready for %d ms", timeoutMs);
                }
                else if (frameErr == cxrError_Not_Connected)
                {
                    CXR_LOGE("LatchFrame failed, receiver no longer connected.");
                    RequestExit();
                }
                else
                {
                    CXR_LOGE("LatchFrame failed with error: %s", cxrErrorString(frameErr));
                }
            }
        }
    }

    for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++)
    {
        // if valid frame and the size has changed, update our buffers to match.
        // TODO: we might want to use a frame subrect if the buffer is BIGGER and we're shrinking,
        //  just to avoid allocation thrash and hiccups due to it.
        cxrVideoFrame &vf = framesLatched.frames[eye];
        if (frameValid && (vf.widthFinal != EyeWidth[eye] || vf.heightFinal != EyeHeight[eye]) )
            RecreateSwapchain(vf.widthFinal, vf.heightFinal, eye);

        const uint32_t swapChainLength = vrapi_GetTextureSwapChainLength(SwapChains[eye]);
        const int swapChainIndex = mFrameCounter % swapChainLength;
        const GLuint colorTexture = vrapi_GetTextureSwapChainHandle(SwapChains[eye], swapChainIndex);

        if (SetupFramebuffer(colorTexture, eye))
        {
            if (frameValid)
            {
                // blit streamed frame into the world layer
                cxrBlitFrame(Receiver, &framesLatched, 1<<eye);
            }
            else
            {
                FillBackground();
            }

            // NOTE: this is where a given app might render UI/overlays
        }

        worldLayer.Textures[eye].ColorSwapChain = SwapChains[eye];
        worldLayer.Textures[eye].SwapChainIndex = swapChainIndex;
        worldLayer.Textures[eye].TexCoordsFromTanAngles = TexCoordsFromTanAngles;
    }

    if (frameValid) // means we had a receiver AND latched frame.
    {
        worldLayer.HeadPose.Pose.Orientation = cxrToQuaternion(framesLatched.poseMatrix);
        worldLayer.HeadPose.Pose.Position = cxrGetTranslation(framesLatched.poseMatrix);

        cxrReleaseFrame(Receiver, &framesLatched);

        // Log connection stats every 3 seconds
        const int STATS_INTERVAL_SEC = 3;
        mFramesUntilStats--;
        if (mFramesUntilStats <= 0 &&
            cxrGetConnectionStats(Receiver, &mStats) == cxrError_Success)
        {
            // Capture the key connection statistics
            char statsString[64] = { 0 };
            snprintf(statsString, 64, "FPS: %6.1f    Bitrate (kbps): %5d    Latency (ms): %3d", mStats.framesPerSecond, mStats.bandwidthUtilizationKbps, mStats.roundTripDelayMs);

            // Turn the connection quality into a visual representation along the lines of a signal strength bar
            char qualityString[64] = { 0 };
            snprintf(qualityString, 64, "Connection quality: [%c%c%c%c%c]",
                     mStats.quality >= cxrConnectionQuality_Bad ? '#' : '_',
                     mStats.quality >= cxrConnectionQuality_Poor ? '#' : '_',
                     mStats.quality >= cxrConnectionQuality_Fair ? '#' : '_',
                     mStats.quality >= cxrConnectionQuality_Good ? '#' : '_',
                     mStats.quality == cxrConnectionQuality_Excellent ? '#' : '_');

            // There could be multiple reasons for low quality however we show only the most impactful to the end user here
            char reasonString[64] = { 0 };
            if (mStats.quality <= cxrConnectionQuality_Fair)
            {
                if (mStats.qualityReasons == cxrConnectionQualityReason_EstimatingQuality)
                {
                    snprintf(reasonString, 64, "Reason: Estimating quality");
                }
                else if (mStats.qualityReasons & cxrConnectionQualityReason_HighLatency)
                {
                    snprintf(reasonString, 64, "Reason: High Latency (ms): %3d", mStats.roundTripDelayMs);
                }
                else if (mStats.qualityReasons & cxrConnectionQualityReason_LowBandwidth)
                {
                    snprintf(reasonString, 64, "Reason: Low Bandwidth (kbps): %5d", mStats.bandwidthAvailableKbps);
                }
                else if (mStats.qualityReasons & cxrConnectionQualityReason_HighPacketLoss)
                {
                    if (mStats.totalPacketsLost == 0)
                    {
                        snprintf(reasonString, 64, "Reason: High Packet Loss (Recoverable)");
                    }
                    else
                    {
                        snprintf(reasonString, 64, "Reason: High Packet Loss (%%): %3.1f", 100.0f * mStats.totalPacketsLost / mStats.totalPacketsReceived);
                    }
                }
            }

            CXR_LOGI("%s    %s    %s", statsString, qualityString, reasonString);
            mFramesUntilStats = (int)mStats.framesPerSecond * STATS_INTERVAL_SEC;
        }
    }

    const ovrLayerHeader2* layers[] = { &worldLayer.Header };
    SubmitLayers(layers, 1, static_cast<ovrFrameFlags>(0));
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::AppResumed()
{
    if (mOvrSession == nullptr)
    {
        CXR_LOGE("OVR session is null, cannot continue.");
        RequestExit();
        return; // false..
    }

    // Force floor level tracking space
    vrapi_SetTrackingSpace(mOvrSession, VRAPI_TRACKING_SPACE_LOCAL_FLOOR);

    // set our color space
    // NOTE: oculus color space guide says to use CV1 and NOT rec709, even if 709 seems correct choice.
    ovrHmdColorDesc colorspace{VRAPI_COLORSPACE_RIFT_CV1};
    vrapi_SetClientColorDesc(mOvrSession, &colorspace);

    float EyeFovDegreesX = vrapi_GetSystemPropertyFloat(&mJavaCtx,
            VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);
    float EyeFovDegreesY = vrapi_GetSystemPropertyFloat(&mJavaCtx,
            VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);

    CXR_LOGI("Headset suggested FOV: %.1f x %.1f.", EyeFovDegreesX, EyeFovDegreesY);

    const auto projectionMatrix = ovrMatrix4f_CreateProjectionFov(
            EyeFovDegreesX, EyeFovDegreesY, 0.0f, 0.0f, VRAPI_ZNEAR, 0.0f);

    TexCoordsFromTanAngles =
            ovrMatrix4f_TanAngleMatrixFromProjection(&projectionMatrix);

    // get controller state and HMD state up-front now.
    DetectControllers();
    // clear input history.  this might be messy if we paused in
    // different state, but can't trust leaving and coming back
    // and guaranteeing input historical status is 'static'.
    memset(mLastInputState, 0, sizeof(mLastInputState));
    mDeviceDesc = GetDeviceDesc(EyeFovDegreesX, EyeFovDegreesY);

    // create the initial swapchain buffers based on HMD specs.
    for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++)
        RecreateSwapchain(mDeviceDesc.videoStreamDescs[eye].width, mDeviceDesc.videoStreamDescs[eye].height, eye);

    // TODO: move this to a once-per-frame check like wvr sample does in its UpdatePauseLogic fn.
    if (!Receiver && mReadyToConnect &&
        CreateReceiver() != cxrError_Success)
    {
        CXR_LOGE("Failed to create the receiver, exiting...");
        RequestExit();
        return;
    }
    else // precondition or CreateReceiver failed test
    {
        // if connecting async, state management happens in callback.
        // but if sync, we reach this point after successful Connect
        // so update renderstate.
        if (!mConnectionDesc.async)
        {
            mRenderState = RenderState_Running;
        }
    }

    // now match variable state
    mWasPaused = mIsPaused;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::AppPaused() {
    // TODO verify whether we need a mutex around resources here to ensure some thread isn't
    //  rendering actively while we're pausing...
    CXR_LOGI("App Paused");

    for (int i = 0; i < NumEyes; ++i)
    {
        glDeleteFramebuffers(1, &Framebuffers[i]);
        Framebuffers[i] = 0;
    }

    if (Receiver)
    {
        TeardownReceiver();
        if (mClientState != cxrClientState_Exiting)
        {
            mClientState = cxrClientState_ReadyToConnect;
            mRenderState = RenderState_Loading;
            CXR_LOGI("Receiver destroyed, client state reset.");
        }
    }

    for (auto& swapChain : SwapChains)
    {
        if (swapChain != nullptr)
            vrapi_DestroyTextureSwapChain(swapChain);
        swapChain = nullptr;
    }

    EyeWidth[0] = EyeWidth[1] = EyeHeight[0] = EyeHeight[1] = 0;
    TrackingState = {};

    // now match variable state
    mWasPaused = mIsPaused;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool OKCloudStreamer::EnterVRMode()
{
    if (mOvrSession == NULL) {
        ovrModeParms parms = vrapi_DefaultModeParms(&mJavaCtx);

        // Note for future from ovr sdk: don't need to reset FS flag when using a View
        parms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
        parms.Flags |= VRAPI_MODE_FLAG_FRONT_BUFFER_SRGB; // bc we have full SRGB pipeline
        parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW; // bc we're using NativeWindow
        parms.WindowSurface = (size_t)GetWindow(); // the NativeWindow
        parms.Display = (size_t)(mEglHelper.GetDisplay());
        parms.ShareContext = (size_t)(mEglHelper.GetContext());

        CXR_LOGI("        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));
        CXR_LOGI("        vrapi_EnterVrMode()");
        mOvrSession = vrapi_EnterVrMode(&parms);
        CXR_LOGI("        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

        // If entering VR mode failed then the ANativeWindow was not valid.
        if (mOvrSession == NULL) {
            CXR_LOGE("EnterVrMode failed, assuming invalid ANativeWindow (%p)!", GetWindow());
            return false;
        }

        // Set performance parameters once we have entered VR mode and have a valid ovrMobile.
        if (mOvrSession != NULL) {
            vrapi_SetClockLevels(mOvrSession, CPU_LEVEL, GPU_LEVEL);
            CXR_LOGI("		vrapi_SetClockLevels( %d, %d )", CPU_LEVEL, GPU_LEVEL);

            vrapi_SetPerfThread(mOvrSession, VRAPI_PERF_THREAD_TYPE_MAIN, gettid());
            CXR_LOGI("		vrapi_SetPerfThread( MAIN, %d )", gettid());

//            vrapi_SetPerfThread(mOvrSession, VRAPI_PERF_THREAD_TYPE_RENDERER, gettid());
//            CXR_LOGI("		vrapi_SetPerfThread( RENDERER, %d )", gettid());
        }
    }

    return true;  // we have ovr session.
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void OKCloudStreamer::HandleVrModeChanges()
{
    if (mIsPaused==mWasPaused) // nothing to do then.
        return;

    if (!mIsPaused && mNativeWindow != NULL && mClientState != cxrClientState_Exiting)
    {
        if (!EnterVRMode())
        {
            // might need to notify user as well here.
            CXR_LOGE("Failed to enter VR mode, exiting...");
            RequestExit();
        }
        else
        {
            // then run app-layer resume code.
            AppResumed();
        }
    }
    else
    {
        if (mIsPaused || mClientState == cxrClientState_Exiting)
        {
            if (mOvrSession != NULL)
            {
                CXR_LOGI("CALLING vrapi_LeaveVrMode()");
                vrapi_LeaveVrMode(mOvrSession);
                mOvrSession = NULL;
            }
            // app-layer pause code
            AppPaused();
        }
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
oboe::DataCallbackResult OKCloudStreamer::onAudioReady(oboe::AudioStream *oboeStream,
        void *audioData, int32_t numFrames)
{
    cxrAudioFrame recordedFrame{};
    recordedFrame.streamBuffer = (int16_t*)audioData;
    recordedFrame.streamSizeBytes = numFrames * CXR_AUDIO_CHANNEL_COUNT * CXR_AUDIO_SAMPLE_SIZE;
    cxrSendAudio(Receiver, &recordedFrame);

    return oboe::DataCallbackResult::Continue;
}

//-----------------------------------------------------------------------------
// TODO: we may need to handle these events for better app lifecycle.
//-----------------------------------------------------------------------------
void OKCloudStreamer::HandleVrApiEvents()
{
    ovrEventDataBuffer eventDataBuffer = {};

    // Poll for VrApi events
    for (;;) {
        ovrEventHeader* eventHeader = (ovrEventHeader*)(&eventDataBuffer);
        ovrResult res = vrapi_PollEvent(eventHeader);
        if (res != ovrSuccess) {
            break;
        }

        switch (eventHeader->EventType) {
            case VRAPI_EVENT_DATA_LOST:
                CXR_LOGI("vrapi_PollEvent: Received VRAPI_EVENT_DATA_LOST");
                break;
            case VRAPI_EVENT_VISIBILITY_GAINED:
                CXR_LOGI("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_GAINED");
                break;
            case VRAPI_EVENT_VISIBILITY_LOST:
                CXR_LOGI("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_LOST");
                break;
            case VRAPI_EVENT_FOCUS_GAINED:
                // FOCUS_GAINED is sent when the application is in the foreground and has
                // input focus. This may be due to a system overlay relinquishing focus
                // back to the application.
                // TODO: to be implemented...
                CXR_LOGI("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_GAINED");
                break;
            case VRAPI_EVENT_FOCUS_LOST:
                // FOCUS_LOST is sent when the application is no longer in the foreground and
                // therefore does not have input focus. This may be due to a system overlay taking
                // focus from the application. The application should take appropriate action when
                // this occurs.
                // TODO: to be implemented...
                CXR_LOGI("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_LOST");
                break;
            case VRAPI_EVENT_DISPLAY_REFRESH_RATE_CHANGE:
            {
                // TODO: consider if we should wrap with a mutex, unclear which thread things occur on, and changed flag wants to be protected.
                ovrEventDisplayRefreshRateChange *rrc = reinterpret_cast<ovrEventDisplayRefreshRateChange*>(eventHeader);
                CXR_LOGI("vrapi_PollEvent: Received VRAPI_EVENT_DISPLAY_REFRESH_RATE_CHANGE");
                CXR_LOGI("Refresh changing from %0.2f to %0.2f", rrc->fromDisplayRefreshRate, rrc->toDisplayRefreshRate);
                // update the member as rate changed under the covers already.
                mTargetDisplayRefresh = rrc->toDisplayRefreshRate;
                // flag to system so next pose update includes this change
                mRefreshChanged = true;

                // get the rate the system thinks it is running at right now.
                // TODO: should this be a Float property??
                const int currDisplayRefresh = vrapi_GetSystemPropertyInt( &mJavaCtx, VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE );
                CXR_LOGI("REFRESH CHANGED! API now returns display refresh as %d", currDisplayRefresh);

                break;
            }
            default:
                CXR_LOGI("vrapi_PollEvent: Unknown event");
                break;
        }
    }

    // now seems as good a time as any to cache state of the hmd
    mHeadsetOnHead = (vrapi_GetSystemStatusInt(&mJavaCtx, VRAPI_SYS_STATUS_MOUNTED) != VRAPI_FALSE);
}
//-----------------------------------------------------------------------------
// we assume here env!=null.
//-----------------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL
Java_com_nvidia_cloudxr_ovr_MainActivity_nativeHandleLaunchOptions(JNIEnv* env, jobject act, jstring jcmdline)
{
    std::lock_guard<std::mutex> lock(GJniMutex);

    // acquire any cmdline from java
    std::string cmdline = "";
    if (jcmdline != nullptr) {
        const char *utfstr = env->GetStringUTFChars(jcmdline, 0);
        if (utfstr != nullptr) {
            CXR_LOGI("Commandline received from Java: %s", utfstr);
            cmdline = utfstr;
            env->ReleaseStringUTFChars(jcmdline, utfstr);
        }
    }

    std::string optionsPath = gClientHandle->GetBasePath();
    optionsPath.append("/CloudXRLaunchOptions.txt");
    CXR_LOGI("Attempting to load launch options from: %s", optionsPath.c_str());
    // first, try to read "command line in a text file"
    ParseStatus p = GOptions.ParseFile(optionsPath.c_str());
    if (p != ParseStatus_Success)
        CXR_LOGE("Unable to open launch options file, %s", strerror(errno));
    // next, process actual 'commandline' args -- overrides any prior values
    GOptions.ParseString(cmdline);

    // For the moment, we prefer to set up logging as early as possible,
    // and it depends upon options having been parsed.
    // Set any logger options PRIOR to init call.
    if (GOptions.mDebugFlags & cxrDebugFlags_LogQuiet) // quiet takes precedence
        g_logFile.setLogLevel(cxrLL_Silence);
    else if (GOptions.mDebugFlags & cxrDebugFlags_LogVerbose)
        g_logFile.setLogLevel(cxrLL_Verbose);
    else
        g_logFile.setLogLevel(cxrLL_Debug); // otherwise defaults to Info.

    g_logFile.setPrivacyEnabled((GOptions.mDebugFlags & cxrDebugFlags_LogPrivacyDisabled) ? 0 : 1);
    g_logFile.setMaxSizeKB(GOptions.mLogMaxSizeKB);
    g_logFile.setMaxAgeDays(GOptions.mLogMaxAgeDays);

    std::string filePrefix = "Oculus Sample";
    g_logFile.init(gClientHandle->GetOutputPath(), filePrefix);

    // if running local latency test, clear server IP so we don't try to connect.
    if (GOptions.mTestLatency && !GOptions.mServerIP.empty())
        GOptions.mServerIP.clear();

    if (GOptions.mTestLatency)
        gClientHandle->SetDefaultBGColor(0xFF000000); // black for now.
    else
        gClientHandle->SetDefaultBGColor(0xFF555555); // dark gray for now.

#if 1
    if (GOptions.mServerIP.empty())
    {
        GOptions.mServerIP = "192.168.2.38";
    }
#endif


    // check if we have a server yet (if have no 'input UI', we have no other source)
    if (!GOptions.mServerIP.empty())
    {
        gClientHandle->SetReadyToConnect(true);
    }
    else
    {
        if (!GOptions.mTestLatency)
        {
            CXR_LOGE("No server IP specified to connect to.");
            // TODO: until we have a VR UI, we should exit here and post system dialog somehow.
            gClientHandle->RequestExit();
        }
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void android_main(struct android_app* app)
{
    cxrError status = cxrError_Success;
    GAndroidApp = app;
    OKCloudStreamer cxrcOvr(app);
    gClientHandle = &cxrcOvr;

    ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

    status = cxrcOvr.Initialize();
    if (status==cxrError_Success)
    {
        status = cxrcOvr.MainLoop();
        // if (status != cxrError_Success)
        // // TODO then report error if haven't already.
    }

    cxrcOvr.Release();
    gClientHandle = NULL;
    GAndroidApp = NULL;

    CXR_LOGI("Finishing the NativeActivity.");
    ANativeActivity_finish(app->activity);
    // just return to native app glue, let it run destroy, activity finish does the rest.

    CXR_LOGE("Exiting android_main, library is in limbo until process terminated.");

    g_logFile.destroy(); // just making it explicit.

    // TODO: after return, app_destroy doesn't terminate the process.  we need to unload and
    //  reload native bits potentially -- or at least reset in constructors and watch statics.
    exit(1);
}
