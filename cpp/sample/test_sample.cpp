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

    auto &faces = ctx.GetTrackingFaceList();
    for (int i = 0; i < faces.size(); ++i) {
        auto &face = faces[i];
        for (int j = 0; j < 3; ++j) {
            LOGD("%f", face.getPoseEulerAngle()[j]);
        }
        if (face.isStandard()) {
            LOGD("OK");
        }
        ctx.FacePipelineModule()->Process(stream, face);
//        ctx.FacePipelineModule()->QualityAndPoseDetect(stream, face);
    }

    return 0;
}