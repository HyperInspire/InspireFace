//
// Created by tunm on 2023/9/8.
//

#include "FaceRecognition.h"
#include "model_index.h"
#include "simd.h"
#include "Alignment.h"
#include "track_module/landmark/FaceLandmark.h"

namespace hyper {

FaceRecognition::FaceRecognition(ModelLoader &loader, bool enable_recognition) {
    if (enable_recognition) {
        auto ret = InitExtractInteraction(loader.ReadModel(ModelIndex::_03_extract));
        if (ret != 0) {
            LOGE("FaceRecognition error.");
        }
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

    return 0;
}

void FaceRecognition::AddFeature(const std::vector<float>& feature) {
    std::lock_guard<std::mutex> lock(mtx);  // 使用互斥锁保护共享数据
    UnsafeAddFeature(feature);
}

void FaceRecognition::DeleteFeature(int rowToDelete) {
    std::lock_guard<std::mutex> lock(mtx);
    UnsafeDeleteFeature(rowToDelete);
}

void FaceRecognition::UnsafeDeleteFeature(int rowToDelete) {
    if (rowToDelete < 0 || rowToDelete >= featureMatrix.rows) {
        return; // 无效的行号，不进行删除
    }

    // 创建一个新的特征矩阵，排除要删除的行
    cv::Mat newFeatureMatrix(featureMatrix.rows - 1, featureMatrix.cols, featureMatrix.type());
    int newRow = 0;

    for (int i = 0; i < featureMatrix.rows; ++i) {
        if (i != rowToDelete) {
            featureMatrix.row(i).copyTo(newFeatureMatrix.row(newRow));
            newRow++;
        }
    }

    featureMatrix = newFeatureMatrix;
}


void FaceRecognition::UnsafeUpdateFeature(int rowToUpdate, const std::vector<float>& newFeature) {
    if (rowToUpdate < 0 || rowToUpdate >= featureMatrix.rows || newFeature.empty()) {
        return; // 无效的行号或新特征为空，不进行更新
    }

    // 创建一个新的行向量矩阵并将新特征向量添加为行
    cv::Mat updatedFeatureMat(1, newFeature.size(), CV_32FC1);
    for (int i = 0; i < newFeature.size(); ++i) {
        updatedFeatureMat.at<float>(0, i) = newFeature[i];
    }

    // 将新特征向量覆盖到指定行
    updatedFeatureMat.copyTo(featureMatrix.row(rowToUpdate));
}

void FaceRecognition::UpdateFeature(int rowToUpdate, const std::vector<float>& newFeature) {
    std::lock_guard<std::mutex> lock(mtx);

    // 调用 UnsafeUpdateFeature 来实现更新操作
    UnsafeUpdateFeature(rowToUpdate, newFeature);
}


bool FaceRecognition::SearchNearest(const std::vector<float>& queryFeature, int& bestIndex, float& top1Score) {
    std::lock_guard<std::mutex> lock(mtx);

    if (featureMatrix.rows == 0) {
        return false; // 如果特征矩阵为空，无法进行搜索
    }

    cv::Mat queryMat(queryFeature.size(), 1, CV_32FC1, (void*)queryFeature.data());
//    cout << queryMat.size << endl;
//    cout << featureMatrix.size << endl;

    // 计算余弦相似性矩阵
    cv::Mat cosineSimilarities;
    cv::gemm(featureMatrix, queryMat, 1, cv::Mat(), 0, cosineSimilarities);

    // 寻找最大相似性值的索引
    double top1ScoreDouble; // 使用 double 来接收结果
    cv::minMaxIdx(cosineSimilarities, nullptr, &top1ScoreDouble, nullptr, &bestIndex);

    top1Score = static_cast<float>(top1ScoreDouble); // 将 double 转换为 float

    if (bestIndex != -1) {
        return true;
    }

    return false;
}



void FaceRecognition::UnsafeAddFeature(const std::vector<float>& feature) {
    if (feature.empty()) {
        return; // 如果特征为空，不进行添加
    }

    // 创建一个行向量矩阵并将新特征向量添加为行
    cv::Mat newFeatureMat(1, feature.size(), CV_32FC1);
    for (int i = 0; i < feature.size(); ++i) {
        newFeatureMat.at<float>(0, i) = feature[i];
    }

    // 如果特征矩阵为空，直接赋值，否则按行追加
    if (featureMatrix.empty()) {
        featureMatrix = newFeatureMat;
    } else {
        cv::vconcat(featureMatrix, newFeatureMat, featureMatrix);
    }
}


float FaceRecognition::CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2) {
    if (v1.size() != v2.size() || v1.empty()) {
        return -1.0f; // 无法计算相似性
    }
    // 计算余弦相似性
    auto score = simd_dot(v1.data(), v2.data(), v1.size());
    return score;
}

int32_t FaceRecognition::FaceExtract(CameraStream &image, const FaceObject &face, Embedded &embedded) {
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

void FaceRecognition::PrintMatrix() const {
    cout << featureMatrix.size << endl;
}

} // namespace hyper
