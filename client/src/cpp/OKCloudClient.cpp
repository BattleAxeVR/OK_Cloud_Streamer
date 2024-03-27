//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ok_defines.h"

#if ENABLE_CLOUDXR

#include "OKCloudClient.h"

#if (ENABLE_CLOUDXR_LOGGING && 0)
extern "C" void dispatchLogMsg(cxrLogLevel level, cxrMessageCategory category, void *extra, const char *tag, const char *fmt, ...)
{
}
#endif

namespace BVR 
{

cxrVector3 convert_xr_to_cxr(const XrVector3f &xr_vec)
{
    cxrVector3 cxr_vec;
    cxr_vec.v[0] = xr_vec.x;
    cxr_vec.v[1] = xr_vec.y;
    cxr_vec.v[2] = xr_vec.z;
    return cxr_vec;
}

XrVector3f convert_cxr_to_xr(const cxrVector3 &cxr_vec)
{
    XrVector3f xr_vec;
    xr_vec.x = cxr_vec.v[0];
    xr_vec.y = cxr_vec.v[1];
    xr_vec.z = cxr_vec.v[2];
    return xr_vec;
}

cxrQuaternion convert_xr_to_cxr(const XrQuaternionf &xr_quat)
{
    cxrQuaternion cxr_quat;
    cxr_quat.w = xr_quat.w;
    cxr_quat.x = xr_quat.x;
    cxr_quat.y = xr_quat.y;
    cxr_quat.z = xr_quat.z;
    return cxr_quat;
}

XrQuaternionf convert_cxr_to_xr(const cxrQuaternion &cxr_quat)
{
    XrQuaternionf xr_quat;
    xr_quat.w = cxr_quat.w;
    xr_quat.x = cxr_quat.x;
    xr_quat.y = cxr_quat.y;
    xr_quat.z = cxr_quat.z;
    return xr_quat;
}


    cxrTrackedDevicePose convert_glm_to_cxr_pose(const GLMPose &glm_pose)
    {
        cxrTrackedDevicePose cxr_pose = {};

        cxr_pose.position.v[0] = glm_pose.translation_.x;
        cxr_pose.position.v[1] = glm_pose.translation_.y;
        cxr_pose.position.v[2] = glm_pose.translation_.z;

        cxr_pose.rotation.w = glm_pose.rotation_.w;
        cxr_pose.rotation.x = glm_pose.rotation_.x;
        cxr_pose.rotation.y = glm_pose.rotation_.y;
        cxr_pose.rotation.z = glm_pose.rotation_.z;

        cxr_pose.deviceIsConnected = true;
        cxr_pose.poseIsValid = glm_pose.is_valid_;
        cxr_pose.trackingResult = cxrTrackingResult_Running_OK;

        return cxr_pose;
    }

    cxrVector3 convert_xr_to_cxr_vector3(const XrVector3f &input)
    {
        cxrVector3 output = {0};

        output.v[0] = input.x;
        output.v[1] = input.y;
        output.v[2] = input.z;

        return output;
    }

    cxrQuaternion convert_xr_to_cxr_quat(const XrQuaternionf &input)
    {
        cxrQuaternion output = {0};

        output.w = input.w;
        output.x = input.x;
        output.y = input.y;
        output.z = input.z;

        return output;
    }

