//
// Created by tunm on 2023/9/7.
//

#include <iostream>
#include "FaceContext.h"
#include "opencv2/opencv.hpp"
#include "utils/test_helper.h"

using namespace hyper;

int main(int argc, char** argv) {
    FaceContext ctx;
    CustomPipelineParameter param;
    param.enable_liveness = true;
    param.enable_face_quality = true;
    int32_t ret = ctx.Configuration("resource/model_zip/T1", DetectMode::DETECT_MODE_IMAGE, 1, param);
    if (ret != 0) {
        LOGE("初始化错误");
        return -1;
    }
    auto image = cv::imread("resource/images/rgb_fake.jpg");
    cv::Mat rot90;
    TestUtils::rotate(image, rot90, ROTATION_90);

    CameraStream stream;
    stream.SetDataFormat(BGR);
    stream.SetRotationMode(ROTATION_90);
    stream.SetDataBuffer(rot90.data, rot90.rows, rot90.cols);
    ctx.FaceDetectAndTrack(stream);

    std::vector<HyperFaceData> faces;
    for (int i = 0; i < ctx.GetNumberOfFacesCurrentlyDetected(); ++i) {
        ByteArray &byteArray = ctx.detectCache[i];
        HyperFaceData face = {0};
        ret = DeserializeHyperFaceData(byteArray, face);
        if (ret != HSUCCEED) {
            return -1;
        }
        faces.push_back(face);
    }

    ret = ctx.FacesProcess(stream, faces, param);
    if (ret != HSUCCEED) {
        return -1;
    }

    // view
    int32_t index = 0;
    LOGD("liveness: %f", ctx.rbgLivenessResultsCache[index]);

    return 0;
}