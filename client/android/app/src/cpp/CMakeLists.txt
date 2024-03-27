#--------------------------------------------------------------------------------------
# Copyright (c) 2024 BattleAxeVR. All rights reserved.
#--------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

target_sources(IGLShellShared PUBLIC GLMPose.cpp)
target_sources(IGLShellShared PUBLIC OKAnalogAxis.cpp)
target_sources(IGLShellShared PUBLIC OKCloudClient.cpp)
#target_sources(IGLShellShared PUBLIC OKCloudSession.cpp)
target_sources(IGLShellShared PUBLIC OKConfig.cpp)
target_sources(IGLShellShared PUBLIC OKController.cpp)
target_sources(IGLShellShared PUBLIC OKDigitalButton.cpp)
target_sources(IGLShellShared PUBLIC OKPlayerState.cpp)

add_subdirectory(jsoncpp)
target_include_directories(IGLShellShared PUBLIC jsoncpp)