    cxrTrackedDevicePose convert_xr_to_cxr_pose(XrSpaceLocation &location)
    {
        const XrPosef &xr_pose = location.pose;

        cxrTrackedDevicePose cxr_pose = {};
        cxr_pose.position = convert_xr_to_cxr_vector3(xr_pose.position);
        cxr_pose.rotation = convert_xr_to_cxr_quat(xr_pose.orientation);

        XrSpaceVelocity *velocity = (XrSpaceVelocity *) location.next;

        if (velocity && velocity->type == XR_TYPE_SPACE_VELOCITY)
        {
            if (velocity->velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
            {
                cxr_pose.velocity = convert_xr_to_cxr_vector3(velocity->linearVelocity);
            }

            if (velocity->velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
            {
                cxr_pose.angularVelocity = convert_xr_to_cxr_vector3(velocity->angularVelocity);
            }
        }

        cxr_pose.deviceIsConnected = true;
        cxr_pose.poseIsValid = true;
        cxr_pose.trackingResult = cxrTrackingResult_Running_OK;

        return cxr_pose;
    }


static DigitalButtonToCloudXR_Map digital_button_maps[][NUM_CONTROLLERS] = {{{DigitalButton_ApplicationMenu, 0},
                                                                                    { DigitalButton_System,INVALID_INDEX }},
                                                                            {{DigitalButton_Trigger_Click, 2},
                                                                                    { DigitalButton_Trigger_Click,2 }},
                                                                            {{DigitalButton_Trigger_Touch, 3},
                                                                                    { DigitalButton_Trigger_Touch,3 }},
                                                                            {{DigitalButton_Grip_Click, 5},
                                                                                    { DigitalButton_Grip_Click,5 }},
                                                                            {{DigitalButton_Grip_Touch, 6},
                                                                                    { DigitalButton_Grip_Touch,6 }},
                                                                            {{DigitalButton_Joystick_Click, 8},
                                                                                    { DigitalButton_Joystick_Click,8 }},
                                                                            {{DigitalButton_Joystick_Touch, 9},
                                                                                    { DigitalButton_Joystick_Touch,9 }},
                                                                            {{DigitalButton_A_Click, 14},
                                                                                    { DigitalButton_A_Click,12 }},
                                                                            {{DigitalButton_B_Click, 15},
                                                                                    { DigitalButton_B_Click,13 }},
                                                                            {{DigitalButton_A_Touch, 18},
                                                                                    { DigitalButton_A_Touch,16 }},
                                                                            { {DigitalButton_B_Touch, 19},
                                                                                    { DigitalButton_B_Touch,17 }},
                                                                            { {DigitalButton_Touchpad_Touch, 20},
                                                                                    { DigitalButton_Touchpad_Touch, 20 }}
};

static AnalogAxisToCloudXRMap analog_axis_maps[] = {{AnalogAxis_Trigger, 4},
                                                    {AnalogAxis_Grip, 7},
                                                    {AnalogAxis_JoystickX, 10},
                                                    {AnalogAxis_JoystickY,11 }};


const char *cxr_input_paths[] =
{
        "/input/system/click",
        "/input/application_menu/click",
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
        "/input/x/click",
        "/input/y/click",
        "/input/a/touch",
        "/input/b/touch",
        "/input/x/touch",
        "/input/y/touch",
        "/input/thumb_rest/touch",
};

cxrInputValueType cxr_input_value_types[] =
{
        cxrInputValueType_boolean,  // input/system/click
        cxrInputValueType_boolean,  // input/application_menu/click
        cxrInputValueType_boolean,  // input/trigger/click
        cxrInputValueType_boolean,  // input/trigger/touch
        cxrInputValueType_float32,  // input/trigger/value
        cxrInputValueType_boolean,  // input/grip/click
        cxrInputValueType_boolean,  // input/grip/touch
        cxrInputValueType_float32,  // input/grip/value
        cxrInputValueType_boolean,  // input/joystick/click
        cxrInputValueType_boolean,  // input/joystick/touch
        cxrInputValueType_float32,  // input/joystick/x
        cxrInputValueType_float32,  // input/joystick/y
        cxrInputValueType_boolean,  // input/a/click
        cxrInputValueType_boolean,  // input/b/click
        cxrInputValueType_boolean,  // input/x/click
        cxrInputValueType_boolean,  // input/y/click
        cxrInputValueType_boolean,  // input/a/touch
        cxrInputValueType_boolean,  // input/b/touch
        cxrInputValueType_boolean,  // input/x/touch
        cxrInputValueType_boolean,  // input/y/touch
        cxrInputValueType_boolean,  // input/thumb_rest/touch
};


OKCloudClient::OKCloudClient()
{
}

OKCloudClient::~OKCloudClient()
{
}

bool OKCloudClient::init_android_gles(OKOpenXRInterface* xr_interface, EGLDisplay egl_display, EGLContext egl_context)
{
    if (!xr_interface || !egl_display || !egl_context)
    {
        return false;
    }

    if (is_cxr_initialized_)
    {
        return true;
    }

    ok_config_.load();

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::init_android_gles\n");

    xr_interface_ = xr_interface;

    graphics_context_.type = cxrGraphicsContext_GLES;
    graphics_context_.egl.display = (void *)egl_display;
    graphics_context_.egl.context = (void *)egl_context;

#if ENABLE_OBOE
    init_audio();
#endif

    const bool init_ok = create_receiver();

    is_cxr_initialized_ = init_ok;
    return init_ok;
}

void OKCloudClient::update_cxr_state(cxrClientState state, cxrError error)
{
    switch (state)
    {
        case cxrClientState_ReadyToConnect:
            //IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_ReadyToConnect");
            break;
        case cxrClientState_ConnectionAttemptInProgress:
            //IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_ConnectionAttemptInProgress");
            break;
        case cxrClientState_ConnectionAttemptFailed:
            //IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_ConnectionAttemptFailed");
            break;
        case cxrClientState_StreamingSessionInProgress:
            //IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_StreamingSessionInProgress");
            xr_interface_->handle_stream_connected();
            break;
        case cxrClientState_Disconnected:
        {
            //IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_Disconnected, setting back to cxrClientState_ReadyToConnect");
            cxr_client_state_ = cxrClientState_ReadyToConnect;
            xr_interface_->handle_stream_disconnected();
            break;
        }
        case cxrClientState_Exiting:
        {
            //IGLLog(IGLLogLevel::LOG_INFO, "CloudXR State = cxrClientState_Exiting, setting back to cxrClientState_ReadyToConnect");
            cxr_client_state_ = cxrClientState_ReadyToConnect;
            xr_interface_->handle_stream_disconnected();
            break;
        }
    }

    cxr_client_state_ = state;
}

void OKCloudClient::shutdown_cxr()
{
    if (!is_cxr_initialized_)
    {
        return;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::shutdown_cxr\n");

    disconnect();

#if ENABLE_OBOE
    shutdown_audio();
#endif

    is_cxr_initialized_ = false;
}

bool OKCloudClient::connect()
{
    if (!is_cxr_initialized_ || !is_ready_to_connect() || ok_config_.server_ip_address_.empty())
    {
        return false;
    }

    if (!cxr_receiver_)
    {
        create_receiver();

        if (!cxr_receiver_)
        {
            return false;
        }
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::connect to IP = %s\n", ok_config_.server_ip_address_.c_str());

    cxrConnectionDesc connection_desc = {0};
    connection_desc.async = true;
    connection_desc.useL4S = false;
    connection_desc.clientNetwork = cxrNetworkInterface_Unknown;
    connection_desc.topology = cxrNetworkTopology_LAN;

    cxrError error = cxrConnect(cxr_receiver_, ok_config_.server_ip_address_.c_str(), &connection_desc);

    if (error)
    {
        //IGLLog(IGLLogLevel::LOG_ERROR, "cxrConnect error = %s\n", cxrErrorString(error));
        return false;
    }

    return true;
}

void OKCloudClient::disconnect()
{
    if (!is_cxr_initialized_ || !is_connected() || !is_connecting())
    {
        return;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::disconnect\n");
    destroy_receiver();
}

bool OKCloudClient::create_receiver()
{
    if (cxr_receiver_  || !xr_interface_)
    {
        return false;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::create_receiver\n");

    // Set parameters here...
    receiver_desc_.requestedVersion = CLOUDXR_VERSION_DWORD;
    receiver_desc_.shareContext = &graphics_context_;

    // Debug flags
#if USE_ADVANCED_IMAGE_READER_DECODER
    receiver_desc_.debugFlags = cxrDebugFlags_EnableAImageReaderDecoder;
#else
    receiver_desc_.debugFlags = 0;
#endif
    
#if ENABLE_CLOUDXR_LOGGING
    {
#if ENABLE_CLOUDXR_LOGGING_VERBOSE
        receiver_desc_.debugFlags |= cxrDebugFlags_LogVerbose;
#endif
        std::string log_dir = ok_config_.app_directory_ + "logs/";
        strncpy(receiver_desc_.appOutputPath, log_dir.c_str(), CXR_MAX_PATH - 1);
        receiver_desc_.logMaxSizeKB = CLOUDXR_LOG_MAX_DEFAULT;
        receiver_desc_.logMaxAgeDays = CLOUDXR_LOG_MAX_DEFAULT;
        receiver_desc_.appOutputPath[CXR_MAX_PATH - 1] = 0;
    }
#endif

    cxrDeviceDesc& device_desc = receiver_desc_.deviceDesc;
    device_desc.maxResFactor = 1.0f;// ok_config_.max_res_factor_;

    compute_ipd();
    device_desc.ipd = ipd_meters_;
    device_desc.foveationModeCaps = cxrFoveation_PiecewiseQuadratic;

    const uint32_t number_of_streams = 2;
    device_desc.numVideoStreamDescs = number_of_streams;

    uint32_t per_eye_width = DEFAULT_CLOUDXR_PER_EYE_WIDTH;
    uint32_t per_eye_height = DEFAULT_CLOUDXR_PER_EYE_HEIGHT;

    float fps = DEFAULT_CLOUDXR_FRAMERATE;

#if 0
    float current_refresh_rate = xr_interface_->get_current_refresh_rate();
    bool should_update_refresh_rate = ((current_refresh_rate > 0.0f) && (current_refresh_rate != (float)ok_config_.desired_refresh_rate_));

    if (should_update_refresh_rate)
    {
        xr_interface_->query_refresh_rates();
        xr_interface_->set_refresh_rate((float)ok_config_.desired_refresh_rate_);
        fps = xr_interface_->get_current_refresh_rate();
    }

    const uint32_t integer_fps = (uint32_t)roundf(fps);

#if ENABLE_CLOUDXR_LINK_SHARPENING
    if (ok_config_.enable_sharpening_)
    {
        xr_interface_->set_sharpening_enabled(true);
    }
#endif
#endif

    for (uint32_t stream_index = 0; stream_index < number_of_streams; stream_index++)
    {
        device_desc.videoStreamDescs[stream_index].format = cxrClientSurfaceFormat_RGB;
        
        device_desc.videoStreamDescs[stream_index].width = ok_config_.per_eye_width_;
        device_desc.videoStreamDescs[stream_index].height = ok_config_.per_eye_height_;
        
        device_desc.videoStreamDescs[stream_index].fps = fps;// (float)integer_fps;
        device_desc.videoStreamDescs[stream_index].maxBitrate = 0.0f;//ok_config_.max_bitrate_kbps_;
    }

    device_desc.disableVVSync = false;
    device_desc.embedInfoInVideo = false;
    device_desc.foveatedScaleFactor = 0.0f;//ok_config_.foveation_;
    device_desc.stereoDisplay = true;

#if USE_FRAME_PERIOD_AS_POSE_PREDICTION_OFFSET
    const float prediction_offset_sec = (1.0f / fps);
#else
    const float prediction_offset_sec = DEFAULT_CLOUDXR_PREDICTION_OFFSET_NS * NS_TO_SEC;
#endif

    device_desc.predOffset = 0;//prediction_offset_sec;

    //const uint32_t polling_rate_hz = clamp<uint32_t>(ok_config_.polling_rate_mult_ * integer_fps, (uint32_t)MIN_CLOUDXR_POSE_POLLING_HZ, (uint32_t)MAX_CLOUDXR_POSE_POLLING_HZ);
    device_desc.posePollFreq = 0;// polling_rate_hz;

#if ENABLE_OBOE
    device_desc.receiveAudio = ok_config_.enable_audio_playback_;
    device_desc.sendAudio = ok_config_.enable_audio_recording_;
#else
    device_desc.receiveAudio = false;
    device_desc.sendAudio = false;
#endif

    device_desc.disablePosePrediction = false;//!ENABLE_CLOUDXR_POSE_PREDICTION;
    device_desc.angularVelocityInDeviceSpace = false;// ANGULAR_VELOCITY_IN_DEVICE_SPACE;
    
#if USE_OK_CALLBACKS
    {
        cxrClientCallbacks& receiver_callbacks = receiver_desc_.clientCallbacks;
        receiver_callbacks.clientContext = this;

        receiver_callbacks.UpdateClientState = [](void *context, cxrClientState cxr_client_state, cxrError error) {
            reinterpret_cast<OKCloudClient*>(context)->update_cxr_state(cxr_client_state, error);
        };


        receiver_callbacks.GetTrackingState = [](void *context, cxrVRTrackingState* cxr_tracking_state_ptr) {
            reinterpret_cast<OKCloudClient*>(context)->get_tracking_state(cxr_tracking_state_ptr);
        };

#if ENABLE_OBOE
        receiver_callbacks.RenderAudio = [](void *context, const cxrAudioFrame *audioFrame) {
            return reinterpret_cast<OKCloudClient*>(context)->render_audio(audioFrame);
        };
#endif

#if ENABLE_HAPTICS
        receiver_callbacks.TriggerHaptic = [](void *context, const cxrHapticFeedback* haptics) {
            reinterpret_cast<OKCloudClient*>(context)->trigger_haptics(haptics);
        };
#endif
    }
#endif

    {
        // Default Chaperone
        device_desc.chaperone.universe = cxrUniverseOrigin_Standing;

        device_desc.chaperone.origin.m[0][0] = 1.0f;
        device_desc.chaperone.origin.m[0][1] = 0.0f;
        device_desc.chaperone.origin.m[0][2] = 0.0f;
        device_desc.chaperone.origin.m[0][3] = 0.0f;

        device_desc.chaperone.origin.m[1][0] = 0.0f;
        device_desc.chaperone.origin.m[1][1] = 1.0f;
        device_desc.chaperone.origin.m[1][2] = 0.0f;
        device_desc.chaperone.origin.m[1][3] = 0.0f;

        device_desc.chaperone.origin.m[2][0] = 0.0f;
        device_desc.chaperone.origin.m[2][1] = 0.0f;
        device_desc.chaperone.origin.m[2][2] = 1.0f;
        device_desc.chaperone.origin.m[2][3] = 0.0f;

        device_desc.chaperone.playArea.v[0] = 1.5f;
        device_desc.chaperone.playArea.v[1] = 1.5f;
    }

    {
        //FOV
        for (int view_id = LEFT_EYE; view_id < NUM_EYES; view_id++)
        {
            const XrView view = xr_interface_->get_view(view_id);
            device_desc.proj[view_id][0] = tanf(view.fov.angleLeft);
            device_desc.proj[view_id][1] = tanf(view.fov.angleRight);
            device_desc.proj[view_id][2] = tanf(view.fov.angleDown);
            device_desc.proj[view_id][3] = tanf(view.fov.angleUp);
        }
    }

    cxrError error = cxrCreateReceiver(&receiver_desc_, &cxr_receiver_);

    if (error)
    {
        //IGLLog(IGLLogLevel::LOG_ERROR, "cxrCreateReceiver error = %s\n", cxrErrorString(error));
        return false;
    }

    return true;
}

void OKCloudClient::destroy_receiver()
{
    if (!is_cxr_initialized_ || !cxr_receiver_)
    {
        return;
    }
    
    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudSession::destroy_receiver\n");

#if ENABLE_OBOE
    shutdown_audio();
#endif

#if ENABLE_CLOUDXR_CONTROLLERS
    remove_controllers();
#endif

    cxrDestroyReceiver(cxr_receiver_);
    cxr_receiver_ = nullptr;
    receiver_desc_ = {0};

#if USE_CLOUDXR_POSE_ID
    poseID_ = 0;
#endif

    update_cxr_state(cxrClientState_Disconnected, cxrError_Success);
}

void OKCloudClient::compute_ipd()
{
    if (!xr_interface_)
    {
        return;
    }

    const XrView left_view = xr_interface_->get_view(LEFT_EYE);
    const XrView right_view = xr_interface_->get_view(RIGHT_EYE);

    const XrPosef& left_eye_pose = left_view.pose;
    const XrPosef& right_eye_pose = right_view.pose;

    const XrVector3f delta = {(right_eye_pose.position.x - left_eye_pose.position.x),
                              (right_eye_pose.position.y - left_eye_pose.position.y),
                              (right_eye_pose.position.z - left_eye_pose.position.z)};

    float ipd = sqrtf((delta.x * delta.x) + (delta.y * delta.y) + (delta.z * delta.z));
    ipd_meters_ = roundf(ipd * 10000.0f) / 10000.0f;

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::compute_ipd IPP =  %.7f meters (%.03f mm)\n", ipd_meters_, ipd_meters_ * MILLIMETERS_PER_METER);
}

#if ENABLE_CLOUDXR_CONTROLLERS
bool OKCloudClient::add_controllers()
{
    if (!is_connected())
    {
        return false;
    }

    if (controllers_initialized_)
    {
        return true;
    }

    for (int controller_id = LEFT_CONTROLLER; controller_id < CXR_NUM_CONTROLLERS; controller_id++)
    {
        //assert(cxr_controller_handles_[controller_id] == nullptr);

        //if (cxr_controller_handles_[controller_id] == nullptr)
        {
            cxrControllerDesc cxr_controller_desc = {};
            cxr_controller_desc.id = controller_id;
            cxr_controller_desc.role = (controller_id == LEFT_CONTROLLER) ? "cxr://input/hand/left" : "cxr://input/hand/right";
            cxr_controller_desc.controllerName = "Oculus Touch";
            cxr_controller_desc.inputCount = ARRAY_SIZE(cxr_input_paths);
            cxr_controller_desc.inputPaths = cxr_input_paths;
            cxr_controller_desc.inputValueTypes = cxr_input_value_types;

            cxrError add_controller_error = cxrAddController(cxr_receiver_,
                                                             &cxr_controller_desc,
                                                             &cxr_controller_handles_[controller_id]);

            if (add_controller_error)
            {
                //IGLLog(IGLLogLevel::LOG_ERROR, "cxrAddController error = %s\n",
                 //      cxrErrorString(add_controller_error));

                return false;
            }
        }
    }

    controllers_initialized_ = true;
    return true;
}

void OKCloudClient::remove_controllers()
{
    if (!controllers_initialized_)
    {
        return;
    }

    for (int controller_id = LEFT_CONTROLLER; controller_id < CXR_NUM_CONTROLLERS; controller_id++)
    {
        //assert(cxr_controller_handles_[controller_id] != nullptr);

        if (cxr_controller_handles_[controller_id] != nullptr)
        {
            if (is_connected())
            {
                cxrError remove_controller_error = cxrRemoveController(cxr_receiver_,
                                                                       cxr_controller_handles_[controller_id]);

                if (remove_controller_error)
                {
                    //IGLLog(IGLLogLevel::LOG_ERROR, "cxrRemoveController error = %s\n",
                    //       cxrErrorString(remove_controller_error));
                }
            }

            cxr_controller_handles_[controller_id] = nullptr;
        }
    }

    controllers_initialized_ = false;
}
#endif

bool OKCloudClient::latch_frame()
{
    if (!is_cxr_initialized_ || !is_connected() || is_latched_)
    {
        return false;
    }

    cxrError error = cxrLatchFrame(cxr_receiver_, &latched_frames_, cxrFrameMask_All, ok_config_.latch_timeout_ms_);

    if (error)
    {
        const bool is_real_error = (error != cxrError_Frame_Not_Ready);
        const bool log_error = (is_real_error || CLOUDXR_LOG_FRAME_NOT_READY_EVENT);

        if (log_error)
        {
            //IGLLog(IGLLogLevel::LOG_ERROR, "OKCloudClient::latch_frame cxrLatchFrame error = %s\n", cxrErrorString(error));
        }

        return false;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::latch_frame SUCCESS\n");
    is_latched_ = true;

    return true;
}

bool OKCloudClient::blit_frame(const int view_id, GLMPose& eye_pose)
{
    if (!is_connected())
    {
        return false;
    }

    if (!is_latched_)
    {
        //IGLLog(IGLLogLevel::LOG_WARNING, "OKCloudClient::blit_frame NOT LATCHED, skipping\n");
        return false;
    }

    const bool is_left_eye = (view_id == LEFT_EYE);

    uint32_t frame_mask = is_left_eye ? cxrFrameMask_Left : cxrFrameMask_Right;
    cxrError blit_error = cxrBlitFrame(cxr_receiver_, &latched_frames_, frame_mask);

    if (blit_error)
    {
        //IGLLog(IGLLogLevel::LOG_ERROR, "OKCloudClient::blit_frame cxrBlitFrame error = %s\n", cxrErrorString(blit_error));
        return false;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::blit_frame SUCCESS\n");

    cxrVector3 cxr_hmd_position = {};
    cxrQuaternion cxr_hmd_rotation = {};
    cxrMatrixToVecQuat(&latched_frames_.poseMatrix, &cxr_hmd_position, &cxr_hmd_rotation);

    XrVector3f xr_hmd_position = convert_cxr_to_xr(cxr_hmd_position);
    XrQuaternionf xr_hmd_rotation = convert_cxr_to_xr(cxr_hmd_rotation);

    const GLMPose hmd_pose(convert_to_glm(xr_hmd_position), convert_to_glm(xr_hmd_rotation));
    eye_pose = hmd_pose;

    const float half_ipd = ipd_meters_ * 0.5f;
    const float ipd_offset = is_left_eye ? -half_ipd : half_ipd;
    const glm::vec3 ipd_offset_vec = glm::vec3(ipd_offset, 0.0f, 0.0f);
    eye_pose.translation_ += hmd_pose.rotation_ * ipd_offset_vec;

    return true;
}

void OKCloudClient::release_frame()
{
    if (!is_cxr_initialized_ || !is_connected() || !is_latched_)
    {
        return;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::release_frame\n");
    cxrReleaseFrame(cxr_receiver_, &latched_frames_);
    is_latched_ = false;
}

void OKCloudClient::get_tracking_state(cxrVRTrackingState* cxr_tracking_state_ptr)
{
    if (!is_cxr_initialized_ || !is_connected() || !cxr_tracking_state_ptr || !xr_interface_)
    {
        return;
    }

    OKOpenXRControllerActions& ok_inputs = xr_interface_->get_actions();

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::get_tracking_state\n");

    const uint64_t predicted_display_time_ns = xr_interface_->get_predicted_display_time_ns() + ok_config_.prediction_offset_ns_;

    cxrVRTrackingState& cxr_tracking_state = *cxr_tracking_state_ptr;
    memset(cxr_tracking_state_ptr, 0, sizeof(*cxr_tracking_state_ptr));

    cxr_tracking_state.poseTimeOffset = ok_config_.pose_time_offset_s_;

#if ENABLE_CLOUDXR_CONTROLLERS
    add_controllers();

    if (controllers_initialized_)
    {
        // Poll from async thread, possibly much higher Hz (up to 1 Khz) than main render thread (to reduce latency)
        xr_interface_->poll_actions(false);

        for (int controller_id = LEFT_CONTROLLER; controller_id < CXR_NUM_CONTROLLERS; controller_id++)
        {
            XrActionStateGetInfo action_info = {XR_TYPE_ACTION_STATE_GET_INFO};
            XrActionStatePose pose_state = {XR_TYPE_ACTION_STATE_POSE};

            action_info.subactionPath = ok_inputs.handSubactionPath[controller_id];
            action_info.action = ok_inputs.aimPoseAction;

            XrResult result = xrGetActionStatePose(xr_interface_->get_session(), &action_info,  &pose_state);

            cxrControllerTrackingState& cxr_controller = cxr_tracking_state.controller[controller_id];
            cxr_controller = {};

            OKController& ok_controller = ok_player_state_.controllers_[controller_id];
            ok_controller.pose_.is_valid_ = false;

            if (XR_UNQUALIFIED_SUCCESS(result) && pose_state.isActive)
            {
                XrSpaceVelocity controller_velocity = {XR_TYPE_SPACE_VELOCITY};
                XrSpaceLocation controller_location = {XR_TYPE_SPACE_LOCATION,
                                                       &controller_velocity};

                XrResult controller_result =
                        xrLocateSpace(ok_inputs.aimSpace[controller_id], xr_interface_->get_base_space(),
                                      predicted_display_time_ns, &controller_location);

                if (controller_result == XR_SUCCESS)
                {
                    cxr_controller.clientTimeNS = predicted_display_time_ns;
                    ok_controller.pose_ = convert_to_glm_pose(controller_location.pose);

                    if (ok_config_.enable_remote_controller_offset_)
                    {
                        GLMPose cloudxr_controller_offset = ok_config_.remote_controller_offset_;

                        if (controller_id == LEFT_CONTROLLER)
                        {
                            cloudxr_controller_offset.translation_.x *= -1.0f;
                        }

                        const glm::vec3 offset_ws = ok_controller.pose_.rotation_ * cloudxr_controller_offset.translation_;
                        ok_controller.pose_.translation_ += offset_ws;
                        ok_controller.pose_.rotation_ = glm::normalize(ok_controller.pose_.rotation_ * cloudxr_controller_offset.rotation_);

                    }

                    cxr_controller.pose = convert_glm_to_cxr_pose(ok_controller.pose_);

                    update_controller_digital_buttons(controller_id);
                    update_controller_analog_axes(controller_id);

                    //send_controller_poses(cxr_controller, controller_id, predicted_display_time_ns);

                    fire_controller_events(controller_id, predicted_display_time_ns);
                }
            }
        }
    }
#endif

#if ENABLE_CLOUDXR_HMD
    {
        cxr_tracking_state.hmd.flags = 0;

#if RECOMPUTE_IPD_EVERY_FRAME
        compute_ipd();
        cxr_tracking_state.hmd.flags |= cxrHmdTrackingFlags_HasIPD;
        cxr_tracking_state.hmd.ipd = ipd_meters_;
#endif

#if USE_CLOUDXR_POSE_ID
        cxr_tracking_state.hmd.flags |= cxrHmdTrackingFlags_HasPoseID;
        cxr_tracking_state.hmd.poseID = poseID_++;
#endif

        cxrTrackedDevicePose& cxr_hmd_pose = cxr_tracking_state.hmd.pose;
        cxr_hmd_pose = {};

        XrSpaceVelocity hmd_velocity = {XR_TYPE_SPACE_VELOCITY};
        XrSpaceLocation hmd_location = {XR_TYPE_SPACE_LOCATION, &hmd_velocity};

        XrResult hmd_result = xrLocateSpace(xr_interface_->get_head_space(), xr_interface_->get_base_space(), predicted_display_time_ns, &hmd_location);

        if (XR_UNQUALIFIED_SUCCESS(hmd_result))
        {
            cxr_hmd_pose = convert_xr_to_cxr_pose(hmd_location);
            cxr_tracking_state.hmd.clientTimeNS = predicted_display_time_ns;
            cxr_tracking_state.hmd.activityLevel = cxrDeviceActivityLevel_UserInteraction;
        }
    }
#endif

}

#if ENABLE_CLOUDXR_CONTROLLERS
void OKCloudClient::send_controller_poses(cxrControllerTrackingState& cxr_controller, const int controller_id, const uint64_t predicted_display_time_ns)
{
    if (!is_cxr_initialized_ || !is_connected() || !controllers_initialized_ || !xr_interface_)
    {
        return;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::send_controller_poses\n");

    {
        const uint32_t pose_count = 1;
        const cxrControllerTrackingState* controller_ptr = &cxr_controller;
        const cxrControllerTrackingState ** controller_states_ptr = &controller_ptr;

        cxrError send_controller_pose_result = cxrSendControllerPoses(cxr_receiver_, pose_count, &cxr_controller_handles_[controller_id], controller_states_ptr);

        if (send_controller_pose_result)
        {
            //IGLLog(IGLLogLevel::LOG_ERROR, "cxrSendControllerPoses error = %s\n",
             //      cxrErrorString(send_controller_pose_result));
        }
    }
}

void OKCloudClient::fire_controller_events(const int controller_id, const uint64_t predicted_display_time_ns)
{
    if (!is_cxr_initialized_ || !is_connected() || !controllers_initialized_)
    {
        return;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::fire_controller_events\n");

    cxrControllerEvent cxr_events[MAX_CLOUDXR_CONTROLLER_EVENTS] = {};
    uint32_t cxr_event_count = 0;

    const OKController& ok_controller = ok_player_state_.controllers_[controller_id];

    {
        const uint32_t num_analog_axis_maps = ARRAY_SIZE(analog_axis_maps);

        for (uint32_t map_id = 0; map_id < num_analog_axis_maps; map_id++)
        {
            const AnalogAxisToCloudXRMap &analog_axis_map = analog_axis_maps[map_id];

            if (analog_axis_map.cloudxr_path_id_ == INVALID_INDEX)
            {
                continue;
            }

            const OKAnalogAxis& ok_analog_axis = ok_controller.analog_axes_[analog_axis_map.analog_axis_id_];
            const bool was_changed = send_all_analog_controller_values_ || ok_analog_axis.was_value_changed();

            if (was_changed)
            {
                const float analog_axis_value = ok_analog_axis.get_current_value();

                cxrControllerEvent &event = cxr_events[cxr_event_count++];
                event.clientTimeNS = predicted_display_time_ns;
                event.clientInputIndex = analog_axis_map.cloudxr_path_id_;
                event.inputValue.valueType = cxrInputValueType_float32;
                event.inputValue.vF32 = analog_axis_value;
            }
        }
    }

    {
        const uint32_t num_digital_button_maps = ARRAY_SIZE(digital_button_maps);

        for (uint32_t map_id = 0; map_id < num_digital_button_maps; map_id++)
        {
            const DigitalButtonToCloudXR_Map& digital_button_map = digital_button_maps[map_id][controller_id];

            if (digital_button_map.cloudxr_path_id_ == INVALID_INDEX)
            {
                continue;
            }

            const OKDigitalButton& ok_digital_button = ok_controller.digital_buttons_[digital_button_map.digital_button_id_];

            const bool is_down = ok_digital_button.is_down();
            const bool was_changed = send_all_digital_controller_values_ || ok_digital_button.was_changed();

            if (was_changed)
            {
                cxrControllerEvent& event = cxr_events[cxr_event_count++];
                event.clientTimeNS = predicted_display_time_ns;
                event.clientInputIndex = digital_button_map.cloudxr_path_id_;
                event.inputValue.valueType = cxrInputValueType_boolean;
                event.inputValue.vBool = is_down;
            }
        }
    }

    if (cxr_event_count > 0)
    {
        cxrError fire_controller_events_result = cxrFireControllerEvents(cxr_receiver_, cxr_controller_handles_[controller_id], cxr_events, cxr_event_count);

        if (fire_controller_events_result)
        {
            //IGLLog(IGLLogLevel::LOG_ERROR, "cxrFireControllerEvents error = %s\n",
//                   cxrErrorString(fire_controller_events_result));
        }
    }
}

void OKCloudClient::update_controller_digital_buttons(const int controller_id)
{
    if (!is_cxr_initialized_ || !is_connected() || !xr_interface_)
    {
        return;
    }

    OKOpenXRControllerActions& ok_inputs = xr_interface_->get_actions();

    struct XRActionToDigitalButtonID_Mapping
    {
        XrAction action;
        DigitalButtonID digital_button_id;
    };

    XRActionToDigitalButtonID_Mapping xr_to_ok_button_mappings[] =
            {
                    {ok_inputs.menuClickAction, DigitalButton_ApplicationMenu},
                    {ok_inputs.triggerTouchAction, DigitalButton_Trigger_Touch},
                    {ok_inputs.triggerClickAction, DigitalButton_Trigger_Click},
                    //{ok_inputs.squeezeTouchAction, DigitalButton_Grip_Touch},
                    {ok_inputs.squeezeClickAction, DigitalButton_Grip_Click},
                    {ok_inputs.thumbstickTouchAction, DigitalButton_Joystick_Touch},
                    {ok_inputs.thumbstickClickAction, DigitalButton_Joystick_Click},
                    //{ok_inputs.thumbRestTouchAction, DigitalButton_Touchpad_Touch},
                    //{ok_inputs.thumbRestClickAction, DigitalButton_Touchpad_Click},
                    {ok_inputs.buttonAXTouchAction, DigitalButton_A_Touch},
                    {ok_inputs.buttonAXClickAction, DigitalButton_A_Click},
                    {ok_inputs.buttonBYTouchAction, DigitalButton_B_Touch},
                    {ok_inputs.buttonBYClickAction, DigitalButton_B_Click}
            };

    OKController& ok_controller = ok_player_state_.controllers_[controller_id];

    XrActionStateGetInfo action_info = {XR_TYPE_ACTION_STATE_GET_INFO};
    action_info.subactionPath = ok_inputs.handSubactionPath[controller_id];

    XrActionStateBoolean button_state = {XR_TYPE_ACTION_STATE_BOOLEAN};

    for (uint32_t j = 0; j < ARRAY_SIZE(xr_to_ok_button_mappings); j++)
    {
        action_info.action = xr_to_ok_button_mappings[j].action;
        XrResult action_result = xrGetActionStateBoolean(xr_interface_->get_session(), &action_info, &button_state);

        if ((action_result != XR_SUCCESS) || !button_state.isActive)
        {
            continue;
        }

        OKDigitalButton& ok_digital_button = ok_controller.digital_buttons_[xr_to_ok_button_mappings[j].digital_button_id];
        ok_digital_button.set_state(button_state.currentState);
    }

    if (simulate_grip_touch_)
    {
        const OKAnalogAxis& grip_analog_axis = ok_controller.analog_axes_[AnalogAxis_Grip];
        OKDigitalButton& group_touch_button = ok_controller.digital_buttons_[DigitalButton_Grip_Touch];
        group_touch_button.set_state(grip_analog_axis.get_current_value() > 0.0f);
    }

    if (simulate_thumb_rest_)
    {
        const OKAnalogAxis& grip_force_analog_axis = ok_controller.analog_axes_[AnalogAxis_Grip_Force];
        OKDigitalButton& touchpad_touch_button = ok_controller.digital_buttons_[DigitalButton_Touchpad_Touch];
        touchpad_touch_button.set_state(grip_force_analog_axis.get_current_value() > 0.0f);
    }
}

void OKCloudClient::update_controller_analog_axes(const int controller_id)
{
    if (!is_cxr_initialized_ || !is_connected() || !xr_interface_)
    {
        return;
    }

    OKOpenXRControllerActions& ok_inputs = xr_interface_->get_actions();

    struct XRActionToAnalogAxisID_Mapping
    {
        XrAction action;
        AnalogAxisID analog_axis_id;
    };

    XRActionToAnalogAxisID_Mapping xr_to_ok_analog_mappings[] =
            {
                    {ok_inputs.triggerValueAction, AnalogAxis_Trigger},
                    {ok_inputs.squeezeValueAction, AnalogAxis_Grip},
                    {ok_inputs.thumbstickXAction, AnalogAxis_JoystickX},
                    {ok_inputs.thumbstickYAction, AnalogAxis_JoystickY},
                    {ok_inputs.thumbProximityAction, AnalogAxis_Proximity},
                    {ok_inputs.thumbRestForceAction, AnalogAxis_Grip_Force},
                    //{ok_inputs.trackpadXAction, AnalogAxis_JoystickX},
                    //{ok_inputs.trackpadYAction, AnalogAxis_JoystickY}
            };

    OKController& ok_controller = ok_player_state_.controllers_[controller_id];

    XrActionStateGetInfo action_info = {XR_TYPE_ACTION_STATE_GET_INFO};
    action_info.subactionPath = ok_inputs.handSubactionPath[controller_id];
    XrActionStateFloat axis_state = {XR_TYPE_ACTION_STATE_FLOAT};

    for (uint32_t j = 0; j < ARRAY_SIZE(xr_to_ok_analog_mappings); j++)
    {
        action_info.action = xr_to_ok_analog_mappings[j].action;
        XrResult action_result = xrGetActionStateFloat(xr_interface_->get_session(), &action_info, &axis_state);

        if ((action_result != XR_SUCCESS) || !axis_state.isActive)
        {
            continue;
        }

        OKAnalogAxis& ok_analog_axis = ok_controller.analog_axes_[xr_to_ok_analog_mappings[j].analog_axis_id];
        ok_analog_axis.set_value(axis_state.currentState);
    }

    if (combine_grip_force_with_grip_)
    {
        const OKAnalogAxis& grip_force_analog_axis = ok_controller.analog_axes_[AnalogAxis_Grip_Force];

        OKAnalogAxis& grip_analog_axis = ok_controller.analog_axes_[AnalogAxis_Grip];
        grip_analog_axis.combine(grip_force_analog_axis);
    }
}

#endif

#if ENABLE_HAPTICS
void OKCloudClient::trigger_haptics(const cxrHapticFeedback* haptics)
{
    if (!is_cxr_initialized_ || !is_connected() || !haptics)
    {
        return;
    }

    const int controller_id = haptics->deviceID;
    const float duration_ms = haptics->seconds * 1e9;

    //IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::trigger_haptics\n");

    //apply_haptics(controller_id, haptics->amplitude, duration_ms, haptics->frequency);
}
#endif

#if ENABLE_OBOE
bool OKCloudClient::init_audio()
{
    if (!ok_config_.enable_audio_playback_ && !ok_config_.enable_audio_recording_)
    {
        return false;
    }

    if (is_audio_initialized_)
    {
        return true;
    }

    IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::init_audio\n");

    if (ok_config_.enable_audio_playback_)
    {
        // Playback Stream
        oboe::AudioStreamBuilder audio_output_stream_builder = {};
        audio_output_stream_builder.setDirection(oboe::Direction::Output);

        audio_output_stream_builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        //audio_output_stream_builder.setPerformanceMode(oboe::PerformanceMode::None);

        audio_output_stream_builder.setSharingMode(oboe::SharingMode::Exclusive);
        audio_output_stream_builder.setFormat(oboe::AudioFormat::I16);
        audio_output_stream_builder.setChannelCount(oboe::ChannelCount::Stereo);
        audio_output_stream_builder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);

        oboe::Result playback_stream_result = audio_output_stream_builder.openStream(audio_playback_stream_);

        if (playback_stream_result != oboe::Result::OK)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "openStream playback error = %s\n", oboe::convertToText(playback_stream_result));
            return false;
        }

        int buffer_size = audio_playback_stream_->getFramesPerBurst() * CXR_AUDIO_CHANNEL_COUNT;
        oboe::Result set_buffer_size_result = audio_playback_stream_->setBufferSizeInFrames(buffer_size);

        if (set_buffer_size_result != oboe::Result::OK)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "setBufferSizeInFrames playback error = %s\n", oboe::convertToText(set_buffer_size_result));
            return false;
        }

        oboe::Result start_playback_result = audio_playback_stream_->start();

        if (start_playback_result != oboe::Result::OK)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "start audio playback error = %s\n", oboe::convertToText(start_playback_result));
            return false;
        }
    }

    if (ok_config_.enable_audio_recording_)
    {
        // Capture Stream
        oboe::AudioStreamBuilder audio_capture_stream_builder = {};
        audio_capture_stream_builder.setDirection(oboe::Direction::Input);

        //audio_capture_stream_builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        audio_capture_stream_builder.setPerformanceMode(oboe::PerformanceMode::None);

        audio_capture_stream_builder.setSharingMode(oboe::SharingMode::Exclusive);
        audio_capture_stream_builder.setFormat(oboe::AudioFormat::I16);
        audio_capture_stream_builder.setChannelCount(oboe::ChannelCount::Stereo);
        audio_capture_stream_builder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);

        audio_capture_stream_builder.setDirection(oboe::Direction::Input);
        audio_capture_stream_builder.setPerformanceMode(oboe::PerformanceMode::None);
        audio_capture_stream_builder.setSharingMode(oboe::SharingMode::Exclusive);
        audio_capture_stream_builder.setFormat(oboe::AudioFormat::I16);
        audio_capture_stream_builder.setChannelCount(oboe::ChannelCount::Stereo);
        audio_capture_stream_builder.setSampleRate(CXR_AUDIO_SAMPLING_RATE);
        audio_capture_stream_builder.setInputPreset(oboe::InputPreset::VoiceCommunication);
        audio_capture_stream_builder.setDataCallback(this);

        oboe::Result capture_stream_result = audio_capture_stream_builder.openStream(audio_record_stream_);

        if (capture_stream_result != oboe::Result::OK)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "openStream record error = %s\n", oboe::convertToText(capture_stream_result));
            return false;
        }

        int buffer_size = audio_playback_stream_->getFramesPerBurst() * 2;
        oboe::Result set_buffer_size_result = audio_playback_stream_->setBufferSizeInFrames(buffer_size);

        if (set_buffer_size_result != oboe::Result::OK)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "setBufferSizeInFrames record error = %s\n", oboe::convertToText(set_buffer_size_result));
            return false;
        }

        oboe::Result start_record_result = audio_record_stream_->start();

        if (start_record_result != oboe::Result::OK)
        {
            IGLLog(IGLLogLevel::LOG_ERROR, "start audio record error = %s\n", oboe::convertToText(start_record_result));
            return false;
        }
    }

