//
// Created by Tunm-Air13 on 2024/4/10.
//

#include <iostream>
#include "inspireface/c_api/inspireface.h"
#include "inspireface/middleware/camera_stream/camera_stream.h"

void non_file_test() {

    HResult ret;
    HPath path = "test_res/model_zip/abc";     // Use error path
    HF_ContextCustomParameter parameter = {0};
    HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
    HContextHandle ctxHandle;
    ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
    if (ret != 0) {
        std::cout << "错误的" << std::endl;
    }

    HF_ReleaseFaceContext(ctxHandle);
}

void camera_test() {
    cv::Mat image = cv::imread("test_res/data/bulk/jntm.jpg");
    inspire::CameraStream stream;
    stream.SetRotationMode(inspire::ROTATION_0);
    stream.SetDataFormat(inspire::NV12);
    stream.SetDataBuffer(image.data, image.rows, image.cols);
    auto decode = stream.GetScaledImage(1.0f, true);

}

int main() {
    camera_test();
}