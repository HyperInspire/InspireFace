//
// Created by Tunm-Air13 on 2023/9/11.
//

#include "feature_block_opencv.h"
#include "herror.h"
#include "log.h"

namespace hyper {


FeatureBlockOpenCV::FeatureBlockOpenCV(int32_t features_max, int32_t feature_length)
    :m_feature_matrix_(features_max, feature_length, CV_32F, cv::Scalar(0.0f)){

}

int32_t FeatureBlockOpenCV::UnsafeAddFeature(const std::vector<float> &feature, const std::string &tag, int32_t customId) {
    if (feature.empty()) {
        return HERR_CTX_REC_ADD_FEAT_EMPTY; // 如果特征为空，不进行添加
    }
    if (feature.size() != m_feature_length_) {
        return HERR_CTX_REC_FEAT_SIZE_ERR;
    }

    if (IsUsedFull()) {
        return HERR_CTX_REC_BLOCK_FULL;
    }

    cv::Mat newFeatureMat(1, feature.size(), CV_32FC1);
    for (int i = 0; i < feature.size(); ++i) {
        newFeatureMat.at<float>(0, i) = feature[i];
    }
    auto idx = FindFirstIdleIndex();    // 找到第一个空闲的向量位置
    if (idx == -1) {
        return HERR_CTX_REC_BLOCK_FULL;
    }
    cv::Mat rowToUpdate = m_feature_matrix_.row(idx);
    newFeatureMat.copyTo(rowToUpdate);

    m_feature_state_[idx] = FEATURE_STATE::USED;    // 设置特征向量已使用
    m_tag_list_[idx] = tag;
    m_custom_id_list_[idx] = customId;

    return HSUCCEED;
}

int32_t FeatureBlockOpenCV::UnsafeDeleteFeature(int rowToDelete) {
    if (m_feature_matrix_.empty() || rowToDelete < 0 || rowToDelete >= m_feature_matrix_.rows) {
        return HERR_CTX_REC_DEL_FAILURE; // 无效的行号或矩阵为空，不进行删除
    }

    cv::Mat rowToUpdate = m_feature_matrix_.row(rowToDelete);
    if (m_feature_state_[rowToDelete] == FEATURE_STATE::IDLE) {
        return HERR_CTX_REC_BLOCK_DEL_FAILURE; // 行处于空闲状态，不进行删除
    }

    m_feature_state_[rowToDelete] = FEATURE_STATE::IDLE;
    m_custom_id_list_[rowToDelete] = -1;

    return HSUCCEED;
}


int32_t FeatureBlockOpenCV::UnsafeRegisterFeature(int rowToUpdate, const std::vector<float> &feature, const std::string &tag, int32_t customId) {
    if (rowToUpdate < 0 || rowToUpdate >= m_feature_matrix_.rows) {
        return HERR_CTX_REC_FEAT_SIZE_ERR; // 无效的行号，不进行更新
    }

    if (feature.size() != m_feature_length_) {
        return HERR_CTX_REC_FEAT_SIZE_ERR; // 新特征大小与预期不符，不进行更新
    }
    cv::Mat rowToUpdateMat = m_feature_matrix_.row(rowToUpdate);
    // 将新特征拷贝到指定行
    for (int i = 0; i < feature.size(); ++i) {
        rowToUpdateMat.at<float>(0, i) = feature[i];
    }
    m_feature_state_[rowToUpdate] = USED;
    m_tag_list_[rowToUpdate] = tag;
    m_custom_id_list_[rowToUpdate] = customId;

    return 0;
}

int32_t FeatureBlockOpenCV::UnsafeUpdateFeature(int rowToUpdate, const std::vector<float> &newFeature, const std::string &tag, int32_t customId) {
    if (rowToUpdate < 0 || rowToUpdate >= m_feature_matrix_.rows) {
        return HERR_CTX_REC_FEAT_SIZE_ERR; // 无效的行号，不进行更新
    }

    if (newFeature.size() != m_feature_length_) {
        return HERR_CTX_REC_FEAT_SIZE_ERR; // 新特征大小与预期不符，不进行更新
    }

    cv::Mat rowToUpdateMat = m_feature_matrix_.row(rowToUpdate);
    if (m_feature_state_[rowToUpdate] == FEATURE_STATE::IDLE) {
        return HERR_CTX_REC_BLOCK_UPDATE_FAILURE; // 行处于空闲状态，不进行更新
    }

    // 将新特征拷贝到指定行
    for (int i = 0; i < newFeature.size(); ++i) {
        rowToUpdateMat.at<float>(0, i) = newFeature[i];
    }
    m_tag_list_[rowToUpdate] = tag;
    m_custom_id_list_[rowToUpdate] = customId;

    return HSUCCEED;
}

int32_t FeatureBlockOpenCV::SearchNearest(const std::vector<float>& queryFeature, SearchResult &searchResult) {
    std::lock_guard<std::mutex> lock(m_mtx_);

    if (queryFeature.size() != m_feature_length_) {
        return HERR_CTX_REC_FEAT_SIZE_ERR;
    }

    if (GetUsedCount() == 0) {
        return HSUCCEED;
    }

    cv::Mat queryMat(queryFeature.size(), 1, CV_32FC1, (void*)queryFeature.data());

    // 计算余弦相似性矩阵
    cv::Mat cosineSimilarities;
    cv::gemm(m_feature_matrix_, queryMat, 1, cv::Mat(), 0, cosineSimilarities);
    // 断言cosineSimilarities是m_features_max_ x 1的向量
    assert(cosineSimilarities.rows == m_features_max_ && cosineSimilarities.cols == 1);

    // 用于存储相似性分数和它们的索引
    std::vector<std::pair<float, int>> similarityScores;

    for (int i = 0; i < m_features_max_; ++i) {
        // 检查状态是否为IDLE
        if (m_feature_state_[i] == FEATURE_STATE::IDLE) {
            continue; // 跳过IDLE状态的特征向量
        }

        // 获取第i行的相似性分数
        float similarityScore = cosineSimilarities.at<float>(i, 0);

        // 将相似性分数和索引作为一对添加到向量中
        similarityScores.push_back(std::make_pair(similarityScore, i));
    }

    // 在similarityScores中找到最大分数的索引
    if (!similarityScores.empty()) {
        auto maxScoreIter = std::max_element(similarityScores.begin(), similarityScores.end());
        float maxScore = maxScoreIter->first;
        int maxScoreIndex = maxScoreIter->second;

        // 设置searchResult中的值
        searchResult.score = maxScore;
        searchResult.index = maxScoreIndex;
        searchResult.tag = m_tag_list_[maxScoreIndex];
        searchResult.customId = m_custom_id_list_[maxScoreIndex];

        return HSUCCEED; // 表示找到了最大分数
    }


    searchResult.score = -1.0f;
    searchResult.index = -1;

    return HSUCCEED;
}

void FeatureBlockOpenCV::PrintMatrixSize() {
    std::cout << m_feature_matrix_.size << std::endl;
}

void FeatureBlockOpenCV::PrintMatrix() {
    LOGD("Num of Features: %d", m_feature_matrix_.cols);
    LOGD("Feature length: %d", m_feature_matrix_.rows);
}

int32_t FeatureBlockOpenCV::GetFeature(int row, std::vector<float> &feature) {
    if (row < 0 || row >= m_feature_matrix_.rows) {
        return HERR_CTX_REC_FEAT_SIZE_ERR; // 无效的行号，不进行更新
    }
    cv::Mat feat = m_feature_matrix_.row(row);
    // 将新特征拷贝到指定行
    for (int i = 0; i < m_feature_length_; ++i) {
        feature.push_back(feat.at<float>(0, i));
    }

    return HSUCCEED;
}


}   // namespace hyper
