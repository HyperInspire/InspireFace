//
// Created by Tunm-Air13 on 2023/9/7.
//

#include "FaceContext.h"
#include "log.h"
#include "herror.h"

namespace hyper {


FaceContext::FaceContext() = default;

int32_t FaceContext::Configuration(const String &model_file_path, DetectMode detect_mode, int32_t max_detect_face,
                                   CustomPipelineParameter param) {
    m_detect_mode_ = detect_mode;
    m_max_detect_face_ = max_detect_face;
    m_parameter_ = param;
    ModelLoader loader(model_file_path);
    if (loader.GetStatusCode() != 0) {
        LOGE("Model loading error.");
        return HERR_CTX_INVALID_RESOURCE;
    }
    m_face_track_ = std::make_shared<FaceTrack>(m_max_detect_face_);
    m_face_track_->Configuration(loader);
    if (m_detect_mode_ == DetectMode::DETECT_MODE_IMAGE) {
        m_always_detect_ = true;
    }

    m_face_recognition_ = std::make_shared<FaceRecognition>(loader, m_parameter_.enable_recognition);
    m_face_pipeline_ = std::make_shared<FacePipeline>(
            loader,
            param.enable_liveness,
            param.enable_mask_detect,
            param.enable_age,
            param.enable_gender,
            param.enable_interaction_liveness,
            param.enable_face_quality
    );

    return HSUCCEED;
}

int32_t FaceContext::FaceDetectAndTrack(CameraStream &image) {
    if (m_face_track_ == nullptr) {
        return HERR_CTX_TRACKER_FAILURE;
    }
    m_face_track_->UpdateStream(image, m_always_detect_);


    return HSUCCEED;
}

FaceObjectList& FaceContext::GetTrackingFaceList() {
    return m_face_track_->trackingFace;
}

const shared_ptr<FaceRecognition>& FaceContext::FaceRecognitionModule() {
    return m_face_recognition_;
}

const shared_ptr<FacePipeline>& FaceContext::FacePipelineModule() {
    return m_face_pipeline_;
}

}   // namespace hyper