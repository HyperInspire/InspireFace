//
// Created by tunm on 2023/9/8.
//

#ifndef HYPERFACEREPO_FACERECOGNITION_H
#define HYPERFACEREPO_FACERECOGNITION_H
#include "extract/Extract.h"
#include "common/face_info/FaceObject.h"
#include "middleware/camera_stream/camera_stream.h"


namespace hyper {

class HYPER_API FaceRecognition {
public:
    FaceRecognition(ModelLoader &loader, bool enable_recognition);

    void AddFeature(const std::vector<float>& feature);
    void DeleteFeature(int rowToDelete);
    void UpdateFeature(int rowToUpdate, const std::vector<float>& newFeature);
    bool SearchNearest(const std::vector<float>& queryFeature, int &bestIndex, float& top1Score);

    // 计算余弦相似性
    float CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2);

    int32_t FaceExtract(CameraStream &image, const FaceObject& face, Embedded &embedded);

    void PrintMatrix() const;

private:
    int32_t InitExtractInteraction(Model *model);


    // 私有函数来执行非线程安全的操作
    void UnsafeAddFeature(const std::vector<float>& feature);
    void UnsafeDeleteFeature(int rowToDelete);
    void UnsafeUpdateFeature(int rowToUpdate, const std::vector<float>& newFeature);

private:

    shared_ptr<Extract> m_extract_;

    cv::Mat featureMatrix;
    cv::flann::Index flannIndex;
    std::mutex mtx;  // 添加互斥锁

};

}   // namespace hyper

#endif //HYPERFACEREPO_FACERECOGNITION_H
