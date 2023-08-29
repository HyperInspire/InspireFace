cmake_minimum_required(VERSION 3.19)
project(HyperFaceSample)

find_package(OpenCV REQUIRED)

include_directories(${SRC_DIR})

# =======================HyperFace测试===========================
# 主体测试
add_executable(ContextSample ${CMAKE_CURRENT_SOURCE_DIR}/context_sample.cpp)
target_link_libraries(ContextSample HyperFace)

set_target_properties(ContextSample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)