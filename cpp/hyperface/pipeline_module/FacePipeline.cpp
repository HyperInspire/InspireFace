//
// Created by tunm on 2023/9/7.
//

#include "FacePipeline.h"
#include "model_index.h"
#include "log.h"

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


    return 0;
}


int32_t FacePipeline::InitAgePredict(Model *model) {
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