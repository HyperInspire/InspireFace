#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(HyperFaceRepo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

option(ENABLE_RKNN "Use RKNPU." ON)
option(BUILD_LINUX_ARM7 "Platform Arm7." ON)
option(BUILD_HYPERFACE_TEST "Platform Arm7." ON)


set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/)


add_subdirectory(cpp/inspireface)  # 添加子项目
get_property(HyperFace TARGET InspireFace PROPERTY InspireFace)

add_subdirectory(cpp/sample)  # 添加子项目

if (BUILD_HYPERFACE_TEST)
    add_subdirectory(cpp/test)  # 添加子项目
endif ()