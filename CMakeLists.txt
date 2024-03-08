#--------------------------------------------------------------------------------------
# Copyright (c) 2024 BattleAxeVR. All rights reserved.
#--------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

project("OK_Cloud_Streamer_Client" CXX C)

option(ENABLE_CLOUDXR "Enable CloudXR Integration" ON)

add_subdirectory(external/igl)