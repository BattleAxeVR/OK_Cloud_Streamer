//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CONTROLLER_H
#define OK_CONTROLLER_H

#include "OKDigitalButton.h"
#include "OKAnalogAxis.h"
#include "GLMPose.h"

namespace BVR 
{
const int NUM_CONTROLLERS = 2;
const int LEFT_CONTROLLER = 0;
const int RIGHT_CONTROLLER = 1;

struct OKOpenXRControllerActions
{
    XrPath handSubactionPath[NUM_CONTROLLERS] = {};

    XrSpace gripSpace[NUM_CONTROLLERS] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrSpace aimSpace[NUM_CONTROLLERS] = {XR_NULL_HANDLE, XR_NULL_HANDLE};

    XrActionSet actionSet{XR_NULL_HANDLE};
    XrAction grabAction{XR_NULL_HANDLE};
    XrAction vibrateAction{XR_NULL_HANDLE};

    XrAction gripPoseAction{ XR_NULL_HANDLE };
    XrAction aimPoseAction{ XR_NULL_HANDLE };
    XrAction menuClickAction{ XR_NULL_HANDLE };

    XrAction triggerClickAction{ XR_NULL_HANDLE };
    XrAction triggerTouchAction{ XR_NULL_HANDLE };
    XrAction triggerValueAction{ XR_NULL_HANDLE };

    XrAction squeezeClickAction{ XR_NULL_HANDLE };
    XrAction squeezeTouchAction{ XR_NULL_HANDLE };
    XrAction squeezeValueAction{ XR_NULL_HANDLE };
    // XrAction squeezeForceAction{ XR_NULL_HANDLE };

    XrAction thumbstickTouchAction{ XR_NULL_HANDLE };
    XrAction thumbstickClickAction{ XR_NULL_HANDLE };

    XrAction thumbstickXAction{ XR_NULL_HANDLE };
    XrAction thumbstickYAction{ XR_NULL_HANDLE };

    XrAction thumbRestTouchAction{ XR_NULL_HANDLE };
    XrAction thumbRestClickAction{ XR_NULL_HANDLE };
    XrAction thumbRestForceAction{ XR_NULL_HANDLE };
    XrAction thumbProximityAction{ XR_NULL_HANDLE };

    XrAction pinchValueAction{ XR_NULL_HANDLE };
    XrAction pinchForceAction{ XR_NULL_HANDLE };

    XrAction buttonAXClickAction{ XR_NULL_HANDLE };
    XrAction buttonAXTouchAction{ XR_NULL_HANDLE };

    XrAction buttonBYClickAction{ XR_NULL_HANDLE };
    XrAction buttonBYTouchAction{ XR_NULL_HANDLE };

    XrAction trackpadXAction{ XR_NULL_HANDLE };
    XrAction trackpadYAction{ XR_NULL_HANDLE };
};

class OKPlayerState;

typedef enum
{
    DigitalButton_System,
    DigitalButton_ApplicationMenu,

    DigitalButton_Trigger_Touch,
    DigitalButton_Trigger_Click,

    DigitalButton_Grip_Touch,
    DigitalButton_Grip_Click,

    DigitalButton_Touchpad_Touch,
    DigitalButton_Touchpad_Click,

    DigitalButton_Joystick_Touch,
    DigitalButton_Joystick_Click,

    DigitalButton_A_Touch,
    DigitalButton_A_Click,

    DigitalButton_B_Touch,
    DigitalButton_B_Click,

    DIGITAL_BUTTON_COUNT
} DigitalButtonID;

typedef enum
{
    AnalogAxis_Trigger,

    AnalogAxis_TouchpadX,
    AnalogAxis_TouchpadY,

    AnalogAxis_JoystickX,
    AnalogAxis_JoystickY,

    AnalogAxis_Grip,
    AnalogAxis_Grip_Force,

    AnalogAxis_Proximity,

    ANALOG_AXIS_COUNT
} AnalogAxisID;

struct DigitalButtonToCloudXR_Map
{
    DigitalButtonID digital_button_id_;
    int cloudxr_path_id_;
};

struct AnalogAxisToCloudXRMap
{
    AnalogAxisID analog_axis_id_;
    int cloudxr_path_id_;
};

class OKController 
{
	public:
		OKController(OKPlayerState& ok_player, const int controller_id);

        OKPlayerState& ok_player_;
		int controller_id_ = LEFT_CONTROLLER;

        GLMPose pose_;

        OKDigitalButton digital_buttons_[DIGITAL_BUTTON_COUNT];
        OKAnalogAxis analog_axes_[ANALOG_AXIS_COUNT];
};

}  // namespace BVR

#endif  // OK_CONTROLLER_H
