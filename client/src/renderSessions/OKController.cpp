//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"
#include "OKController.h"
#include "OKPlayerState.h"

namespace BVR 
{

OKController::OKController(OKPlayerState& ok_player, const int controller_id) : ok_player_(ok_player), controller_id_(controller_id)
{

}

}  // namespace BVR
