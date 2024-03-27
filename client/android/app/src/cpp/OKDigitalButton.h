//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_DIGITAL_BUTTON_H
#define OK_DIGITAL_BUTTON_H

#include <chrono>

namespace BVR 
{

class OKDigitalButton 
{
	public:
		OKDigitalButton() 
		{
		};
		~OKDigitalButton(){};

		bool is_down() const;
		bool was_pressed() const;
		bool was_released() const;
		bool was_changed() const;
		bool is_active() const;

		void clear();
		void set_state(const bool down);

		uint get_pressed_count() const;
		uint get_released_count() const;
		uint get_new_pressed_count();
		uint get_new_released_count();

		float get_held_duration_ms() const;
		float get_released_duration_ms() const;

		void combine(const OKDigitalButton& other);

	private:
		bool down_ = false;
		bool pressed_ = false;
		bool released_ = false;

		uint pressed_count_ = 0;
		uint previous_pressed_count_ = 0;

		uint released_count_ = 0;
		uint previous_released_count_ = 0;

		std::chrono::time_point<std::chrono::high_resolution_clock> pressed_timestamp_;
		std::chrono::time_point<std::chrono::high_resolution_clock> released_timestamp_;
};

}  // namespace BVR

#endif  // OK_DIGITAL_BUTTON_H

