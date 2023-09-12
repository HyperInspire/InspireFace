//
// Created by tunm on 2023/9/8.
//

#include "FaceRecognition.h"
#include "model_index.h"
#include "simd.h"
#include "recognition_module/extract/Alignment.h"
#include "track_module/landmark/FaceLandmark.h"
#include "herror.h"

namespace hyper {

FaceRecognition::FaceRecognition(ModelLoader &loader, bool enable_recognition, MatrixCore core, int feature_block_num) {
    if (enable_recognition) {
        auto ret = InitExtractInteraction(loader.ReadModel(ModelIndex::_03_extract));
        if (ret != 0) {
            LOGE("FaceRecognition error.");
        }
    }

    for (int i = 0; i < feature_block_num; ++i) {
        shared_ptr<FeatureBlock> block;
        block.reset(FeatureBlock::Create(core, 512, 512));
        m_feature_matrix_list_.push_back(block);
    }
}

int32_t FaceRecognition::InitExtractInteraction(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_03_extract);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"fc1_scale", });
    param.set<vector<int>>("input_size", {112, 112});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125, 0.0078125, 0.0078125});
    m_extract_ = make_shared<Extract>();
    m_extract_->LoadParam(param, model);

    return HSUCCEED;
}

int32_t FaceRecognition::CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2, float &res) {
    if (v1.size() != v2.size() || v1.empty()) {
        return HERR_CTX_REC_CONTRAST_FEAT_ERR; // 无法计算相似性
    }
    // 计算余弦相似性
    res = simd_dot(v1.data(), v2.data(), v1.size());
    return HSUCCEED;
}

int32_t FaceRecognition::FaceExtract(CameraStream &image, const FaceObject &face, Embedded &embedded) {
    if (m_extract_ == nullptr) {
        return HERR_CTX_REC_EXTRACT_FAILURE;
    }

    auto lmk = face.landmark_;
    vector<cv::Point2f> lmk_5 = {lmk[FaceLandmark::LEFT_EYE_CENTER],
                                 lmk[FaceLandmark::RIGHT_EYE_CENTER],
                                 lmk[FaceLandmark::NOSE_CORNER],
                                 lmk[FaceLandmark::MOUTH_LEFT_CORNER],
                                 lmk[FaceLandmark::MOUTH_RIGHT_CORNER]};
    auto trans = getTransformMatrix112(lmk_5);
    trans.convertTo(trans, CV_64F);
    auto crop = image.GetAffineRGBImage(trans, 112, 112);
    embedded = (*m_extract_)(crop);

    return 0;
}

int32_t FaceRecognition::RegisterFaceFeature(const std::vector<float>& feature, int featureIndex, const std::string &tag) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }

    // 计算特征向量应该存储在哪个FeatureBlock和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在FeatureBlock中的行号

    // 调用适当的FeatureBlock的注册函数
    int32_t result = m_feature_matrix_list_[blockIndex]->RegisterFeature(rowIndex, feature, tag);

    return result;
}

int32_t FaceRecognition::SearchFaceFeature(const std::vector<float>& queryFeature, SearchResult &searchResult, float threshold, bool mostSimilar) {
    if (queryFeature.size() != NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_FEAT_SIZE_ERR; // 查询特征大小与预期不符
    }

    bool found = false; // 是否找到匹配的特征
    float maxScore = -1.0f; // 最大分数初始化为一个负数
    int maxIndex = -1; // 最大分数对应的索引
    string tag = "None";

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
        return 0; // 返回成功
    } else {
        searchResult.score = -1.0f;
        searchResult.index = -1;
        searchResult.tag = "None";
    }

    return HSUCCEED; // 没有找到匹配的特征 但是不算错误
}

int32_t FaceRecognition::DeleteFaceFeature(int featureIndex) {
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

int32_t FaceRecognition::GetFaceFeature(int featureIndex, Embedded &feature) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }

    // 计算特征向量应该删除在哪个 FeatureBlock 和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的 FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在 FeatureBlock 中的行号

    int32_t result = m_feature_matrix_list_[blockIndex]->GetFeature(rowIndex, feature);

    return result;
}

int32_t FaceRecognition::GetFaceFeatureCount() {
    int totalFeatureCount = 0;

    // 遍历所有 FeatureBlock，累加已使用的特征向量数量
    for (const auto& block : m_feature_matrix_list_) {
        totalFeatureCount += block->GetUsedCount();
    }

    return totalFeatureCount;
}

int32_t FaceRecognition::UpdateFaceFeature(const vector<float> &feature, int featureIndex, const string &tag) {
    if (featureIndex < 0 || featureIndex >= m_feature_matrix_list_.size() * NUM_OF_FEATURES_IN_BLOCK) {
        return HERR_CTX_REC_INVALID_INDEX; // 无效的特征索引号
    }

    // 计算特征向量应该存储在哪个FeatureBlock和哪一行
    int blockIndex = featureIndex / NUM_OF_FEATURES_IN_BLOCK; // 计算所在的FeatureBlock
    int rowIndex = featureIndex % NUM_OF_FEATURES_IN_BLOCK;   // 计算在FeatureBlock中的行号

    // 调用适当的FeatureBlock的注册函数
    int32_t result = m_feature_matrix_list_[blockIndex]->UpdateFeature(rowIndex, feature, tag);

    return result;
}

void FaceRecognition::PrintFeatureMatrixInfo() {
    m_feature_matrix_list_[0]->PrintMatrix();
}

} // namespace hyper
