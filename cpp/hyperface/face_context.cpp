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

    m_face_feature_ptr_cache_ = std::make_shared<FaceFeatureEntity>();



    return HSUCCEED;
}

int32_t FaceContext::FaceDetectAndTrack(CameraStream &image) {
    std::vector<ByteArray>().swap(m_detect_cache_);
    std::vector<FaceBasicData>().swap(m_face_basic_data_cache_);
    std::vector<FaceRect>().swap(m_face_rects_cache_);
    std::vector<int32_t>().swap(m_track_id_cache_);
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
        m_track_id_cache_.push_back(face.GetTrackingId());
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

const std::vector<int32_t>& FaceContext::GetTrackIDCache() const {
    return m_track_id_cache_;
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

const Embedded& FaceContext::GetFaceFeatureCache() const {
    return m_face_feature_cache_;
}

const Embedded& FaceContext::GetSearchFaceFeatureCache() const {
    return m_search_face_feature_cache_;
}

char *FaceContext::GetStringCache() {
    return m_string_cache_;
}

const std::shared_ptr<FaceFeaturePtr>& FaceContext::GetFaceFeaturePtrCache() const {
    return m_face_feature_ptr_cache_;
}


int32_t FaceContext::FaceFeatureExtract(CameraStream &image, FaceBasicData& data) {
    int32_t ret;
    HyperFaceData face = {0};
    ret = DeserializeHyperFaceData((char* )data.data, data.dataSize, face);
    if (ret != HSUCCEED) {
        return ret;
    }
    Embedded().swap(m_face_feature_cache_);
    ret = m_face_recognition_->FaceExtract(image, face, m_face_feature_cache_);

    return ret;
}

int32_t FaceContext::SearchFaceFeature(const Embedded &queryFeature, SearchResult &searchResult) {
    Embedded().swap(m_search_face_feature_cache_);
    std::memset(m_string_cache_, 0, sizeof(m_string_cache_)); // 初始化为0
    auto ret = m_face_recognition_->SearchFaceFeature(queryFeature, searchResult, m_recognition_threshold_, m_search_most_similar_);
    if (ret == HSUCCEED) {
        ret = m_face_recognition_->GetFaceFeature(searchResult.index, m_search_face_feature_cache_);
        // 确保不会出现缓冲区溢出
        size_t copy_length = std::min(searchResult.tag.size(), sizeof(m_string_cache_) - 1);
        std::strncpy(m_string_cache_, searchResult.tag.c_str(), copy_length);
        // 确保字符串以空字符结束
        m_string_cache_[copy_length] = '\0';
    }

    return ret;
}

int32_t FaceContext::FaceFeatureRemoveFromCustomId(int32_t customId) {
    auto index = m_face_recognition_->FindFeatureIndexByCustomId(customId);
    if (index == -1) {
        return HERR_CTX_REC_INVALID_INDEX;
    }
    auto ret = m_face_recognition_->DeleteFaceFeature(index);

    return ret;
}

int32_t FaceContext::FaceFeatureUpdateFromCustomId(const std::vector<float> &feature, const std::string &tag,
                                                   int32_t customId) {
    auto index = m_face_recognition_->FindFeatureIndexByCustomId(customId);
    if (index == -1) {
        return HERR_CTX_REC_INVALID_INDEX;
    }
    auto ret = m_face_recognition_->UpdateFaceFeature(feature, index, tag, customId);

    return ret;
}

int32_t FaceContext::GetFaceFeatureFromCustomId(int32_t customId) {
    auto index = m_face_recognition_->FindFeatureIndexByCustomId(customId);
    if (index == -1) {
        return HERR_CTX_REC_INVALID_INDEX;
    }
    auto ret = m_face_recognition_->GetFaceFeature(index, m_search_face_feature_cache_);

    return ret;
}

const CustomPipelineParameter &FaceContext::getMParameter() const {
    return m_parameter_;
}


}   // namespace hyper