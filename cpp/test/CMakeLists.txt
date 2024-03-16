cmake_minimum_required(VERSION 3.10)
project(HyperFaceTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")


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

# =======================Common File Configuration===========================
set(TEST_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/test_settings.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

# =======================Internal Import Tests===========================
file(GLOB_RECURSE INTERNAL_TEST_INTERNAL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(TestSource ${CMAKE_CURRENT_SOURCE_DIR}/test_source.cpp ${INTERNAL_TEST_INTERNAL_FILES} ${TEST_COMMON_FILES})
target_link_libraries(TestSource InspireFace ${DEPEND})
target_include_directories(TestSource PUBLIC
        .
        ${THIRD_PARTY_DIR}/spdlog
        ${THIRD_PARTY_DIR}/catch2
        ${THIRD_PARTY_DIR}/cnpy
        ${SRC_DIR}
)

set_target_properties(TestSource PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")


# =======================External API Testing===========================
file(GLOB_RECURSE TEST_INTERNAL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp)
add_executable(TestAPI ${CMAKE_CURRENT_SOURCE_DIR}/test_api.cpp ${TEST_INTERNAL_FILES} ${TEST_COMMON_FILES})
target_link_libraries(TestAPI InspireFace ${DEPEND})
target_include_directories(TestAPI PUBLIC
        .
        ${THIRD_PARTY_DIR}/spdlog
        ${THIRD_PARTY_DIR}/catch2
        ${THIRD_PARTY_DIR}/cnpy
        ${SRC_DIR}
        )

set_target_properties(TestAPI PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")