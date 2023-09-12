//
// Created by Tunm-Air13 on 2023/9/11.
//

#include "FeatureBlock.h"
#include "log.h"
#include "recognition_module/features_block/implement/FeatureBlockNone.h"

#ifdef FEATURE_BLOCK_ENABLE_OPENCV
#include "recognition_module/features_block/implement/FeatureBlockOpenCV.h"
#endif

namespace hyper {


FeatureBlock *FeatureBlock::Create(const MatrixCore crop_type, int32_t features_max, int32_t feature_length) {
    FeatureBlock* p = nullptr;
    switch (crop_type) {
#ifdef FEATURE_BLOCK_ENABLE_OPENCV
        case MC_OPENCV:
            p = new FeatureBlockOpenCV(features_max, feature_length);
            break;
#endif
#ifdef FEATURE_BLOCK_ENABLE_EIGEN
        case MC_EIGEN:
            LOGD("Not Implement");
            break;
#endif
        case MC_NONE:
            LOGD("Not Implement");
            break;
    }

    if (p != nullptr) {
        p->m_matrix_core_ = crop_type;
        p->m_features_max_ = features_max;          // 人脸特征数量
        p->m_feature_length_ = feature_length;      // 人脸特征长度（默认512）
        p->m_feature_state_.resize(features_max, FEATURE_STATE::IDLE);
        p->m_tag_list_.resize(features_max, "None");
    } else {
        LOGE("Create FeatureBlock error.");
    }

    return p;
}

}   // namespace hyper