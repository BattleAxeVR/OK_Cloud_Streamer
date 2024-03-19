//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CONFIG_H
#define OK_CONFIG_H

namespace BVR 
{

class OKConfig 
{
public:
    OKConfig();

	bool init();
	void shutdown();
	
	bool update();
	
};

} // namespace BVR

#endif // OK_CONFIG_H

