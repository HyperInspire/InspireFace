#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(InspireFace)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(TARGET_PLATFORM "drawin-x86" CACHE STRING "Target platform")

# OpenCV dependency configuration
option(USE_MOBILE_OPENCV_IN_LOCAL "Use mobile-opencv in local environment." ON)
set(MOBILE_OPENCV_VERSION "4.9.0" CACHE STRING "The mobile-opencv version number")
if (USE_MOBILE_OPENCV_IN_LOCAL)
    message("Use mobile-opencv ${MOBILE_OPENCV_VERSION}, Target platform ${TARGET_PLATFORM}")
endif ()

# RKNN dependency configuration
option(ENABLE_RKNN "Use RKNPU." OFF)
set(RK_DEVICE_TYPE "RV1109RV1126" CACHE STRING "Type of the device")
if (ENABLE_RKNN)
    # Device list
    set(RKNPU1_DEVICES "RV1109RV1126")
    set(RKNPU2_DEVICES "RK356X" "RK3588" "RV1106")
    set(RKNPU_MAJOR "")
    list(FIND RKNPU1_DEVICES "${RK_DEVICE_TYPE}" INDEX_RKNPU1)
    list(FIND RKNPU2_DEVICES "${RK_DEVICE_TYPE}" INDEX_RKNPU2)
    if(INDEX_RKNPU1 GREATER_EQUAL 0)
        # match rknn1
        set(RKNPU_MAJOR "rknpu1")
    elseif(INDEX_RKNPU2 GREATER_EQUAL 0)
        # match rknn2
        set(RKNPU_MAJOR "rknpu2")
    endif()
    # Result
    message(STATUS "Use ${RKNPU_MAJOR}")
endif ()


option(BUILD_LINUX_ARM7 "Platform Armv7." OFF)
option(BUILD_LINUX_AARCH64 "Platform Armv8." OFF)

if (BUILD_LINUX_ARM7)
    set(CPU_ARCH "armhf")
endif()
if (BUILD_LINUX_AARCH64)
    set(CPU_ARCH "aarch64")
endif()

option(BUILD_WITH_TEST "Open Build Unit-Test." ON)
option(BUILD_WITH_SAMPLE "Open Build Sample Exec." ON)

option(LINUX_FETCH_MNN "Fetch and build MNN from git" OFF)


if (USE_MOBILE_OPENCV_IN_LOCAL)
    add_definitions(-DUSE_MOBILE_OPENCV_IN_LOCAL)
endif ()

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/)


add_subdirectory(cpp/inspireface)  # Add a child project: InspireFace Source
get_property(HyperFace TARGET InspireFace PROPERTY InspireFace)

if (BUILD_WITH_SAMPLE)
    add_subdirectory(cpp/sample)  # Add a child project: Samples
endif ()

if (BUILD_WITH_TEST)
    add_subdirectory(cpp/test)  # Add a child project: Unit-Test
endif ()