#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(HyperFaceTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -o3")
find_package(OpenCV REQUIRED)

include_directories(${SRC_DIR})


# =======================公共文件配置===========================
set(TEST_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/test_settings.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

# =======================内部引入测试===========================
file(GLOB_RECURSE TEST_INTERNAL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(TestSource ${CMAKE_CURRENT_SOURCE_DIR}/test_source.cpp ${TEST_INTERNAL_FILES} ${TEST_COMMON_FILES})
target_link_libraries(TestSource InspireFace)
target_include_directories(TestSource PUBLIC
        .
        ${THIRD_PARTY_DIR}/spdlog
        ${THIRD_PARTY_DIR}/catch2
        ${THIRD_PARTY_DIR}/cnpy
        ${SRC_DIR}
)

set_target_properties(TestSource PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")


# =======================外部API测试===========================
file(GLOB_RECURSE TEST_INTERNAL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp)
add_executable(TestAPI ${CMAKE_CURRENT_SOURCE_DIR}/test_api.cpp ${TEST_INTERNAL_FILES} ${TEST_COMMON_FILES})
target_link_libraries(TestAPI InspireFace)
target_include_directories(TestAPI PUBLIC
        .
        ${THIRD_PARTY_DIR}/spdlog
        ${THIRD_PARTY_DIR}/catch2
        ${THIRD_PARTY_DIR}/cnpy
        ${SRC_DIR}
        )

set_target_properties(TestAPI PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")