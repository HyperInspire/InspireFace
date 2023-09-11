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

    // 计算余弦相似性
    static int32_t CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2, float &res);

    int32_t FaceExtract(CameraStream &image, const FaceObject& face, Embedded &embedded);


private:
    int32_t InitExtractInteraction(Model *model);

private:

    shared_ptr<Extract> m_extract_;
};

}   // namespace hyper

#endif //HYPERFACEREPO_FACERECOGNITION_H
