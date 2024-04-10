#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(InspireFace)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(TARGET_PLATFORM "drawin-x86" CACHE STRING "Target platform")

# OpenCV dependency configuration
option(USE_MOBILE_OPENCV_IN_LOCAL "Use mobile-opencv in local environment." OFF)
set(MOBILE_OPENCV_VERSION "4.9.0" CACHE STRING "The mobile-opencv version number")
if (USE_MOBILE_OPENCV_IN_LOCAL)
    message("Use mobile-opencv ${MOBILE_OPENCV_VERSION}, Target platform ${TARGET_PLATFORM}")
endif ()

# RKNN dependency configuration
option(ENABLE_RKNN "Use RKNPU." OFF)
set(RK_DEVICE_TYPE "RV1109RV1126" CACHE STRING "Type of the device")
if (ENABLE_RKNN)
    add_definitions("-DENABLE_RKNN")
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

option(GLOBAL_INFERENCE_BACKEND_USE_MNN_CUDA "The global inference backend uses MNN CUDA." OFF)
if (GLOBAL_INFERENCE_BACKEND_USE_MNN_CUDA)
    message("Global MNN CUDA device inference")
    add_definitions("-DGLOBAL_INFERENCE_BACKEND_USE_MNN_CUDA")

endif ()

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

# OpenCV Configuration

string(REPLACE "." ";" VERSION_LIST ${MOBILE_OPENCV_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
if (APPLE)
    set(PLAT darwin)
    if (USE_MOBILE_OPENCV_IN_LOCAL)
        set(OPENCV_FRAMEWORK "${THIRD_PARTY_DIR}/opencv-mobile/${MOBILE_OPENCV_VERSION}/${TARGET_PLATFORM}/opencv2.framework")
        list(APPEND CMAKE_FRAMEWORK_PATH "${OPENCV_FRAMEWORK}")
        include_directories("${OPENCV_FRAMEWORK}/Headers")
        message(OPENCV_FRAMEWORK=${OPENCV_FRAMEWORK})
        find_library(ACCELERATE_FRAMEWORK Accelerate)
    else ()
        find_package(OpenCV REQUIRED)
    endif ()
else()
    if (BUILD_LINUX_ARM7 OR BUILD_LINUX_AARCH64)
        add_definitions("-DDISABLE_GUI")
        #        set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/opencv-linux-armhf/share/OpenCV)
        #        set(OpenCV_STATIC_INCLUDE_DIR ${PATH_3RDPARTY}/opencv/opencv-linux-armhf/include/)
        if (RK_DEVICE_TYPE STREQUAL "RV1109RV1126")
            # In special cases, specialize for that version
            message(STATUS "The OpenCV that builds the RV1109RV1126 version depends on is specialized!")
            set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/3.4.5/opencv-linux-armhf/share/OpenCV)
            set(OpenCV_STATIC_INCLUDE_DIR ${PATH_3RDPARTY}/opencv/3.4.5/opencv-linux-armhf/include/)
            set(PLAT linux-arm7)
        else()
            if (VERSION_MAJOR STREQUAL "3")
                set(CV_CMAKE_FOLDER share/OpenCV)
            elseif(VERSION_MAJOR STREQUAL "4")
                set(CV_CMAKE_FOLDER lib/cmake/opencv4)
            endif ()
            if(BUILD_LINUX_ARM7)
                set(PLAT linux-arm7)
                set(OpenCV_DIR "${THIRD_PARTY_DIR}/opencv-mobile/${MOBILE_OPENCV_VERSION}/${TARGET_PLATFORM}/arm-linux-gnueabihf/${CV_CMAKE_FOLDER}")
            elseif(BUILD_LINUX_AARCH64)
                set(OpenCV_DIR "${THIRD_PARTY_DIR}/opencv-mobile/${MOBILE_OPENCV_VERSION}/${TARGET_PLATFORM}/aarch64-linux-gnu/${CV_CMAKE_FOLDER}")
            endif()
        endif()
        message(OpenCV_DIR="${OpenCV_DIR}")
    else ()
        set(PLAT linux)
        message(OpenCV_DIR=${OpenCV_DIR})
        set(OpenCV_DIR "${THIRD_PARTY_DIR}/opencv-mobile/${MOBILE_OPENCV_VERSION}/ubuntu-2004/lib/cmake/opencv4")
    endif ()
    find_package(OpenCV REQUIRED)
endif ()

add_subdirectory(cpp/inspireface)  # Add a child project: InspireFace Source
get_property(InspireFace TARGET InspireFace PROPERTY InspireFace)

if (BUILD_WITH_SAMPLE)
    add_subdirectory(cpp/sample)  # Add a child project: Samples
endif ()

if (BUILD_WITH_TEST)
    add_subdirectory(cpp/test)  # Add a child project: Unit-Test
endif ()