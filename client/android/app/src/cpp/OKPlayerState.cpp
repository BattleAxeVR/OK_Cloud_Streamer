//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ok_defines.h"

#include "OKPlayerState.h"
#include "OKController.h"

namespace BVR 
{

OKPlayerState::OKPlayerState() : controllers_{{*this, LEFT_CONTROLLER}, {*this, RIGHT_CONTROLLER}}
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

