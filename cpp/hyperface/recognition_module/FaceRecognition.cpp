//
// Created by tunm on 2023/9/8.
//

#include "FaceRecognition.h"
#include "model_index.h"
#include "simd.h"
#include "recognition_module/extract/Alignment.h"
#include "track_module/landmark/FaceLandmark.h"
#include "herror.h"

namespace hyper {

FaceRecognition::FaceRecognition(ModelLoader &loader, bool enable_recognition) {
    if (enable_recognition) {
        auto ret = InitExtractInteraction(loader.ReadModel(ModelIndex::_03_extract));
        if (ret != 0) {
            LOGE("FaceRecognition error.");
        }
    }

}

int32_t FaceRecognition::InitExtractInteraction(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_03_extract);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"fc1_scale", });
    param.set<vector<int>>("input_size", {112, 112});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125, 0.0078125, 0.0078125});
    m_extract_ = make_shared<Extract>();
    m_extract_->LoadParam(param, model);

    return HSUCCEED;
}

int32_t FaceRecognition::CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2, float &res) {
    if (v1.size() != v2.size() || v1.empty()) {
        return HERR_CTX_REC_CONTRAST_FEAT_ERR; // 无法计算相似性
    }
    // 计算余弦相似性
    res = simd_dot(v1.data(), v2.data(), v1.size());
    return HSUCCEED;
}

int32_t FaceRecognition::FaceExtract(CameraStream &image, const FaceObject &face, Embedded &embedded) {
    if (m_extract_ == nullptr) {
        return HERR_CTX_REC_EXTRACT_FAILURE;
    }

    auto lmk = face.landmark_;
    vector<cv::Point2f> lmk_5 = {lmk[FaceLandmark::LEFT_EYE_CENTER],
                                 lmk[FaceLandmark::RIGHT_EYE_CENTER],
                                 lmk[FaceLandmark::NOSE_CORNER],
                                 lmk[FaceLandmark::MOUTH_LEFT_CORNER],
                                 lmk[FaceLandmark::MOUTH_RIGHT_CORNER]};
    auto trans = getTransformMatrix112(lmk_5);
    trans.convertTo(trans, CV_64F);
    auto crop = image.GetAffineRGBImage(trans, 112, 112);
    embedded = (*m_extract_)(crop);

    return 0;
}

} // namespace hyper
