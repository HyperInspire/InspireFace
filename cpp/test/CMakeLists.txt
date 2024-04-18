cmake_minimum_required(VERSION 3.10)
project(HyperFaceTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
option(ENABLE_BENCHMARK "Enable the benchmark test cases." ON)
option(ENABLE_USE_LFW_DATA "Enable test cases for LFW data sets." ON)
option(ENABLE_TEST_EVALUATION "Enable evaluation function test cases." ON)

if (ENABLE_BENCHMARK)
    add_definitions("-DENABLE_BENCHMARK")
endif ()

if (ENABLE_USE_LFW_DATA)
    add_definitions("-DENABLE_USE_LFW_DATA")
endif ()

if (ENABLE_USE_LFW_DATA)
    add_definitions("-DENABLE_TEST_EVALUATION")
endif ()

# OpenCV Configuration
if(NOT APPLE)
    message(STATUS OpenCV_DIR=${OpenCV_DIR})
    find_package(OpenCV REQUIRED)
endif()


if (ENABLE_RKNN)
    set(DEPEND rknn_api dl)
    set(RKNN_API_LIB ${THIRD_PARTY_DIR}/${RKNPU_MAJOR}/runtime/${RK_DEVICE_TYPE}/Linux/librknn_api/${CPU_ARCH}/)
    message("Enable RKNN Inference")
    link_directories(${RKNN_API_LIB})
    set(DEPEND rknn_api dl)
endif ()

include_directories(${SRC_DIR})

# =======================Common File Configuration===========================
set(TEST_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/settings/test_settings.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/settings)

# =======================Internal Import Tests===========================
file(GLOB_RECURSE INTERNAL_TEST_INTERNAL_FILES unit/source/*.cpp)
add_executable(TestSource ${CMAKE_CURRENT_SOURCE_DIR}/test_source.cpp ${INTERNAL_TEST_INTERNAL_FILES} ${TEST_COMMON_FILES})
target_link_libraries(TestSource InspireFace ${DEPEND})
target_include_directories(TestSource PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${THIRD_PARTY_DIR}/spdlog
        ${THIRD_PARTY_DIR}/catch2
        ${THIRD_PARTY_DIR}/cnpy
        ${THIRD_PARTY_DIR}/indicators
        ${THIRD_PARTY_DIR}/limonp
        ${SRC_DIR}
)

set_target_properties(TestSource PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")


# =======================External API Testing===========================
file(GLOB_RECURSE TEST_INTERNAL_FILES unit/api/*.cpp)
add_executable(TestAPI ${CMAKE_CURRENT_SOURCE_DIR}/test_api.cpp ${TEST_INTERNAL_FILES} ${TEST_COMMON_FILES}
        unit/api/test_evaluation.cpp)
target_link_libraries(TestAPI InspireFace ${DEPEND})
target_include_directories(TestAPI PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${THIRD_PARTY_DIR}/spdlog
        ${THIRD_PARTY_DIR}/catch2
        ${THIRD_PARTY_DIR}/cnpy
        ${THIRD_PARTY_DIR}/indicators
        ${THIRD_PARTY_DIR}/limonp
        ${SRC_DIR}
        )

set_target_properties(TestAPI PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")
