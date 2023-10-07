//
// Created by Tunm-Air13 on 2023/9/7.
//

#include "face_context.h"
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
            param.enable_interaction_liveness
    );


    return HSUCCEED;
}

int32_t FaceContext::FaceDetectAndTrack(CameraStream &image) {
    std::vector<ByteArray>().swap(m_detect_cache_);
    std::vector<FaceBasicData>().swap(m_face_basic_data_cache_);
    std::vector<FaceRect>().swap(m_face_rects_cache_);
    std::vector<FacePoseQualityResult>().swap(m_quality_results_cache_);
    std::vector<float>().swap(m_roll_results_cache_);
    std::vector<float>().swap(m_yaw_results_cache_);
    std::vector<float>().swap(m_pitch_results_cache_);
    if (m_face_track_ == nullptr) {
        return HERR_CTX_TRACKER_FAILURE;
    }
    m_face_track_->UpdateStream(image, m_always_detect_);
    for (int i = 0; i < m_face_track_->trackingFace.size(); ++i) {
        auto &face = m_face_track_->trackingFace[i];
        HyperFaceData data = FaceObjectToHyperFaceData(face, i);
        ByteArray byteArray;
        auto ret = SerializeHyperFaceData(data, byteArray);
        m_detect_cache_.push_back(byteArray);
        assert(ret == HSUCCEED);
        m_face_rects_cache_.push_back(data.rect);
        m_quality_results_cache_.push_back(face.high_result);
        m_roll_results_cache_.push_back(face.high_result.roll);
        m_yaw_results_cache_.push_back(face.high_result.yaw);
        m_pitch_results_cache_.push_back(face.high_result.pitch);
    }
    // ptr face_basic
    m_face_basic_data_cache_.resize(m_face_track_->trackingFace.size());
    for (int i = 0; i < m_face_basic_data_cache_.size(); ++i) {
        auto &basic = m_face_basic_data_cache_[i];
        basic.dataSize = m_detect_cache_[i].size();
        basic.data = m_detect_cache_[i].data();
    }

//    LOGD("跟踪COST: %f", m_face_track_->GetTrackTotalUseTime());
    return HSUCCEED;
}

FaceObjectList& FaceContext::GetTrackingFaceList() {
    return m_face_track_->trackingFace;
}

const std::shared_ptr<FaceRecognition>& FaceContext::FaceRecognitionModule() {
    return m_face_recognition_;
}

const std::shared_ptr<FacePipeline>& FaceContext::FacePipelineModule() {
    return m_face_pipeline_;
}


const int32_t FaceContext::GetNumberOfFacesCurrentlyDetected() const {
    return m_face_track_->trackingFace.size();
}

int32_t FaceContext::FacesProcess(CameraStream &image, const std::vector<HyperFaceData> &faces, const CustomPipelineParameter &param) {
    m_mask_results_cache_.resize(faces.size(), -1.0f);
    m_rgb_liveness_results_cache_.resize(faces.size(), -1.0f);
    for (int i = 0; i < faces.size(); ++i) {
        const auto &face = faces[i];
        // RGB活体检测
        if (param.enable_liveness) {
            auto ret = m_face_pipeline_->Process(image, face, PROCESS_RGB_LIVENESS);
            if (ret != HSUCCEED) {
                return ret;
            }
            m_rgb_liveness_results_cache_[i] = m_face_pipeline_->faceLivenessCache;
        }
        // 口罩检测
        if (param.enable_mask_detect) {
            auto ret = m_face_pipeline_->Process(image, face, PROCESS_MASK);
            if (ret != HSUCCEED) {
                return ret;
            }
            m_mask_results_cache_[i] = m_face_pipeline_->faceMaskCache;
        }
        // 年龄预测
        if (param.enable_age) {
            auto ret = m_face_pipeline_->Process(image, face, PROCESS_AGE);
            if (ret != HSUCCEED) {
                return ret;
            }
        }
        // 性别预测
        if (param.enable_age) {
            auto ret = m_face_pipeline_->Process(image, face, PROCESS_GENDER);
            if (ret != HSUCCEED) {
                return ret;
            }
        }

    }

    return 0;
}


const std::vector<ByteArray>& FaceContext::GetDetectCache() const {
    return m_detect_cache_;
}

const std::vector<FaceBasicData>& FaceContext::GetFaceBasicDataCache() const {
    return m_face_basic_data_cache_;
}

const std::vector<FaceRect>& FaceContext::GetFaceRectsCache() const {
    return m_face_rects_cache_;
}

const std::vector<float>& FaceContext::GetRollResultsCache() const {
    return m_roll_results_cache_;
}

const std::vector<float>& FaceContext::GetYawResultsCache() const {
    return m_yaw_results_cache_;
}

const std::vector<float>& FaceContext::GetPitchResultsCache() const {
    return m_pitch_results_cache_;
}

const std::vector<FacePoseQualityResult>& FaceContext::GetQualityResultsCache() const {
    return m_quality_results_cache_;
}

const std::vector<float>& FaceContext::GetMaskResultsCache() const {
    return m_mask_results_cache_;
}

const std::vector<float>& FaceContext::GetRgbLivenessResultsCache() const {
    return m_rgb_liveness_results_cache_;
}


}   // namespace hyper