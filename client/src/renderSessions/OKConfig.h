//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CONFIG_H
#define OK_CONFIG_H

#include <string>

namespace BVR 
{

class OKConfig 
{
public:
    OKConfig();

	bool init();
	void shutdown();
	
	bool update();

    std::string ip_address_ = DEFAULT_IP_ADDRESS;
    std::string app_directory_ = OK_CLOUD_STREAMER_APP_DIRECTORY;

    bool is_initialized_ = false;
};

} // namespace BVR

#endif // OK_CONFIG_H

