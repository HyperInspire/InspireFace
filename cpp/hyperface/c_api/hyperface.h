//
// Created by tunm on 2023/10/3.
//

#ifndef HYPERFACEREPO_HYPERFACE_H
#define HYPERFACEREPO_HYPERFACE_H

#include <stdint.h>
#include "htypedef.h"
#include "herror.h"

#if defined(_WIN32)
#ifdef HYPER_BUILD_SHARED_LIB
#define HYPER_CAPI_EXPORT __declspec(dllexport)
#else
#define HYPER_CAPI_EXPORT
#endif
#else
#define HYPER_CAPI_EXPORT __attribute__((visibility("default")))
#endif // _WIN32


#ifdef __cplusplus
extern "C" {
#endif

/**
 * camera stream format - 支持的相机流格式
 * Contains several common camera stream formats on the market -
 * 包含了几款市面上常见的相机流格式
 */
typedef enum HF_ImageFormat {
    STREAM_RGB = 0,             ///< Image in RGB format - RGB排列格式的图像
    STREAM_BGR = 1,             ///< Image in BGR format (Opencv Mat default) - BGR排列格式的图像(OpenCV的Mat默认)
    STREAM_RGBA = 2,            ///< Image in RGB with alpha channel format - 带alpha通道的RGB排列格式的图像
    STREAM_BGRA = 3,            ///< Image in BGR with alpha channel format - 带alpha通道的BGR排列格式的图像
    STREAM_YUV_NV12 = 4,        ///< Image in YUV NV12 format - YUV NV12排列的图像格式
    STREAM_YUV_NV21 = 5,        ///< Image in YUV NV21 format - YUV NV21排列的图像格式
} HF_ImageFormat;

/**
 * Camera picture corner mode - 相机画面转角模式
 * To cope with the rotation of some devices, four image rotation modes are provided here -
 * 为应对某些设备的画面自带旋转，这里提供四种图像旋转模式
 */
typedef enum HF_Rotation {
    CAMERA_ROTATION_0 = 0,      ///< 0 degree - 0
    CAMERA_ROTATION_90 = 1,     ///< 90 degree - 90
    CAMERA_ROTATION_180 = 2,    ///< 180 degree - 180
    CAMERA_ROTATION_270 = 3,    ///< 270 degree - 270
} HF_Rotation;

/**
 * Image Buffer Data struct - 图像数据流结构
 * */
typedef struct HF_ImageData {
    uint8_t *data;                      ///< Image data stream - 图像数据流
    int width;                          ///< Width of the image - 宽
    int height;                         ///< Height of the image - 高
    HF_ImageFormat format;            ///< Format of the image - 传入需要解析数据流格式
    HF_Rotation rotation;             ///< The rotation Angle of the image - 图像的画面旋转角角度
} HF_ImageData, *Ptr_HF_ImageData;


/************************************************************************
* Carry parameters to create a data buffer stream instantiation object.
* 携带创建数据缓冲流实例化对象.
* [out] return: Model instant handle - 返回实例化后的指针句柄
************************************************************************/
HYPER_CAPI_EXPORT extern HResult HF_CreateImageStream(
        Ptr_HF_ImageData data,       // [in] Image Buffer Data struct - 图像数据流结构
        HHandle* handle              // [out] Return a stream handle
);

/************************************************************************
* Releases the DataBuffer object that has been instantiated.
* 释放已经被实例化后的模型对象.
* [out] Result Code - 返回结果码
************************************************************************/
HYPER_CAPI_EXPORT extern HResult HF_ReleaseImageStream(
        HHandle streamHandle             // [in] DataBuffer handle - 相机流组件的句柄指针
);


#ifdef __cplusplus
}
#endif


#endif //HYPERFACEREPO_HYPERFACE_H
