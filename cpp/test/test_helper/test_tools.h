//
// Created by tunm on 2023/10/12.
//
#pragma
#ifndef HYPERFACEREPO_TEST_TOOLS_H
#define HYPERFACEREPO_TEST_TOOLS_H

#include "opencv2/opencv.hpp"
#include "inspireface/c_api/inspireface.h"

inline HResult ReadImageToImageStream(const char *path, HImageHandle& handle) {
    cv::Mat image = cv::imread(path);
    if (image.empty()) {
        return -1;
    }
    HF_ImageData imageData = {0};
    imageData.data = image.data;
    imageData.height = image.rows;
    imageData.width = image.cols;
    imageData.format = STREAM_BGR;
    imageData.rotation = CAMERA_ROTATION_0;

    auto ret = HF_CreateImageStream(&imageData, &handle);

    return ret;
}

#endif //HYPERFACEREPO_TEST_TOOLS_H
