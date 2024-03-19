//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_PLAYER_STATE_H
#define OK_PLAYER_STATE_H

#include "GLMPose.h"

namespace BVR 
{

class OKPlayerState 
{
public:
    OKPlayerState();

	bool init();
	void shutdown();
	
	bool update();
	
};

} // namespace BVR

#endif // OK_PLAYER_STATE_H

