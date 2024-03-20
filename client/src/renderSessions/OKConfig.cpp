//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"

#include "OKConfig.h"

namespace BVR 
{

static const OKConfig default_config;

OKConfig::OKConfig()
{
}

void OKConfig::reset()
{
    *this = default_config;
}

bool OKConfig::load()
{
	return true;
}

bool OKConfig::save()
{
	return true;
}


} // namespace BVR

