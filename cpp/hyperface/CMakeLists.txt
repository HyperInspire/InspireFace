#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.10)
project(HyperFaceSDK)

set(CMAKE_CXX_STANDARD 14)

option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." ON)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if (APPLE)
    set(PLAT darwin)
else()
    if (BUILD_LINUX_ARM7)
        set(PLAT linux-arm7)
        set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/opencv-linux-armhf/share/OpenCV)
        set(OpenCV_STATIC_INCLUDE_DIR ${PATH_3RDPARTY}/opencv/opencv-linux-armhf/include/)
    else()
        set(PLAT linux)
    endif ()
endif ()

find_package(OpenCV REQUIRED)

if (NOT ANDROID)
    # MNN Third party dependence
    set(MNN_INCLUDE_DIRS ${THIRD_PARTY_DIR}/MNN/${PLAT}-static/include)
    set(MNN_LIBS ${THIRD_PARTY_DIR}/MNN/${PLAT}-static/lib/libMNN.a)
endif()


if (ENABLE_RKNN)
    set(RKNN_API_INCLUDE_DIRS ${THIRD_PARTY_DIR}/rknn_api/include)
    set(RKNN_API_LIB ${THIRD_PARTY_DIR}/rknn_api/lib)
    message("Enable RKNN Inference")
    link_directories(${RKNN_API_LIB})
endif()

set(LINK_THIRD_LIBS ${OpenCV_LIBS} ${MNN_LIBS})

if (ENABLE_RKNN)
    set(LINK_THIRD_LIBS ${LINK_THIRD_LIBS} rknn_api dl)
    # InferenceHelp use RkNN
    add_definitions("-DINFERENCE_HELPER_ENABLE_RKNN")

endif()

# mnn
message(MNN_INCLUDE_DIRS=${MNN_INCLUDE_DIRS})
message(MNN_LIBS=${MNN_LIBS})
#link_directories(${MNN_LIBS})

if(BUILD_SHARED_LIBS)
    add_library(HyperFace SHARED ${CPP_FILES})
else()
    add_library(HyperFace STATIC ${CPP_FILES})
endif()

target_compile_definitions(HyperFace PUBLIC INFERENCE_HELPER_ENABLE_MNN)
target_compile_definitions(HyperFace PUBLIC FEATURE_BLOCK_ENABLE_OPENCV)

# Include files
set(NEED_INCLUDE . ${MNN_INCLUDE_DIRS})
if (ENABLE_RKNN)
    set(NEED_INCLUDE ${NEED_INCLUDE} ${RKNN_API_INCLUDE_DIRS})
endif ()
if (BUILD_LINUX_ARM7)
    set(NEED_INCLUDE ${NEED_INCLUDE} ${OpenCV_STATIC_INCLUDE_DIR})
endif ()

target_include_directories(HyperFace PUBLIC
    ${NEED_INCLUDE}
)

if (NOT ANDROID)
    target_link_libraries(HyperFace PUBLIC ${LINK_THIRD_LIBS})
else()
    # xxx
endif()
