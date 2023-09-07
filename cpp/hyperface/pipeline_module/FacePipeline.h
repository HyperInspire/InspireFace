//
// Created by tunm on 2023/9/7.
//

#ifndef HYPERFACEREPO_FACEPIPELINE_H
#define HYPERFACEREPO_FACEPIPELINE_H

#include "recognition/Recognition.h"

namespace hyper {

typedef struct CustomPipelineParameter {
    bool enable_recognition = true;               ///< 人脸识别功能
    bool enable_liveness = false;                 ///< RBG活体检测功能
    bool enable_ir_liveness = false;              ///< IR活体检测功能
    bool enable_mask_detect = false;              ///< 口罩检测功能
    bool enable_age = false;                      ///< 年龄预测功能
    bool enable_gender = false;                   ///< 性别预测功能
    bool enable_face_quality = false;             ///< 人脸质量检测功能
    bool enable_interaction_liveness = false;     ///< 配合活体检测功能
    int recognition_interval_frame = 10;          ///< 人脸识别特征抽取间隔帧

} ContextCustomParameter;

class FacePipeline {
public:
    explicit FacePipeline(ModelLoader &loader, CustomPipelineParameter param);

private:

    int32_t InitRecognition(Model *model);

private:

    ContextCustomParameter m_param_;

    shared_ptr<Recognition> m_recognition_;

};

}

#endif //HYPERFACEREPO_FACEPIPELINE_H
