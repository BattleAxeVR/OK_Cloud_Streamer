//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CONTROLLER_H
#define OK_CONTROLLER_H

#include "OKDigitalButton.h"
#include "OKAnalogAxis.h"

namespace BVR 
{
    const int NUM_CONTROLLERS = 2;
    const int LEFT_CONTROLLER = 0;
    const int RIGHT_CONTROLLER = 1;

    struct OKPlayerState;

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

