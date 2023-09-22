#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(HyperFaceSample)

if (BUILD_LINUX_ARM7)
    set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/opencv-linux-armhf/share/OpenCV)
    set(OpenCV_STATIC_INCLUDE_DIR ${PATH_3RDPARTY}/opencv/opencv-linux-armhf/include/)
endif()

find_package(OpenCV REQUIRED)

include_directories(${SRC_DIR})

#find_package(Eigen3 REQUIRED)
#include_directories("/usr/local/include/eigen3")

if (NOT BUILD_LINUX_ARM7)

# =======================HyperFace测试===========================
    # 跟踪测试
    add_executable(TrackerSample ${CMAKE_CURRENT_SOURCE_DIR}/tracker_sample.cpp)
    target_link_libraries(TrackerSample HyperFace)

    set_target_properties(TrackerSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )


    # Context测试
    add_executable(ContextSample ${CMAKE_CURRENT_SOURCE_DIR}/context_sample.cpp)
    target_link_libraries(ContextSample HyperFace)

    set_target_properties(ContextSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )


    # 测试
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


    # 性能测试
    add_executable(BMSample ${CMAKE_CURRENT_SOURCE_DIR}/bm_sample.cpp)
    target_link_libraries(BMSample HyperFace)

    set_target_properties(BMSample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
    )

else()

# =======================RK临时测试类目===========================

    if (ENABLE_RKNN)
        set(RKNN_API_LIB ${THIRD_PARTY_DIR}/rknn_api/lib)
        message("Enable RKNN Inference")
        link_directories(${RKNN_API_LIB})

        # 人脸检测
        add_executable(RKFaceDetSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_face_det_sample.cpp)
        target_link_libraries(RKFaceDetSample HyperFace rknn_api dl)

        set_target_properties(RKFaceDetSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

        # 简单的网络测试
        add_executable(RKSimpleNetSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_simple_net_sample.cpp)
        target_link_libraries(RKSimpleNetSample HyperFace rknn_api dl)

        set_target_properties(RKSimpleNetSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

        # 人脸识别
        add_executable(RKFaceRecSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_face_recognize_sample.cpp)
        target_link_libraries(RKFaceRecSample HyperFace rknn_api dl)

        set_target_properties(RKFaceRecSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

        # 跟踪模块识别
        add_executable(RKTrackerSample ${CMAKE_CURRENT_SOURCE_DIR}/rk_sample/rk_tracker_sample.cpp)
        target_link_libraries(RKTrackerSample HyperFace rknn_api dl)

        set_target_properties(RKTrackerSample PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample/"
        )

    endif()
endif()




