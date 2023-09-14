//
// Created by tunm on 2023/9/7.
//

#include <iostream>
#include "FaceContext.h"
#include "opencv2/opencv.hpp"

using namespace hyper;

int main(int argc, char** argv) {
    FaceContext ctx;
    CustomPipelineParameter param;
    param.enable_liveness = true;
    int32_t ret = ctx.Configuration("resource/model_zip/T1", DetectMode::DETECT_MODE_IMAGE, 1, param);
    if (ret != 0) {
        LOGE("初始化错误");
        return -1;
    }
    auto image = cv::imread("/Users/tunm/Downloads/fake.jpg");
    CameraStream stream;
    stream.SetDataFormat(BGR);
    stream.SetRotationMode(ROTATION_0);
    stream.SetDataBuffer(image.data, image.rows, image.cols);
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
    }

    return 0;
}