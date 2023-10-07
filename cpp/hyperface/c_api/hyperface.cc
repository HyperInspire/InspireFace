//
// Created by tunm on 2023/10/3.
//

#include "hyperface.h"
#include "htypedef.h"
#include "hyperface_internal.h"


HYPER_CAPI_EXPORT extern HResult HF_CreateImageStream(Ptr_HF_ImageData data, HImageHandle* handle) {
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

    *handle = (HImageHandle )stream;

    return HSUCCEED;
}

HYPER_CAPI_EXPORT extern HResult HF_ReleaseImageStream(HImageHandle streamHandle) {
    if (streamHandle == nullptr) {
        return HERR_INVALID_HANDLE;
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        return HERR_INVALID_HANDLE;
    } else {
        delete stream;
        stream = nullptr;  // 设置指针为 nullptr，以避免悬挂指针
    }

    return HSUCCEED;
}


void HF_DeBugImageStreamImShow(HImageHandle streamHandle) {
    if (streamHandle == nullptr) {
        LOGE("Handle error");
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        LOGE("Image error");
        return;
    }
    auto image = stream->impl.GetScaledImage(1.0f, true);

    cv::imshow("Debug", image);
    cv::waitKey(0);
}


HResult HF_CreateFaceContextFromResourceFile(HString resourceFile, Ptr_HF_ContextCustomParameter parameter, HF_DetectMode detectMode, HInt32 maxDetectFaceNum, HContextHandle *handle) {
    hyper::ContextCustomParameter param;
    param.enable_mask_detect = parameter->enable_mask_detect;
    param.enable_age = parameter->enable_age;
    param.enable_liveness = parameter->enable_liveness;
    param.enable_face_quality = parameter->enable_face_quality;
    param.enable_gender = parameter->enable_gender;
    param.enable_interaction_liveness = parameter->enable_interaction_liveness;
    param.enable_ir_liveness = parameter->enable_ir_liveness;
    param.enable_recognition = parameter->enable_recognition;
    hyper::DetectMode detMode = hyper::DETECT_MODE_IMAGE;
    if (detectMode == HF_DETECT_MODE_VIDEO) {
        detMode = hyper::DETECT_MODE_VIDEO;
    }

    std::string path(resourceFile);
    HF_FaceContext *ctx = new HF_FaceContext();
    auto ret = ctx->impl.Configuration(path, detMode, maxDetectFaceNum, param);
    if (ret != HSUCCEED) {
        delete ctx;
        ctx = nullptr;
    }

    return ret;
}