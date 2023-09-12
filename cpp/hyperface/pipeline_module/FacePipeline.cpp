//
// Created by tunm on 2023/9/7.
//

#include "FacePipeline.h"
#include "model_index.h"
#include "log.h"
#include "track_module/landmark/FaceLandmark.h"
#include "recognition_module/extract/Alignment.h"

namespace hyper {

FacePipeline::FacePipeline(ModelLoader &loader, bool enableLiveness, bool enableMaskDetect, bool enableAge,
                           bool enableGender, bool enableInteractionLiveness)
        : m_enable_liveness_(enableLiveness),
          m_enable_mask_detect_(enableMaskDetect),
          m_enable_age_(enableAge),
          m_enable_gender_(enableGender),
          m_enable_interaction_liveness_(enableInteractionLiveness){

    if (m_enable_age_) {
        auto ret = InitAgePredict(loader.ReadModel(0));
        if (ret != 0) {
            LOGE("InitAgePredict error.");
        }
    }

    // 初始化性别预测模型（假设Index为0）
    if (m_enable_gender_) {
        auto ret = InitGenderPredict(loader.ReadModel(0));
        if (ret != 0) {
            LOGE("InitGenderPredict error.");
        }
    }

    // 初始化口罩检测模型（假设Index为0）
    if (m_enable_mask_detect_) {
        auto ret = InitMaskPredict(loader.ReadModel(0));
        if (ret != 0) {
            LOGE("InitMaskPredict error.");
        }
    }

    // 初始化RGB活体检测模型（假设Index为0）
    if (m_enable_liveness_) {
        auto ret = InitRBGAntiSpoofing(loader.ReadModel(0));
        if (ret != 0) {
            LOGE("InitRBGAntiSpoofing error.");
        }
    }

    // 初始化配合活体检测的模型（假设Index为0）
    if (m_enable_interaction_liveness_) {
        auto ret = InitLivenessInteraction(loader.ReadModel(0));
        if (ret != 0) {
            LOGE("InitLivenessInteraction error.");
        }
    }

}


int32_t FacePipeline::operator()(CameraStream &image, FaceObject &face) {
    // 跟踪状态下计次达到要求 或 处于检测状态 执行pipeline
    auto lmk = face.landmark_;
    vector<cv::Point2f> lmk_5 = {lmk[FaceLandmark::LEFT_EYE_CENTER],
                                 lmk[FaceLandmark::RIGHT_EYE_CENTER],
                                 lmk[FaceLandmark::NOSE_CORNER],
                                 lmk[FaceLandmark::MOUTH_LEFT_CORNER],
                                 lmk[FaceLandmark::MOUTH_RIGHT_CORNER]};
    auto trans = getTransformMatrix112(lmk_5);
    trans.convertTo(trans, CV_64F);
    auto align112x = image.GetAffineRGBImage(trans, 112, 112);
    if (!m_enable_mask_detect_) {
        auto mask_score = (*m_mask_predict_)(align112x);
    }

    return 0;
}


int32_t FacePipeline::InitAgePredict(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_03_extract);
    param.set<string>("input_layer", "input_1");
    param.set<vector<string>>("outputs_layers", {"activation_1/Softmax", });
    param.set<vector<int>>("input_size", {96, 96});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {0.003921568627f, 0.003921568627f, 0.003921568627f});
    param.set<bool>("swap_color", true);        // RGB mode
    m_mask_predict_ = make_shared<MaskPredict>();
    m_mask_predict_->LoadParam(param, model);

    return 0;
}

int32_t FacePipeline::InitGenderPredict(Model *model) {
    return 0;
}

int32_t FacePipeline::InitMaskPredict(Model *model) {

    return 0;
}

int32_t FacePipeline::InitRBGAntiSpoofing(Model *model) {
    return 0;
}

int32_t FacePipeline::InitLivenessInteraction(Model *model) {
    return 0;
}



}