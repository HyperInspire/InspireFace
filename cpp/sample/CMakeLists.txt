#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(InspireFaceSample)

if (BUILD_LINUX_ARM7 OR BUILD_LINUX_AARCH64)
    set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/opencv-linux-armhf/share/OpenCV)
    set(OpenCV_STATIC_INCLUDE_DIR ${PATH_3RDPARTY}/opencv/opencv-linux-armhf/include/)
endif()

if (APPLE)
    if (USE_MOBILE_OPENCV_IN_LOCAL)
        set(OPENCV_FRAMEWORK "${THIRD_PARTY_DIR}/opencv-mobile/4.9.0/drawin-x86/opencv2.framework")
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
        set(OpenCV_DIR "${THIRD_PARTY_DIR}/opencv-mobile/4.9.0/ubuntu-2004/lib/cmake/opencv4")
    endif()
    find_package(OpenCV REQUIRED)
endif ()

find_package(OpenCV REQUIRED)

include_directories(${SRC_DIR})

#find_package(Eigen3 REQUIRED)
#include_directories("/usr/local/include/eigen3")

if (NOT BUILD_LINUX_ARM7 AND NOT BUILD_LINUX_AARCH64)

# =======================InspireFace Sample===========================
    add_executable(TrackerSample ${CMAKE_CURRENT_SOURCE_DIR}/tracker_sample.cpp)
    target_link_libraries(TrackerSample InspireFace)

    set_target_properties(TrackerSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )


    add_executable(ContextSample ${CMAKE_CURRENT_SOURCE_DIR}/context_sample.cpp)
    target_link_libraries(ContextSample InspireFace)

    set_target_properties(ContextSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )


    add_executable(TestSample ${CMAKE_CURRENT_SOURCE_DIR}/test_sample.cpp)
    target_link_libraries(TestSample InspireFace)

    set_target_properties(TestSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )

    add_executable(NetSample ${CMAKE_CURRENT_SOURCE_DIR}/net_sample.cpp)
    target_link_libraries(NetSample InspireFace)

    set_target_properties(NetSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )


    add_executable(RecSample ${CMAKE_CURRENT_SOURCE_DIR}/rec_sample.cpp)
    target_link_libraries(RecSample InspireFace)

    set_target_properties(RecSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )


    add_executable(BMSample ${CMAKE_CURRENT_SOURCE_DIR}/bm_sample.cpp)
    target_link_libraries(BMSample InspireFace)

    set_target_properties(BMSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )

else()

# =======================RK Temporary test category===========================

    if (ENABLE_RKNN)
        set(RKNN_API_LIB ${THIRD_PARTY_DIR}/rknn_api/lib)
        message("Enable RKNN Inference")
        link_directories(${RKNN_API_LIB})

        # Face detection
        add_executable(RKFaceDetSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_face_det_sample.cpp)
        target_link_libraries(RKFaceDetSample InspireFace rknn_api dl)

        set_target_properties(RKFaceDetSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

        # Simple network test
        add_executable(RKSimpleNetSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_simple_net_sample.cpp)
        target_link_libraries(RKSimpleNetSample InspireFace rknn_api dl)

        set_target_properties(RKSimpleNetSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

        # Face recognize
        add_executable(RKFaceRecSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_face_recognize_sample.cpp)
        target_link_libraries(RKFaceRecSample InspireFace rknn_api dl)

        set_target_properties(RKFaceRecSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

        # Tracking module
        add_executable(RKTrackerSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_tracker_sample.cpp)
        target_link_libraries(RKTrackerSample InspireFace rknn_api dl)

        set_target_properties(RKTrackerSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

        # Debug
        add_executable(DebugRKRec ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/debug_rk_rec.cpp)
        target_link_libraries(DebugRKRec InspireFace rknn_api dl)

        set_target_properties(DebugRKRec PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

    endif()

endif()

# Tracking module
add_executable(SQLiteTest ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_sample.cpp)
target_link_libraries(SQLiteTest InspireFace)

set_target_properties(SQLiteTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)


# C_API Demo
add_executable(CAPISample ${CMAKE_CURRENT_SOURCE_DIR}/c_api_sample.cpp)
target_link_libraries(CAPISample InspireFace)

set_target_properties(CAPISample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)


