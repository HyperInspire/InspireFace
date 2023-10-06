//
// Created by tunm on 2023/9/7.
//

#ifndef HYPERFACEREPO_FACEPIPELINE_H
#define HYPERFACEREPO_FACEPIPELINE_H

#include "middleware/camera_stream/camera_stream.h"
#include "common/face_info/face_object.h"
#include "attribute/all.h"
#include "liveness/all.h"
#include "middleware/model_loader/model_loader.h"
#include "common/face_data/data_tools.h"

namespace hyper {

typedef enum FaceProcessFunction {
    PROCESS_MASK = 0,               // 口罩检测
    PROCESS_RGB_LIVENESS,           // RGB活体检测
    PROCESS_AGE,                    // 年龄检测
    PROCESS_GENDER,                 // 性别检测
} FaceProcessFunction;

class FacePipeline {
public:
    explicit FacePipeline(ModelLoader &loader, bool enableLiveness, bool enableMaskDetect, bool enableAge,
                          bool enableGender, bool enableInteractionLiveness);

    int32_t Process(CameraStream &image, FaceObject &face);

    int32_t Process(CameraStream &image, const HyperFaceData &face, FaceProcessFunction proc);



private:

    int32_t InitAgePredict(Model *model);

    int32_t InitGenderPredict(Model *model);

    int32_t InitMaskPredict(Model *model);

    int32_t InitRBGAntiSpoofing(Model *model);

    int32_t InitLivenessInteraction(Model *model);

private:

    const bool m_enable_liveness_ = false;                 ///< RGB活体检测功能
    const bool m_enable_mask_detect_ = false;              ///< 口罩检测功能
    const bool m_enable_age_ = false;                      ///< 年龄预测功能
    const bool m_enable_gender_ = false;                   ///< 性别预测功能
    const bool m_enable_interaction_liveness_ = false;     ///< 配合活体检测功能

    std::shared_ptr<AgePredict> m_age_predict_;

    std::shared_ptr<GenderPredict> m_gender_predict_;

    std::shared_ptr<MaskPredict> m_mask_predict_;

    std::shared_ptr<RBGAntiSpoofing> m_rgb_anti_spoofing_;

    std::shared_ptr<LivenessInteraction> m_liveness_interaction_spoofing_;

public:

    float faceMaskCache;

    float faceLivenessCache;


};

}

#endif //HYPERFACEREPO_FACEPIPELINE_H
