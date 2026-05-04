cmake_minimum_required(VERSION 3.25)

project(WalkSpeedTuner VERSION 1.4.0 LANGUAGES CXX)

option(BUILD_TESTING "Build unit tests (pure-function modules only; no SKSE link)" OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(BUILD_TESTING)
    include(testtarget)
else()
    include(pluginconfig)
    include(addpluginsources)
    include(plugintarget)
endif()
