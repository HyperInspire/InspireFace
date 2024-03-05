#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(HyperFaceTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -o3")

if (APPLE)
    if (USE_MOBILE_OPENCV_IN_LOCAL)
        set(OPENCV_FRAMEWORK "${THIRD_PARTY_DIR}/opencv-mobile/${MOBILE_OPENCV_VERSION}/${TARGET_PLATFORM}/opencv2.framework")
        list(APPEND CMAKE_FRAMEWORK_PATH "${OPENCV_FRAMEWORK}")
        include_directories("${OPENCV_FRAMEWORK}/Headers")
        message(OPENCV_FRAMEWORK=${OPENCV_FRAMEWORK})
        find_library(ACCELERATE_FRAMEWORK Accelerate)
    else ()
        find_package(OpenCV REQUIRED)
    endif()
else ()
    if (BUILD_LINUX_ARM7 OR BUILD_LINUX_AARCH64)
        set(PLAT linux-arm7)
        set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/opencv-linux-armhf/share/OpenCV)
        set(OpenCV_STATIC_INCLUDE_DIR ${PATH_3RDPARTY}/opencv/opencv-linux-armhf/include/)
    else()
        set(PLAT linux)
        message(OpenCV_DIR=${OpenCV_DIR})
        set(OpenCV_DIR "${THIRD_PARTY_DIR}/opencv-mobile/${MOBILE_OPENCV_VERSION}/${TARGET_PLATFORM}/lib/cmake/opencv4")
    endif()
    find_package(OpenCV REQUIRED)
endif ()

include_directories(${SRC_DIR})


# =======================Common File Configuration===========================
set(TEST_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/test_settings.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

# =======================Internal Import Tests===========================
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


# =======================External API Testing===========================
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