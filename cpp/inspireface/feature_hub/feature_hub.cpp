//
// Created by tunm on 2023/9/8.
//

#include "feature_hub.h"
#include "simd.h"
#include "recognition_module/extract/alignment.h"
#include "track_module/landmark/face_landmark.h"
#include "herror.h"

namespace inspire {

FeatureHub::FeatureHub(MatrixCore core, int feature_block_num) {
    for (int i = 0; i < feature_block_num; ++i) {
        std::shared_ptr<FeatureBlock> block;
        block.reset(FeatureBlock::Create(core, 512, 512));
        m_feature_matrix_list_.push_back(block);
    }

}

int32_t FeatureHub::CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2, float &res) {
    if (v1.size() != v2.size() || v1.empty()) {
        return HERR_CTX_REC_CONTRAST_FEAT_ERR; // 无法计算相似性
    }
    // 计算余弦相似性
    res = simd_dot(v1.data(), v2.data(), v1.size());

    return HSUCCEED;
}


int32_t FeatureHub::CosineSimilarity(const float *v1, const float *v2, int32_t size, float &res) {
    res = simd_dot(v1, v2, size);

    return HSUCCEED;
}


int32_t FeatureHub::RegisterFaceFeature(const std::vector<float>& feature, int featureIndex, const std::string &tag, int32_t customId) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }

    // 计算特征向量应该存储在哪个FeatureBlock和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在FeatureBlock中的行号

    // 调用适当的FeatureBlock的注册函数
    int32_t result = m_feature_matrix_list_[blockIndex]->RegisterFeature(rowIndex, feature, tag, customId);

    return result;
}

int32_t FeatureHub::InsertFaceFeature(const std::vector<float>& feature, const std::string &tag, int32_t customId) {
    int32_t ret;
    for (int i = 0; i < m_feature_matrix_list_.size(); ++i) {
        auto &block = m_feature_matrix_list_[i];
        ret = block->AddFeature(feature, tag, customId);
        if (ret != HERR_CTX_REC_BLOCK_FULL) {
            break;
        }
    }

    return ret;
}

int32_t FeatureHub::SearchFaceFeature(const std::vector<float>& queryFeature, SearchResult &searchResult, float threshold, bool mostSimilar) {
    if (queryFeature.size() != NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_FEAT_SIZE_ERR; // 查询特征大小与预期不符
    }

    bool found = false; // 是否找到匹配的特征
    float maxScore = -1.0f; // 最大分数初始化为一个负数
    int maxIndex = -1; // 最大分数对应的索引
    std::string tag = "None";
    int maxCid = -1;

    for (int blockIndex = 0; blockIndex < m_feature_matrix_list_.size(); ++blockIndex) {
        if (m_feature_matrix_list_[blockIndex]->GetUsedCount() == 0) {
            // 如果该 FeatureBlock 没有已使用的特征，跳到下一个块
            continue;
        }

        int startIndex = blockIndex * NUM_OF_FEATURES_IN_BLOCK;
        SearchResult tempResult;

        // 调用适当的 FeatureBlock 的搜索函数
        int32_t result = m_feature_matrix_list_[blockIndex]->SearchNearest(queryFeature, tempResult);

        if (result != 0) {
            // 处理错误
            return result;
        }

        // 如果找到更高分数的特征
        if (tempResult.score > maxScore) {
            maxScore = tempResult.score;
            maxIndex = startIndex + tempResult.index;
            tag = tempResult.tag;
            maxCid = tempResult.customId;
            if (maxScore >= threshold) {
                found = true;
                if (!mostSimilar) {
                    break; // 当分数大于等于阈值时，停止搜索下一个 FeatureBlock
                }
            }
        }
    }

    if (found) {
        searchResult.score = maxScore;
        searchResult.index = maxIndex;
        searchResult.tag = tag;
        searchResult.customId = maxCid;
        return 0; // 返回成功
    } else {
        searchResult.score = -1.0f;
        searchResult.index = -1;
        searchResult.tag = "None";
        searchResult.customId = -1;
    }

    return HSUCCEED; // 没有找到匹配的特征 但是不算错误
}

int32_t FeatureHub::DeleteFaceFeature(int featureIndex) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }

    // 计算特征向量应该删除在哪个 FeatureBlock 和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的 FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在 FeatureBlock 中的行号

    // 调用适当的 FeatureBlock 的删除函数
    int32_t result = m_feature_matrix_list_[blockIndex]->DeleteFeature(rowIndex);

    return result;
}

int32_t FeatureHub::GetFaceFeature(int featureIndex, Embedded &feature) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }
    // 计算特征向量应该删除在哪个 FeatureBlock 和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的 FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在 FeatureBlock 中的行号

    int32_t result = m_feature_matrix_list_[blockIndex]->GetFeature(rowIndex, feature);

    return result;
}

int32_t FeatureHub::GetFaceEntity(int featureIndex, Embedded &feature, std::string& tag, FEATURE_STATE& status) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }
    // 计算特征向量应该删除在哪个 FeatureBlock 和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的 FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在 FeatureBlock 中的行号

    int32_t result = m_feature_matrix_list_[blockIndex]->GetFeature(rowIndex, feature);
    tag = m_feature_matrix_list_[blockIndex]->GetTagFromRow(rowIndex);
    status = m_feature_matrix_list_[blockIndex]->GetStateFromRow(rowIndex);


    return result;
}

int32_t FeatureHub::GetFaceFeatureCount() {
    int totalFeatureCount = 0;

    // 遍历所有 FeatureBlock，累加已使用的特征向量数量
    for (const auto& block : m_feature_matrix_list_) {
        totalFeatureCount += block->GetUsedCount();
    }

    return totalFeatureCount;
}

int32_t FeatureHub::GetFeatureNum() const {
    return NUM_OF_FEATURES_IN_BLOCK;
}

int32_t FeatureHub::UpdateFaceFeature(const std::vector<float> &feature, int featureIndex, const std::string &tag, int32_t customId) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }

    // 计算特征向量应该存储在哪个FeatureBlock和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在FeatureBlock中的行号

    // 调用适当的FeatureBlock的注册函数
    int32_t result = m_feature_matrix_list_[blockIndex]->UpdateFeature(rowIndex, feature, tag, customId);

    return result;
}

void FeatureHub::PrintFeatureMatrixInfo() {
    m_feature_matrix_list_[0]->PrintMatrix();
}


int32_t FeatureHub::FindFeatureIndexByCustomId(int32_t customId) {
    // 遍历所有的 FeatureBlock
    for (int blockIndex = 0; blockIndex < m_feature_matrix_list_.size(); ++blockIndex) {
        int startIndex = blockIndex * NUM_OF_FEATURES_IN_BLOCK;

        // 从当前 FeatureBlock 查询 customId
        int rowIndex = m_feature_matrix_list_[blockIndex]->FindIndexByCustomId(customId);

        if (rowIndex != -1) {
            return startIndex + rowIndex;  // 返回行号
        }
    }

    return -1;  // 如果所有 FeatureBlock 中都没有找到，则返回-1
}

FeatureHub::FeatureHub() : m_enable_(false) {

}


} // namespace hyper
