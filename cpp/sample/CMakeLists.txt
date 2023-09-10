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


# 主体测试
add_executable(TestSample ${CMAKE_CURRENT_SOURCE_DIR}/test_sample.cpp)
target_link_libraries(TestSample HyperFace)

set_target_properties(TestSample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)

# 调试测试
add_executable(NetSample ${CMAKE_CURRENT_SOURCE_DIR}/net_sample.cpp)
target_link_libraries(NetSample HyperFace)

set_target_properties(NetSample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)


# 调试测试
add_executable(RecSample ${CMAKE_CURRENT_SOURCE_DIR}/rec_sample.cpp)
target_link_libraries(RecSample HyperFace)

set_target_properties(RecSample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)


