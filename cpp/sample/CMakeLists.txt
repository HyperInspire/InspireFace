cmake_minimum_required(VERSION 3.10)
project(InspireFaceSample)

# OpenCV Configuration
if(NOT APPLE)
    message(STATUS OpenCV_DIR=${OpenCV_DIR})
    find_package(OpenCV REQUIRED)
endif()

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
        set(RKNN_API_LIB ${THIRD_PARTY_DIR}/${RKNPU_MAJOR}/runtime/${RK_DEVICE_TYPE}/Linux/librknn_api/${CPU_ARCH}/)
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


        add_executable(ArchTest ${CMAKE_CURRENT_SOURCE_DIR}/archive_test.cpp)
        target_link_libraries(ArchTest InspireFace)

        set_target_properties(ArchTest PROPERTIES
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

if (ENABLE_RKNN)
    set(DEPEND rknn_api dl)
endif ()

# C_API Demo
add_executable(CAPISample ${CMAKE_CURRENT_SOURCE_DIR}/c_api_sample.cpp)
target_link_libraries(CAPISample InspireFace ${DEPEND})

set_target_properties(CAPISample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)



# C_API Demo
add_executable(LoopTracker ${CMAKE_CURRENT_SOURCE_DIR}/loop_tracker.cpp)
target_link_libraries(LoopTracker InspireFace ${DEPEND})

set_target_properties(LoopTracker PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
)



add_executable(ArchTracker ${CMAKE_CURRENT_SOURCE_DIR}/archive_tracker.cpp)
target_link_libraries(ArchTracker InspireFace)

set_target_properties(ArchTracker PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )
