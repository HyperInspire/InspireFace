//
// Created by tunm on 2023/10/3.
//

#include "inspireface.h"
#include "intypedef.h"
#include "inspireface_internal.h"
#include "information.h"
#include "feature_hub/feature_hub.h"
#include "model_hub/model_hub.h"

using namespace inspire;

HYPER_CAPI_EXPORT extern HResult HF_CreateImageStream(Ptr_HF_ImageData data, HImageHandle* handle) {
    if (data == nullptr || handle == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }

    auto stream = new HF_CameraStream();
    switch (data->rotation) {
        case CAMERA_ROTATION_90: stream->impl.SetRotationMode(ROTATION_90); break;
        case CAMERA_ROTATION_180: stream->impl.SetRotationMode(ROTATION_180); break;
        case CAMERA_ROTATION_270: stream->impl.SetRotationMode(ROTATION_270); break;
        default: stream->impl.SetRotationMode(ROTATION_0); break;
    }
    switch (data->format) {
        case STREAM_RGB: stream->impl.SetDataFormat(RGB); break;
        case STREAM_BGR: stream->impl.SetDataFormat(BGR); break;
        case STREAM_RGBA: stream->impl.SetDataFormat(RGBA); break;
        case STREAM_BGRA: stream->impl.SetDataFormat(BGRA); break;
        case STREAM_YUV_NV12: stream->impl.SetDataFormat(NV12); break;
        case STREAM_YUV_NV21: stream->impl.SetDataFormat(NV21); break;
        default: return HERR_INVALID_IMAGE_STREAM_PARAM;  // Assume there's a return code for unsupported formats
    }
    stream->impl.SetDataBuffer(data->data, data->height, data->width);

    *handle = (HImageHandle)stream;
    return HSUCCEED;
}


HYPER_CAPI_EXPORT extern HResult HF_ReleaseImageStream(HImageHandle streamHandle) {
    if (streamHandle == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    delete (HF_CameraStream*)streamHandle;
    return HSUCCEED;
}


void HF_DeBugImageStreamImShow(HImageHandle streamHandle) {
    if (streamHandle == nullptr) {
        INSPIRE_LOGE("Handle error");
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        INSPIRE_LOGE("Image error");
        return;
    }
    auto image = stream->impl.GetScaledImage(1.0f, true);
# ifdef DISABLE_GUI
    cv::imwrite("tmp.jpg", image);
#else
    cv::imshow("Debug", image);
    cv::waitKey(0);
#endif
}


HResult HF_ReleaseFaceContext(HContextHandle handle) {
    if (handle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    delete (HF_FaceContext*)handle;
    return HSUCCEED;
}


HResult HF_CreateFaceContextFromResourceFile(HF_ContextCustomParameter parameter, HF_DetectMode detectMode, HInt32 maxDetectFaceNum, HContextHandle *handle) {
    inspire::ContextCustomParameter param;
    param.enable_mask_detect = parameter.enable_mask_detect;
    param.enable_age = parameter.enable_age;
    param.enable_liveness = parameter.enable_liveness;
    param.enable_face_quality = parameter.enable_face_quality;
    param.enable_gender = parameter.enable_gender;
    param.enable_interaction_liveness = parameter.enable_interaction_liveness;
    param.enable_ir_liveness = parameter.enable_ir_liveness;
    param.enable_recognition = parameter.enable_recognition;
    inspire::DetectMode detMode = inspire::DETECT_MODE_IMAGE;
    if (detectMode == HF_DETECT_MODE_VIDEO) {
        detMode = inspire::DETECT_MODE_VIDEO;
    }

    HF_FaceContext *ctx = new HF_FaceContext();
    auto ret = ctx->impl.Configuration(detMode, maxDetectFaceNum, param);
    if (ret != HSUCCEED) {
        delete ctx;
        *handle = nullptr;
    } else {
        *handle = ctx;
    }


    return ret;
}

HResult HF_CreateFaceContextFromResourceFileOptional(HInt32 customOption, HF_DetectMode detectMode, HInt32 maxDetectFaceNum, HContextHandle *handle) {
    inspire::ContextCustomParameter param;
    if (customOption & HF_ENABLE_FACE_RECOGNITION) {
        param.enable_recognition = true;
    }
    if (customOption & HF_ENABLE_LIVENESS) {
        param.enable_liveness = true;
    }
    if (customOption & HF_ENABLE_IR_LIVENESS) {
        param.enable_ir_liveness = true;
    }
    if (customOption & HF_ENABLE_AGE_PREDICT) {
        param.enable_age = true;
    }
    if (customOption & HF_ENABLE_GENDER_PREDICT) {
        param.enable_gender = true;
    }
    if (customOption & HF_ENABLE_MASK_DETECT) {
        param.enable_mask_detect = true;
    }
    if (customOption & HF_ENABLE_QUALITY) {
        param.enable_face_quality = true;
    }
    if (customOption & HF_ENABLE_INTERACTION) {
        param.enable_interaction_liveness = true;
    }
    inspire::DetectMode detMode = inspire::DETECT_MODE_IMAGE;
    if (detectMode == HF_DETECT_MODE_VIDEO) {
        detMode = inspire::DETECT_MODE_VIDEO;
    }
    HF_FaceContext *ctx = new HF_FaceContext();
    auto ret = ctx->impl.Configuration(detMode, maxDetectFaceNum, param);
    if (ret != HSUCCEED) {
        delete ctx;
        *handle = nullptr;
    } else {
        *handle = ctx;
    }

    return ret;
}

HResult HF_LaunchInspireFace(HPath resourcePath) {
    std::string path(resourcePath);
    return MODEL_HUB->Load(resourcePath);
}

HResult HF_FeatureHubDataDisable() {
    return FEATURE_HUB->DisableHub();
}

HResult HF_FeatureHubDataEnable(HF_FeatureHubConfiguration configuration) {
    inspire::DatabaseConfiguration param = {0};
    param.db_path = (configuration.dbPath != nullptr) ? std::string(configuration.dbPath) : std::string();
    param.enable_use_db = configuration.enablePersistence;
    param.feature_block_num = configuration.featureBlockNum;
    param.recognition_threshold = configuration.searchThreshold;
    param.search_mode = (SearchMode )configuration.searchMode;
    auto ret = FEATURE_HUB->EnableHub(param);

    return ret;
}

HResult HF_FaceContextSetTrackPreviewSize(HContextHandle ctxHandle, HInt32 previewSize) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    return ctx->impl.SetTrackPreviewSize(previewSize);
}

HResult HF_FaceContextSetFaceTrackMode(HContextHandle ctxHandle, HF_DetectMode detectMode) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    inspire::DetectMode detMode = inspire::DETECT_MODE_IMAGE;
    if (detectMode == HF_DETECT_MODE_VIDEO) {
        detMode = inspire::DETECT_MODE_VIDEO;
    }
    return ctx->impl.SetDetectMode(detMode);
}

HResult HF_FaceContextSetFaceDetectThreshold(HContextHandle ctxHandle, HFloat threshold) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    return ctx->impl.SetFaceDetectThreshold(threshold);
}

HResult HF_FaceContextRunFaceTrack(HContextHandle ctxHandle, HImageHandle streamHandle, Ptr_HF_MultipleFaceData results) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (streamHandle == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    auto ret = ctx->impl.FaceDetectAndTrack(stream->impl);
    results->detectedNum = ctx->impl.GetNumberOfFacesCurrentlyDetected();
    results->rects = (HFaceRect *) ctx->impl.GetFaceRectsCache().data();
    results->trackIds = (HInt32 *) ctx->impl.GetTrackIDCache().data();
    results->angles.pitch = (HFloat *) ctx->impl.GetPitchResultsCache().data();
    results->angles.roll = (HFloat *) ctx->impl.GetRollResultsCache().data();
    results->angles.yaw = (HFloat *) ctx->impl.GetYawResultsCache().data();
    results->tokens = (HF_FaceBasicToken *) ctx->impl.GetFaceBasicDataCache().data();

    return ret;
}

