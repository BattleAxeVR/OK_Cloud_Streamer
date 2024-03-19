//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"

#include "OKPlayerState.h"
#include "OKController.h"

namespace BVR 
{

OKPlayerState::OKPlayerState() : controllers_{{*this, LEFT}, {*this, RIGHT}}
{
	
}

bool OKPlayerState::init()
{
	return true;
}

void OKPlayerState::shutdown()
{
	
}

bool OKPlayerState::update()
{
	return true;
}

} // namespace BVR
