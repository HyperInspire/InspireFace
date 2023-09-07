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
    int32_t ret = ctx.Configuration("resource/model_zip/T1", DetectMode::DETECT_MODE_IMAGE, 1, param);
    if (ret != 0) {
        LOGE("初始化错误");
        return -1;
    }
    auto image = cv::imread("resource/images/cxk.jpg");
    CameraStream stream;
    stream.SetDataFormat(BGR);
    stream.SetRotationMode(ROTATION_0);
    stream.SetDataBuffer(image.data, image.rows, image.cols);
    ctx.InputUpdateStream(stream);


    return 0;
}