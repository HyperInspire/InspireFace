cmake_minimum_required(VERSION 3.19)
project(HyperFaceRepo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/)


add_subdirectory(cpp/hyperface)  # 添加子项目
get_property(HyperFace TARGET HyperFace PROPERTY HyperFace)

add_subdirectory(cpp/sample)  # 添加子项目

add_subdirectory(cpp/test)  # 添加子项目