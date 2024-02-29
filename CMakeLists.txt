#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(InspireFace)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

option(ENABLE_RKNN "Use RKNPU." OFF)
option(BUILD_LINUX_ARM7 "Platform Arm7." OFF)
option(BUILD_WITH_TEST "Open Build Unit-Test." ON)
option(BUILD_WITH_SAMPLE "Open Build Sample Exec." ON)

option(USE_MOBILE_OPENCV_IN_LOCAL "Use mobile-opencv in local environment." ON)

if (USE_MOBILE_OPENCV_IN_LOCAL)
    add_definitions(-DUSE_MOBILE_OPENCV_IN_LOCAL)
endif ()

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/)


add_subdirectory(cpp/inspireface)  # Add a child project: InspireFace Source
get_property(HyperFace TARGET InspireFace PROPERTY InspireFace)

if (BUILD_WITH_SAMPLE)
    add_subdirectory(cpp/sample)  # Add a child project: Samples
endif()

if (BUILD_WITH_TEST)
    add_subdirectory(cpp/test)  # Add a child project: Unit-Test
endif ()