    is_audio_initialized_ = true;
    return true;
}

void OKCloudClient::shutdown_audio()
{
    if (!is_audio_initialized_)
    {
        return;
    }

    IGLLog(IGLLogLevel::LOG_INFO, "OKCloudClient::shutdown_audio\n");

    if (audio_playback_stream_)
    {
        audio_playback_stream_->close();
    }

    if (audio_record_stream_)
    {
        audio_record_stream_->close();
    }

    is_audio_initialized_ = false;
}

cxrBool OKCloudClient::render_audio(const cxrAudioFrame* audio_frame)
{
    if (!audio_frame || !is_audio_initialized_ || !ok_config_.enable_audio_playback_ || !audio_playback_stream_)
    {
        return cxrFalse;
    }

    uint32_t timeout = audio_frame->streamSizeBytes / CXR_AUDIO_BYTES_PER_MS;
    uint32_t frame_count = timeout * CXR_AUDIO_SAMPLING_RATE / 1000;

    oboe::ResultWithValue write_result =
            audio_playback_stream_->write(audio_frame->streamBuffer, frame_count, timeout * oboe::kNanosPerMillisecond);

    if (!write_result)
    {
        IGLLog(IGLLogLevel::LOG_ERROR, "Error rendering audio: %s", oboe::convertToText(write_result.error()));

        if (write_result.error() == oboe::Result::ErrorDisconnected)
        {
            shutdown_audio();
            init_audio();
        }
    }

    return cxrTrue;
}

oboe::DataCallbackResult OKCloudClient::onAudioReady(oboe::AudioStream* audio_stream, void *data, int32_t frame_count)
{
    if (is_connected())
    {
        cxrAudioFrame audio_frame = {};
        audio_frame.streamBuffer = (int16_t *)data;
        audio_frame.streamSizeBytes = frame_count * CXR_AUDIO_CHANNEL_COUNT * CXR_AUDIO_SAMPLE_SIZE;
        cxrSendAudio(cxr_receiver_, &audio_frame);
    }

    return oboe::DataCallbackResult::Continue;
}
#endif // ENABLE_OBOE

}  // namespace BVR


#endif // ENABLE_CLOUDXR


