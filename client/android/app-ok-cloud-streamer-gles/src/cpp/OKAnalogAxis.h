//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_ANALOG_AXIS_H
#define OK_ANALOG_AXIS_H

#include "OKDigitalButton.h"

namespace BVR 
{
const float MIN_ANALOG_AXIS_VALUE = -1.0f;
const float MAX_ANALOG_AXIS_VALUE = 1.0f;

class OKAnalogAxis
{
	public:
        OKAnalogAxis(){};
		~OKAnalogAxis(){};

		float get_current_value() const;
		float get_previous_value() const;
		bool was_value_changed() const;

		void set_value(const float value);
		void add_value(const float value);
		void negate();
		void clear();

		bool is_down() const;
		bool was_pressed() const;
		bool was_released() const;

		bool is_active() const;

		uint get_pressed_count() const 
		{
			return digital_button_.get_pressed_count();
		}

		uint get_released_count() const 
		{
			return digital_button_.get_released_count();
		}

		float get_held_duration_ms() const 
		{
			return digital_button_.get_held_duration_ms();
		}

		float get_released_duration_ms() const 
		{
			return digital_button_.get_released_duration_ms();
		}


		float get_deadzone() const 
		{
			return deadzone_;
		}

		void set_deadzone(const float deadzone) 
		{
			deadzone_ = deadzone;
		}

		void combine(const OKAnalogAxis& other);

	private:
		OKDigitalButton digital_button_;
	
		float deadzone_ = 0.0f;

		bool use_fast_release_ = true;

		float current_value_ = 0.0f;
		float previous_value_ = 0.0f;
		float highest_value_ = 0.0f;

		float last_release_value_ = 0.0f;

		float pressed_threshold_ = 0.05f;
		float release_threshold_ = 0.04f;
};

}  // namespace BVR

#endif  // OK_ANALOG_AXIS_H

