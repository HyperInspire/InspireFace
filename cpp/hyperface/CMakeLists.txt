cmake_minimum_required(VERSION 3.19)
project(HyperFaceSDK)

set(CMAKE_CXX_STANDARD 14)

option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." ON)


find_package(OpenCV REQUIRED)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if (APPLE)
    set(PLAT darwin)
else()
    set(PLAT linux)
endif ()

if (NOT ANDROID)
    # MNN Third party dependence
    set(MNN_INCLUDE_DIRS ${THIRD_PARTY_DIR}/MNN/${PLAT}-static/include)
    set(MNN_LIBS ${THIRD_PARTY_DIR}/MNN/${PLAT}-static/lib/libMNN.a)
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
target_include_directories(HyperFace PUBLIC
    .
    ${MNN_INCLUDE_DIRS}
)

if (NOT ANDROID)
    target_link_libraries(HyperFace PUBLIC ${OpenCV_LIBS} ${MNN_LIBS})

else()

endif()
