//
// Created by tunm on 2023/10/3.
//

#include "inspireface.h"
#include "intypedef.h"
#include "inspireface_internal.h"
#include "information.h"

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
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
    }
    HF_CameraStream *stream = (HF_CameraStream* ) streamHandle;
    if (stream == nullptr) {
        return HERR_INVALID_IMAGE_STREAM_HANDLE;
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


HResult HF_ReleaseFaceContext(HContextHandle handle) {
    if (handle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) handle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    } else {
        delete ctx;
        ctx = nullptr;
    }

    return HSUCCEED;
}

HResult HF_CreateFaceContextFromResourceFile(HString resourceFile, HF_ContextCustomParameter parameter, HF_DetectMode detectMode, HInt32 maxDetectFaceNum, HContextHandle *handle) {
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

    std::string path(resourceFile);
    HF_FaceContext *ctx = new HF_FaceContext();
    auto ret = ctx->impl.Configuration(path, detMode, maxDetectFaceNum, param);
    if (ret != HSUCCEED) {
        delete ctx;
        ctx = nullptr;
    } else {
        *handle = ctx;
    }


    return ret;
}

HResult HF_CreateFaceContextFromResourceFileOptional(HString resourceFile,HInt32 customOption, HF_DetectMode detectMode, HInt32 maxDetectFaceNum, HContextHandle *handle) {
    inspire::ContextCustomParameter param;
    if (customOption & HF_ENABLE_FACE_RECOGNITION) {
        param.enable_recognition = true;
    }
    if (customOption & HF_ENABLE_LIVENESS) {
        param.enable_ir_liveness = true;
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

    std::string path(resourceFile);
    HF_FaceContext *ctx = new HF_FaceContext();
    auto ret = ctx->impl.Configuration(path, detMode, maxDetectFaceNum, param);
    if (ret != HSUCCEED) {
        delete ctx;
        ctx = nullptr;
    } else {
        *handle = ctx;
    }

    return ret;
}

HResult HF_FaceContextDataPersistence(HContextHandle ctxHandle, HF_DatabaseConfiguration configuration) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    inspire::DatabaseConfiguration param = {0};
    param.db_path = std::string(configuration.dbPath);
    param.enable_use_db = configuration.enableUseDb;
    auto ret = ctx->impl.DataPersistenceConfiguration(param);

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



HResult HF_FaceComparison1v1(HContextHandle ctxHandle, HF_FaceFeature feature1, HF_FaceFeature feature2, HPFloat result) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (feature1.data == nullptr || feature2.data == nullptr) {
        return HERR_INVALID_FACE_FEATURE;
    }
    if (feature1.size != feature2.size) {
        return HERR_INVALID_FACE_FEATURE;
    }
    float res = -1.0f;
    auto ret = ctx->impl.FaceRecognitionModule()->CosineSimilarity(feature1.data, feature2.data, feature1.size, res);
    *result = res;

    return ret;
}

HResult HF_GetFeatureLength(HContextHandle ctxHandle, HPInt32 num) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    *num = ctx->impl.FaceRecognitionModule()->GetFeatureNum();

    return HSUCCEED;
}


HResult HF_FeaturesGroupInsertFeature(HContextHandle ctxHandle, HF_FaceFeatureIdentity featureIdentity) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    auto *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (featureIdentity.feature->data == nullptr) {
        return HERR_INVALID_FACE_FEATURE;
    }
    std::vector<float> feat;
    feat.reserve(featureIdentity.feature->size);
    for (int i = 0; i < featureIdentity.feature->size; ++i) {
        feat.push_back(featureIdentity.feature->data[i]);
    }
    std::string tag(featureIdentity.tag);
    HInt32 ret = ctx->impl.FaceFeatureInsertFromCustomId(feat, tag, featureIdentity.customId);

    return ret;
}


HResult HF_FeaturesGroupFeatureSearch(HContextHandle ctxHandle, HF_FaceFeature searchFeature, HPFloat confidence, Ptr_HF_FaceFeatureIdentity mostSimilar) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    auto *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (searchFeature.data == nullptr) {
        return HERR_INVALID_FACE_FEATURE;
    }
    std::vector<float> feat;
    feat.reserve(searchFeature.size);
    for (int i = 0; i < searchFeature.size; ++i) {
        feat.push_back(searchFeature.data[i]);
    }
    inspire::SearchResult result;
    HInt32 ret = ctx->impl.SearchFaceFeature(feat, result);
    mostSimilar->feature = (HF_FaceFeature* )ctx->impl.GetFaceFeaturePtrCache().get();
    mostSimilar->feature->data = (HFloat* )ctx->impl.GetSearchFaceFeatureCache().data();
    mostSimilar->feature->size = ctx->impl.GetSearchFaceFeatureCache().size();
    mostSimilar->tag = ctx->impl.GetStringCache();
    mostSimilar->customId = result.customId;
    *confidence = result.score;

    return ret;
}

HResult HF_FeaturesGroupFeatureRemove(HContextHandle ctxHandle, HInt32 customId) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    auto *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }

    auto ret = ctx->impl.FaceFeatureRemoveFromCustomId(customId);

    return ret;
}

HResult HF_FeaturesGroupFeatureUpdate(HContextHandle ctxHandle, HF_FaceFeatureIdentity featureIdentity) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    auto *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    if (featureIdentity.feature->data == nullptr) {
        return HERR_INVALID_FACE_FEATURE;
    }
    std::vector<float> feat;
    feat.reserve(featureIdentity.feature->size);
    for (int i = 0; i < featureIdentity.feature->size; ++i) {
        feat.push_back(featureIdentity.feature->data[i]);
    }
    std::string tag(featureIdentity.tag);

    auto ret = ctx->impl.FaceFeatureUpdateFromCustomId(feat, tag, featureIdentity.customId);

    return ret;
}

HResult HF_FeaturesGroupGetFeatureIdentity(HContextHandle ctxHandle, HInt32 customId, Ptr_HF_FaceFeatureIdentity identity) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    auto *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    auto ret = ctx->impl.GetFaceFeatureFromCustomId(customId);
    identity->feature = (HF_FaceFeature* )ctx->impl.GetFaceFeaturePtrCache().get();
    identity->feature->data = (HFloat* )ctx->impl.GetSearchFaceFeatureCache().data();
    identity->feature->size = ctx->impl.GetSearchFaceFeatureCache().size();

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
        param.enable_ir_liveness = true;
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

HResult HF_FaceRecognitionThresholdSetting(HContextHandle ctxHandle, float threshold) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    return ctx->impl.SetRecognitionThreshold(threshold);
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

HResult HF_FeatureGroupGetCount(HContextHandle ctxHandle, HInt32* count) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }

    *count = ctx->impl.FaceRecognitionModule()->GetFaceFeatureCount();

    return HSUCCEED;
}

HResult HF_ViewFaceDBTable(HContextHandle ctxHandle) {
    if (ctxHandle == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }
    HF_FaceContext *ctx = (HF_FaceContext* ) ctxHandle;
    if (ctx == nullptr) {
        return HERR_INVALID_CONTEXT_HANDLE;
    }

    return ctx->impl.ViewDBTable();
}


HResult HF_QueryInspireFaceVersion(Ptr_HF_InspireFaceVersion version) {
    version->major = std::stoi(INSPIRE_FACE_VERSION_MAJOR_STR);
    version->minor = std::stoi(INSPIRE_FACE_VERSION_MINOR_STR);
    version->patch = std::stoi(INSPIRE_FACE_VERSION_PATCH_STR);

    return HSUCCEED;
}