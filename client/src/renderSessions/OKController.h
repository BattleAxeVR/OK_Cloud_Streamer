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