HResult HF_CopyFaceBasicToken(HF_FaceBasicToken token, HPBuffer buffer, HInt32 bufferSize) {
    if (bufferSize < sizeof(inspire::HyperFaceData)) {
        return HERR_INVALID_BUFFER_SIZE;
    }
    std::memcpy(buffer, token.data, sizeof(inspire::HyperFaceData));
    return HSUCCEED;
}

HResult HF_GetFaceBasicTokenSize(HPInt32 bufferSize) {
    *bufferSize = sizeof(inspire::HyperFaceData);
    return HSUCCEED;
}

HResult HF_FeatureHubFaceSearchThresholdSetting(float threshold) {
    FEATURE_HUB->SetRecognitionThreshold(threshold);
    return HSUCCEED;
}

HResult HF_FaceFeatureExtract(HContextHandle ctxHandle, HImageHandle streamHandle, HF_FaceBasicToken singleFace, Ptr_HF_FaceFeature feature) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (streamHandle == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    if (singleFace.data == nullptr || singleFace.size <= 0) {
        return HERR_INVALID_FACE_TOKEN;
    }
    inspire::FaceBasicData data;
    data.dataSize = singleFace.size;
    data.data = singleFace.data;
    auto ret = ctx->impl.FaceFeatureExtract(stream->impl, data);
    feature->size = ctx->impl.GetFaceFeatureCache().size();
    feature->data = (HFloat *)ctx->impl.GetFaceFeatureCache().data();

    return ret;
}


HResult HF_FaceFeatureExtractCpy(HContextHandle ctxHandle, HImageHandle streamHandle, HF_FaceBasicToken singleFace, HPFloat feature) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (streamHandle == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    if (singleFace.data == nullptr || singleFace.size <= 0) {
        return HERR_INVALID_FACE_TOKEN;
    }
    inspire::FaceBasicData data;
    data.dataSize = singleFace.size;
    data.data = singleFace.data;
    auto ret = ctx->impl.FaceFeatureExtract(stream->impl, data);
    for (int i = 0; i < ctx->impl.GetFaceFeatureCache().size(); ++i) {
        feature[i] = ctx->impl.GetFaceFeatureCache()[i];
    }

    return ret;
}



HResult HF_FaceComparison1v1(HF_FaceFeature feature1, HF_FaceFeature feature2, HPFloat result) {
    if (feature1.data == nullptr || feature2.data == nullptr) {
        INSPIRE_LOGE("1");
        return HERR_INVALID_FACE_FEATURE;
    }
    if (feature1.size != feature2.size) {
        INSPIRE_LOGE("feature1.size: %d, feature2.size: %d", feature1.size, feature2.size);
        return HERR_INVALID_FACE_FEATURE;
    }
    *result = 0.0f;
    float res = -1.0f;
    auto ret = FEATURE_HUB->CosineSimilarity(feature1.data, feature2.data, feature1.size, res);
    *result = res;

    return ret;
}

HResult HF_GetFeatureLength(HPInt32 num) {
    *num = FEATURE_HUB->GetFeatureNum();

    return HSUCCEED;
}


HResult HF_FeatureHubInsertFeature(HF_FaceFeatureIdentity featureIdentity) {
    if (featureIdentity.feature->data == nullptr) {
        return HERR_INVALID_FACE_FEATURE;
    }
    std::vector<float> feat;
    feat.reserve(featureIdentity.feature->size);
    for (int i = 0; i < featureIdentity.feature->size; ++i) {
        feat.push_back(featureIdentity.feature->data[i]);
    }
    std::string tag(featureIdentity.tag);
    HInt32 ret = FEATURE_HUB->FaceFeatureInsertFromCustomId(feat, tag, featureIdentity.customId);

    return ret;
}


HResult HF_FeatureHubFaceSearch(HF_FaceFeature searchFeature, HPFloat confidence, Ptr_HF_FaceFeatureIdentity mostSimilar) {
    if (searchFeature.data == nullptr) {
        return HERR_INVALID_FACE_FEATURE;
    }
    std::vector<float> feat;
    feat.reserve(searchFeature.size);
    for (int i = 0; i < searchFeature.size; ++i) {
        feat.push_back(searchFeature.data[i]);
    }
    inspire::SearchResult result;
    HInt32 ret = FEATURE_HUB->SearchFaceFeature(feat, result);
    mostSimilar->feature = (HF_FaceFeature* ) FEATURE_HUB->GetFaceFeaturePtrCache().get();
    mostSimilar->feature->data = (HFloat* ) FEATURE_HUB->GetSearchFaceFeatureCache().data();
    mostSimilar->feature->size = FEATURE_HUB->GetSearchFaceFeatureCache().size();
    mostSimilar->tag = FEATURE_HUB->GetStringCache();
    mostSimilar->customId = result.customId;
    *confidence = result.score;

    return ret;
}

HResult HF_FeatureHubFaceRemove(HInt32 customId) {
    auto ret = FEATURE_HUB->FaceFeatureRemoveFromCustomId(customId);
    return ret;
}

HResult HF_FeatureHubFaceUpdate(HF_FaceFeatureIdentity featureIdentity) {
    if (featureIdentity.feature->data == nullptr) {
        return HERR_INVALID_FACE_FEATURE;
    }
    std::vector<float> feat;
    feat.reserve(featureIdentity.feature->size);
    for (int i = 0; i < featureIdentity.feature->size; ++i) {
        feat.push_back(featureIdentity.feature->data[i]);
    }
    std::string tag(featureIdentity.tag);

    auto ret = FEATURE_HUB->FaceFeatureUpdateFromCustomId(feat, tag, featureIdentity.customId);

    return ret;
}

HResult HF_FeatureHubGetFaceIdentity(HInt32 customId, Ptr_HF_FaceFeatureIdentity identity) {
    auto ret = FEATURE_HUB->GetFaceFeatureFromCustomId(customId);
    if (ret == HSUCCEED) {
        identity->tag = FEATURE_HUB->GetStringCache();
        identity->customId = customId;
        identity->feature = (HF_FaceFeature* ) FEATURE_HUB->GetFaceFeaturePtrCache().get();
        identity->feature->data = (HFloat* ) FEATURE_HUB->GetFaceFeaturePtrCache()->data;
        identity->feature->size = FEATURE_HUB->GetFaceFeaturePtrCache()->dataSize;
    } else {
        identity->customId = -1;
    }

    return ret;
}

HResult HF_MultipleFacePipelineProcess(HContextHandle ctxHandle, HImageHandle streamHandle, Ptr_HF_MultipleFaceData faces, HF_ContextCustomParameter parameter) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (streamHandle == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    if (faces->detectedNum <= 0 || faces->tokens->data == nullptr) {
        return HERR_INVALID_FACE_LIST;
    }
    inspire::ContextCustomParameter param;
    param.enable_mask_detect = parameter.enable_mask_detect;
    param.enable_age = parameter.enable_age;
    param.enable_liveness = parameter.enable_liveness;
    param.enable_face_quality = parameter.enable_face_quality;
    param.enable_gender = parameter.enable_gender;
    param.enable_interaction_liveness = parameter.enable_interaction_liveness;
    param.enable_ir_liveness = parameter.enable_ir_liveness;
    param.enable_recognition = parameter.enable_recognition;

    HResult ret;
    std::vector<inspire::HyperFaceData> data;
    data.resize(faces->detectedNum);
    for (int i = 0; i < faces->detectedNum; ++i) {
        auto &face = data[i];
        ret = DeserializeHyperFaceData((char* )faces->tokens[i].data, faces->tokens[i].size, face);
        if (ret != HSUCCEED) {
            return HERR_INVALID_FACE_TOKEN;
        }
    }

    ret = ctx->impl.FacesProcess(stream->impl, data, param);

    return ret;

}

HResult HF_MultipleFacePipelineProcessOptional(HContextHandle ctxHandle, HImageHandle streamHandle, Ptr_HF_MultipleFaceData faces, HInt32 customOption) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (streamHandle == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    if (faces->detectedNum <= 0 || faces->tokens->data == nullptr) {
        return HERR_INVALID_FACE_LIST;
    }

    inspire::ContextCustomParameter param;
    if (customOption & HF_ENABLE_FACE_RECOGNITION) {
        param.enable_recognition = true;
    }
    if (customOption & HF_ENABLE_LIVENESS) {
        param.enable_liveness = true;
    }
    if (customOption & HF_ENABLE_IR_LIVENESS) {
        param.enable_ir_liveness = true;
    }
    if (customOption & HF_ENABLE_AGE_PREDICT) {
        param.enable_age = true;
    }
    if (customOption & HF_ENABLE_GENDER_PREDICT) {
        param.enable_gender = true;
    }
    if (customOption & HF_ENABLE_MASK_DETECT) {
        param.enable_mask_detect = true;
    }
    if (customOption & HF_ENABLE_QUALITY) {
        param.enable_face_quality = true;
    }
    if (customOption & HF_ENABLE_INTERACTION) {
        param.enable_interaction_liveness = true;
    }


    HResult ret;
    std::vector<inspire::HyperFaceData> data;
    data.resize(faces->detectedNum);
    for (int i = 0; i < faces->detectedNum; ++i) {
        auto &face = data[i];
        ret = DeserializeHyperFaceData((char* )faces->tokens[i].data, faces->tokens[i].size, face);
        if (ret != HSUCCEED) {
            return HERR_INVALID_FACE_TOKEN;
        }
    }

    ret = ctx->impl.FacesProcess(stream->impl, data, param);

    return ret;

}

HResult HF_GetRGBLivenessConfidence(HContextHandle ctxHandle, Ptr_HF_RGBLivenessConfidence confidence) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }

    confidence->num = ctx->impl.GetRgbLivenessResultsCache().size();
    confidence->confidence = (HFloat* )ctx->impl.GetRgbLivenessResultsCache().data();

    return HSUCCEED;
}

HResult HF_GetFaceMaskConfidence(HContextHandle ctxHandle, Ptr_HF_FaceMaskConfidence confidence) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }

    confidence->num = ctx->impl.GetMaskResultsCache().size();
    confidence->confidence = (HFloat* )ctx->impl.GetMaskResultsCache().data();

    return HSUCCEED;
}

HResult HF_FaceQualityDetect(HContextHandle ctxHandle, HF_FaceBasicToken singleFace, HFloat *confidence) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }

    inspire::FaceBasicData data;
    data.dataSize = singleFace.size;
    data.data = singleFace.data;

    auto ret = inspire::FaceContext::FaceQualityDetect(data, *confidence);

    return ret;

}

HResult HF_FeatureHubGetFaceCount(HInt32* count) {
    *count = FEATURE_HUB->GetFaceFeatureCount();
    return HSUCCEED;
}

HResult HF_FeatureHubViewDBTable() {
    return FEATURE_HUB->ViewDBTable();
}


HResult HF_QueryInspireFaceVersion(Ptr_HF_InspireFaceVersion version) {
    version->major = std::stoi(INSPIRE_FACE_VERSION_MAJOR_STR);
    version->minor = std::stoi(INSPIRE_FACE_VERSION_MINOR_STR);
    version->patch = std::stoi(INSPIRE_FACE_VERSION_PATCH_STR);

    return HSUCCEED;
}

HResult HF_SetLogLevel(HF_LogLevel level) {
    INSPIRE_SET_LOG_LEVEL(LogLevel(level));
    return HSUCCEED;
}

HResult HF_LogDisable() {
    INSPIRE_SET_LOG_LEVEL(inspire::LOG_NONE);
    return HSUCCEED;
}