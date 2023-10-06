//
// Created by tunm on 2023/10/3.
//

#include "hyperface.h"
#include "htypedef.h"
#include "hyperface_internal.h"


HYPER_CAPI_EXPORT extern HResult HF_CreateImageStream(Ptr_HF_ImageData data, HHandle* handle) {
    auto stream = new HF_CameraStream();
    if (data->rotation == 1) {
        stream->impl.SetRotationMode(ROTATION_90);
    } else if (data->rotation == 2) {
        stream->impl.SetRotationMode(ROTATION_180);
    } else if (data->rotation == 3) {
        stream->impl.SetRotationMode(ROTATION_270);
    } else {
        stream->impl.SetRotationMode(ROTATION_0);
    }
    if (data->format == 0) {
        stream->impl.SetDataFormat(RGB);
    } else if (data->format == 1) {
        stream->impl.SetDataFormat(BGR);
    } else if (data->format == 2) {
        stream->impl.SetDataFormat(RGBA);
    } else if (data->format == 3) {
        stream->impl.SetDataFormat(BGRA);
    } else if (data->format == 4) {
        stream->impl.SetDataFormat(NV12);
    } else if (data->format == 5) {
        stream->impl.SetDataFormat(NV21);
    }
    stream->impl.SetDataBuffer(data->data, data->height, data->width);

    handle = (HHandle* ) stream;

    return HSUCCEED;
}

HYPER_CAPI_EXPORT extern HResult HF_ReleaseImageStream(HHandle streamHandle) {
    HF_CameraStream  *stream = (HF_CameraStream* ) streamHandle;
    delete stream;

    return HSUCCEED;
}