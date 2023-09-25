//
// Created by tunm on 2023/9/7.
//

#include "FacePipeline.h"
#include "model_index.h"
#include "log.h"
#include "track_module/landmark/FaceLandmark.h"
#include "recognition_module/extract/Alignment.h"
#include "herror.h"

namespace hyper {

FacePipeline::FacePipeline(ModelLoader &loader, bool enableLiveness, bool enableMaskDetect, bool enableAge,
                           bool enableGender, bool enableInteractionLiveness)
        : m_enable_liveness_(enableLiveness),
          m_enable_mask_detect_(enableMaskDetect),
          m_enable_age_(enableAge),
          m_enable_gender_(enableGender),
          m_enable_interaction_liveness_(enableInteractionLiveness) {

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
        auto ret = InitMaskPredict(loader.ReadModel(ModelIndex::_05_mask));
        if (ret != 0) {
            LOGE("InitMaskPredict error.");
        }
    }

    // 初始化RGB活体检测模型（假设Index为0）
    if (m_enable_liveness_) {
        auto ret = InitRBGAntiSpoofing(loader.ReadModel(ModelIndex::_06_msafa27));
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


int32_t FacePipeline::Process(CameraStream &image, const HyperFaceData &face, FaceProcessFunction proc) {
    switch (proc) {
        case PROCESS_MASK: {
            if (m_mask_predict_ == nullptr) {
                return HERR_CTX_PIPELINE_FAILURE;       // 未初始化
            }
            std::vector<cv::Point2f> pointsFive;
            for (const auto &p: face.keyPoints) {
                pointsFive.push_back(HPointToPoint2f(p));
            }
            auto trans = getTransformMatrix112(pointsFive);
            trans.convertTo(trans, CV_64F);
            auto crop = image.GetAffineRGBImage(trans, 112, 112);
            auto mask_score = (*m_mask_predict_)(crop);
            faceMaskCache = mask_score;
            break;
        }
        case PROCESS_RGB_LIVENESS: {
            if (m_rgb_anti_spoofing_ == nullptr) {
                return HERR_CTX_PIPELINE_FAILURE;       // 未初始化
            }
            std::vector<cv::Point2f> pointsFive;
            for (const auto &p: face.keyPoints) {
                pointsFive.push_back(HPointToPoint2f(p));
            }
            auto trans27 = getTransformMatrixSafas(pointsFive);
            trans27.convertTo(trans27, CV_64F);
            auto align112x27 = image.GetAffineRGBImage(trans27, 112, 112);
            auto score = (*m_rgb_anti_spoofing_)(align112x27);
            faceLivenessCache = score;
            break;
        }
        case PROCESS_AGE: {
            if (m_age_predict_ == nullptr) {
                return HERR_CTX_PIPELINE_FAILURE;       // 未初始化
            }
            break;
        }
        case PROCESS_GENDER: {
            if (m_gender_predict_ == nullptr) {
                return HERR_CTX_PIPELINE_FAILURE;       // 未初始化
            }
            break;
        }
    }
    return HSUCCEED;
}

int32_t FacePipeline::Process(CameraStream &image, FaceObject &face) {
    // 跟踪状态下计次达到要求 或 处于检测状态 执行pipeline
    auto lmk = face.landmark_;
    std::vector<cv::Point2f> lmk_5 = {lmk[FaceLandmark::LEFT_EYE_CENTER],
                                 lmk[FaceLandmark::RIGHT_EYE_CENTER],
                                 lmk[FaceLandmark::NOSE_CORNER],
                                 lmk[FaceLandmark::MOUTH_LEFT_CORNER],
                                 lmk[FaceLandmark::MOUTH_RIGHT_CORNER]};
    auto trans = getTransformMatrix112(lmk_5);
    trans.convertTo(trans, CV_64F);
    auto align112x = image.GetAffineRGBImage(trans, 112, 112);
    if (m_mask_predict_ != nullptr) {
        auto mask_score = (*m_mask_predict_)(align112x);
        if (mask_score > 0.95) {
            face.faceProcess.maskInfo = MaskInfo::MASKED;
        } else {
            face.faceProcess.maskInfo = MaskInfo::UNMASKED;
        }
    }

    if (m_rgb_anti_spoofing_ != nullptr) {
        auto trans27 = getTransformMatrixSafas(lmk_5);
        trans27.convertTo(trans27, CV_64F);
        auto align112x27 = image.GetAffineRGBImage(trans27, 112, 112);
        auto score = (*m_rgb_anti_spoofing_)(align112x27);
        if (score > 0.88) {
            face.faceProcess.rgbLivenessInfo = RGBLivenessInfo::LIVENESS_REAL;
        } else {
            face.faceProcess.rgbLivenessInfo = RGBLivenessInfo::LIVENESS_FAKE;
        }
    }

    return HSUCCEED;
}


int32_t FacePipeline::InitAgePredict(Model *model) {

    return 0;
}

int32_t FacePipeline::InitGenderPredict(Model *model) {
    return 0;
}

int32_t FacePipeline::InitMaskPredict(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_05_mask);
    param.set<std::string>("input_layer", "input_1");
    param.set<std::vector<std::string>>("outputs_layers", {"activation_1/Softmax",});
    param.set<std::vector<int>>("input_size", {96, 96});
    param.set<std::vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<std::vector<float>>("norm", {0.003921568627f, 0.003921568627f, 0.003921568627f});
    param.set<bool>("swap_color", true);        // RGB mode
    m_mask_predict_ = std::make_shared<MaskPredict>();
    m_mask_predict_->LoadParam(param, model);
    return 0;
}

int32_t FacePipeline::InitRBGAntiSpoofing(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_06_msafa27);
    param.set<std::string>("input_layer", "data");
    param.set<std::vector<std::string>>("outputs_layers", {"softmax",});
    param.set<std::vector<int>>("input_size", {112, 112});
    param.set<std::vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<std::vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<bool>("swap_color", true);        // RGB mode
    m_rgb_anti_spoofing_ = std::make_shared<RBGAntiSpoofing>();
    m_rgb_anti_spoofing_->LoadParam(param, model);
    return 0;
}

int32_t FacePipeline::InitLivenessInteraction(Model *model) {
    return 0;
}


}