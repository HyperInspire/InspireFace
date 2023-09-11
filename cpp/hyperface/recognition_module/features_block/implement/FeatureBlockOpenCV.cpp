//
// Created by Tunm-Air13 on 2023/9/11.
//

#include "FeatureBlockOpenCV.h"
#include "herror.h"

namespace hyper {


FeatureBlockOpenCV::FeatureBlockOpenCV(int32_t features_max, int32_t feature_length)
    :m_feature_matrix_(features_max, feature_length, CV_32F, cv::Scalar(0.0f)){

}

int32_t FeatureBlockOpenCV::UnsafeAddFeature(const vector<float> &feature) {
    if (feature.empty()) {
        return HERR_CTX_REC_ADD_FEAT_EMPTY; // 如果特征为空，不进行添加
    }
    if (feature.size() != m_feature_length_) {
        return HERR_CTX_REC_ADD_FEAT_SIZE_ERR;
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

    cv::Mat idle_feat(1, m_feature_length_, CV_32FC1, cv::Scalar(-1.0f));
    idle_feat.copyTo(rowToUpdate);
    m_feature_state_[rowToDelete] = FEATURE_STATE::IDLE;

    std::cout << m_feature_matrix_.row(rowToDelete) << std::endl;

    return HSUCCEED;
}


int32_t FeatureBlockOpenCV::UnsafeUpdateFeature(int rowToUpdate, const vector<float> &newFeature) {
    if (rowToUpdate < 0 || rowToUpdate >= m_feature_matrix_.rows || newFeature.empty()) {
        return HERR_CTX_REC_UPDATE_FAILURE; // 无效的行号或新特征为空，不进行更新
    }

    if (newFeature.size() != m_feature_length_) {
        return HERR_CTX_REC_ADD_FEAT_SIZE_ERR;
    }

    // 创建一个新的行向量矩阵并将新特征向量添加为行
    cv::Mat updatedFeatureMat(1, newFeature.size(), CV_32FC1);
    for (int i = 0; i < newFeature.size(); ++i) {
        updatedFeatureMat.at<float>(0, i) = newFeature[i];
    }

    // 将新特征向量覆盖到指定行
    updatedFeatureMat.copyTo(m_feature_matrix_.row(rowToUpdate));

    return HSUCCEED;
}

bool FeatureBlockOpenCV::SearchNearest(const vector<float> &queryFeature, int &bestIndex, float &top1Score) {
    std::lock_guard<std::mutex> lock(m_mtx_);

    if (m_feature_matrix_.rows == 0) {
        return false; // 如果特征矩阵为空，无法进行搜索
    }

    if (queryFeature.size() != m_feature_length_) {
        return HERR_CTX_REC_ADD_FEAT_SIZE_ERR;
    }

    cv::Mat queryMat(queryFeature.size(), 1, CV_32FC1, (void*)queryFeature.data());
//    cout << queryMat.size << endl;
//    cout << featureMatrix.size << endl;

    // 计算余弦相似性矩阵
    cv::Mat cosineSimilarities;
    cv::gemm(m_feature_matrix_, queryMat, 1, cv::Mat(), 0, cosineSimilarities);

    // 寻找最大相似性值的索引
    double top1ScoreDouble; // 使用 double 来接收结果
    cv::minMaxIdx(cosineSimilarities, nullptr, &top1ScoreDouble, nullptr, &bestIndex);

    top1Score = static_cast<float>(top1ScoreDouble); // 将 double 转换为 float

    if (bestIndex != -1) {
        return true;
    }

    return false;
}

void FeatureBlockOpenCV::PrintMatrixSize() {
    std::cout << m_feature_matrix_.size << std::endl;
}

void FeatureBlockOpenCV::PrintMatrix() {
    std::cout << m_feature_matrix_ << std::endl;
}


}   // namespace hyper
