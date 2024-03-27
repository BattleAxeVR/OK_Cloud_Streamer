//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"
#include "OKAnalogAxis.h"
#include "GLMPose.h"
#include <algorithm>
#include <math.h>

namespace BVR
{
    
float OKAnalogAxis::get_current_value() const
{
	return current_value_;
}

float OKAnalogAxis::get_previous_value() const
{
	return previous_value_;
}

bool OKAnalogAxis::was_value_changed() const
{
    return (current_value_ != previous_value_);
    return (fabs(current_value_ - previous_value_) > FLT_EPSILON);
}

void OKAnalogAxis::set_value(const float value)
{
	previous_value_ = current_value_;
	current_value_ = clamp(value, MIN_ANALOG_AXIS_VALUE, MAX_ANALOG_AXIS_VALUE);

	const bool button_was_down = digital_button_.is_down();

	if (use_fast_release_) 
	{
		if (current_value_ >= pressed_threshold_) 
		{
			if (current_value_ > highest_value_) 
			{
				highest_value_ = current_value_;
			}

			if (!button_was_down) 
			{
				bool button_is_down = (current_value_ > (last_release_value_ + release_threshold_));
				
				if (button_is_down) 
				{
                    digital_button_.set_state(true);
					last_release_value_ = current_value_;
				} 
				else 
				{
                    digital_button_.set_state(false);
				}
			} 
			else if (button_was_down) 
			{
				bool button_is_up = current_value_ < (highest_value_ - release_threshold_);
				if (button_is_up) 
				{
                    digital_button_.set_state(false);
					last_release_value_ = current_value_;
					highest_value_ = current_value_;
				} 
				else 
				{
                    digital_button_.set_state(true);
				}
			}
		} 
		else if (button_was_down) 
		{
			bool button_is_up = current_value_ < (highest_value_ - release_threshold_);
			
			if (button_is_up) 
			{
                digital_button_.set_state(false);
				last_release_value_ = current_value_;
				highest_value_ = current_value_;
			} 
			else 
			{
                digital_button_.set_state(false);
			}
		} 
		else 
		{
			highest_value_ = 0.0f;
			last_release_value_ = 0.0f;
		}
	} 
	else 
	{
		const bool should_be_down = (current_value_ >= pressed_threshold_);

		if (button_was_down != should_be_down) 
		{
            digital_button_.set_state(should_be_down);
		}
	}
}

void OKAnalogAxis::add_value(const float value)
{
	if (value == 0.0f) 
	{
		return;
	}

	set_value(current_value_ + value);
}

void OKAnalogAxis::negate()
{
	current_value_ = -current_value_;
}

void OKAnalogAxis::clear()
{
	current_value_ = 0.0f;
	previous_value_ = 0.0f;
	highest_value_ = 0.0f;
	last_release_value_ = 0.0f;
    digital_button_.clear();
}

bool OKAnalogAxis::is_down() const
{
	return digital_button_.is_down();
}

bool OKAnalogAxis::was_pressed() const
{
	return digital_button_.was_pressed();
}

bool OKAnalogAxis::was_released() const
{
	return digital_button_.was_released();
}

bool OKAnalogAxis::is_active() const
{
	return (fabs(current_value_) > deadzone_);
}

void OKAnalogAxis::combine(const OKAnalogAxis& other)
{
	current_value_ = clamp(current_value_ + other.current_value_, MIN_ANALOG_AXIS_VALUE, MAX_ANALOG_AXIS_VALUE);
	previous_value_ = clamp(previous_value_ + other.previous_value_, MIN_ANALOG_AXIS_VALUE, MAX_ANALOG_AXIS_VALUE);
	highest_value_ = std::max(highest_value_, other.highest_value_);
	last_release_value_ = 0.0f;
	digital_button_.combine(other.digital_button_);
}

}  // namespace BVR



