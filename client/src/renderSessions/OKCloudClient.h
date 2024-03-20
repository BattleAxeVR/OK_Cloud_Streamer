//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CLOUD_CLIENT_H
#define OK_CLOUD_CLIENT_H

#include "defines.h"

#if ENABLE_CLOUDXR

#include "OKConfig.h"
#include "OKPlayerState.h"

#include <CloudXRClient.h>
#include <CloudXRMatrixHelpers.h>
#include <CloudXRClientOptions.h>
#include <CloudXRController.h>

#if ENABLE_OBOE
#include <oboe/Oboe.h>
#endif

namespace BVR 
{

class OKCloudClient 
{
	public:
		OKCloudClient();
};

}  // namespace BVR

#endif // ENABLE_CLOUDXR

#endif  // OK_CLOUD_CLIENT_H

