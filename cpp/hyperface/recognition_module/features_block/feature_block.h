//
// Created by Tunm-Air13 on 2023/9/11.
//

#ifndef HYPERFACEREPO_FEATUREBLOCK_H
#define HYPERFACEREPO_FEATUREBLOCK_H
#include <mutex>
#include <iostream>
#include "data_type.h"

namespace hyper {

typedef enum {
    MC_NONE,           // C/C++ Native
    MC_OPENCV,         // 依赖OpenCV的Mat
    MC_EIGEN,          // 依赖Eigen3的Mat

} MatrixCore;

typedef enum {
    IDLE = 0,       // 闲置的
    USED,           // 使用的
} FEATURE_STATE;

typedef struct SearchResult {
    float score = -1.0f;
    int32_t index = -1;
    std::string tag = "None";
    int32_t customId = -1;
} SearchResult;

class HYPER_API FeatureBlock {
public:
    static FeatureBlock* Create(const MatrixCore crop_type, int32_t features_max = 512, int32_t feature_length = 512);

public:
    virtual ~FeatureBlock() {}

    virtual int32_t AddFeature(const std::vector<float>& feature, const std::string &tag, int32_t customId) {
        std::lock_guard<std::mutex> lock(m_mtx_);  // 使用互斥锁保护共享数据
        return UnsafeAddFeature(feature, tag, customId);
    }

    virtual int32_t DeleteFeature(int rowToDelete) {
        std::lock_guard<std::mutex> lock(m_mtx_);
        return UnsafeDeleteFeature(rowToDelete);
    }

    virtual int32_t UpdateFeature(int rowToUpdate, const std::vector<float>& newFeature, const std::string &tag, int32_t customId) {
        std::lock_guard<std::mutex> lock(m_mtx_);
        return UnsafeUpdateFeature(rowToUpdate, newFeature, tag, customId);
    }

    virtual int32_t RegisterFeature(int rowToUpdate, const std::vector<float>& feature, const std::string &tag, int32_t customId) {
        std::lock_guard<std::mutex> lock(m_mtx_);
        return UnsafeRegisterFeature(rowToUpdate, feature, tag, customId);
    }

    virtual int32_t SearchNearest(const std::vector<float>& queryFeature, SearchResult &searchResult) = 0;

    virtual int32_t GetFeature(int row, std::vector<float>& feature) = 0;

    virtual void PrintMatrixSize() = 0;

    virtual void PrintMatrix() = 0;

public:

    int FindFirstIdleIndex() const {
        for (int i = 0; i < m_feature_state_.size(); ++i) {
            if (m_feature_state_[i] == FEATURE_STATE::IDLE) {
                return i; // 找到第一个 IDLE 的索引
            }
        }
        return -1; // 没有找到 IDLE
    }

    int FindFirstUsedIndex() const {
        for (int i = 0; i < m_feature_state_.size(); ++i) {
            if (m_feature_state_[i] == FEATURE_STATE::USED) {
                return i; // 找到第一个 USED 的索引
            }
        }
        return -1; // 没有找到 USED
    }

    int GetUsedCount() const {
        int usedCount = 0;
        for (const FEATURE_STATE& state : m_feature_state_) {
            if (state == FEATURE_STATE::USED) {
                usedCount++;
            }
        }
        return usedCount;
    }

    bool IsUsedFull() const {
        int usedCount = GetUsedCount();
        return usedCount >= m_features_max_;
    }




protected:
    virtual int32_t UnsafeAddFeature(const std::vector<float>& feature, const std::string &tag, int32_t customId) = 0;
    virtual int32_t UnsafeRegisterFeature(int rowToUpdate, const std::vector<float>& feature, const std::string &tag, int32_t customId) = 0;
    virtual int32_t UnsafeDeleteFeature(int rowToDelete) = 0;
    virtual int32_t UnsafeUpdateFeature(int rowToUpdate, const std::vector<float>& newFeature, const std::string &tag, int32_t customId) = 0;

protected:
    MatrixCore m_matrix_core_;

    int32_t m_features_max_;

    int32_t m_feature_length_;

    std::mutex m_mtx_;  // 添加互斥锁

    std::vector<FEATURE_STATE> m_feature_state_;

    std::vector<String> m_tag_list_;

    std::vector<int32_t> m_custom_id_list_;

};

}   // namespace hyper

#endif //HYPERFACEREPO_FEATUREBLOCK_H
