//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"
#include "OKDigitalButton.h"
#include <math.h>
#include <algorithm>

namespace BVR
{
bool OKDigitalButton::is_down() const 
{
	return down_;
}

bool OKDigitalButton::was_pressed() const 
{
	return pressed_;
}

bool OKDigitalButton::was_released() const 
{
	return released_;
}

bool OKDigitalButton::was_changed() const
{
    return (pressed_ || released_);
}

bool OKDigitalButton::is_active() const 
{
	return (down_ || released_ || released_);
}

void OKDigitalButton::clear() 
{
	down_ = false;
	pressed_ = false;
	released_ = false;
	pressed_count_ = 0;
	previous_pressed_count_ = 0;
	released_count_ = 0;
	previous_released_count_ = 0;
	pressed_timestamp_ = std::chrono::high_resolution_clock::now();
	released_timestamp_ = pressed_timestamp_;
}

void OKDigitalButton::set_state(const bool down) 
{
	if (down == down_) 
	{
		pressed_ = false;
		released_ = false;
	} 
	else if ((down == true) && (down_ == false))
	{
		pressed_ = true;
		released_ = false;
		down_ = down;

		pressed_count_++;
		pressed_timestamp_ = std::chrono::high_resolution_clock::now();
	} 
	else if ((down == false) && (down_ == true)) 
	{
		pressed_ = false;
		released_ = true;
		down_ = down;

		released_count_++;
		released_timestamp_ = std::chrono::high_resolution_clock::now();
	}
}

uint OKDigitalButton::get_pressed_count() const 
{
	return pressed_count_;
}

uint OKDigitalButton::get_released_count() const 
{
	return released_count_;
}

uint OKDigitalButton::get_new_pressed_count() 
{
	int new_pressed_count = pressed_count_ - previous_pressed_count_;

	if (new_pressed_count > 0) 
	{
		previous_pressed_count_ = pressed_count_;
	}

	return new_pressed_count;
}

uint OKDigitalButton::get_new_released_count() 
{
	int new_released_count = released_count_ - previous_released_count_;

	if (new_released_count > 0) 
	{
		previous_released_count_ = released_count_;
	}

	return new_released_count;
}

float OKDigitalButton::get_held_duration_ms() const 
{
	if (!is_down()) 
	{
		return 0.0f;
	}

	float time_since_last_pressed_ms =
		(float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - pressed_timestamp_)
					.count());
	return time_since_last_pressed_ms;
}

float OKDigitalButton::get_released_duration_ms() const 
{
	if (!was_released()) 
	{
		return 0.0f;
	}

	float time_since_last_pressed_ms =
		(float)(std::chrono::duration<double, std::milli>(released_timestamp_ - pressed_timestamp_).count());
		
	return time_since_last_pressed_ms;
}

void OKDigitalButton::combine(const OKDigitalButton& other) 
{
	down_ = down_ || other.down_;
	pressed_ = pressed_ || other.pressed_;
	released_ = released_ || other.released_;

	pressed_count_ += other.pressed_count_;
	previous_pressed_count_ += other.previous_pressed_count_;

	released_count_ += other.released_count_;
	previous_released_count_ += other.previous_released_count_;

	pressed_timestamp_ = std::max(pressed_timestamp_, other.pressed_timestamp_);
	released_timestamp_ = std::max(released_timestamp_, other.released_timestamp_);
}


}  // namespace BVR



