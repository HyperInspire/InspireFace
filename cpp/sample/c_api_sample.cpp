//
// Created by tunm on 2023/10/3.
//
#include <iostream>
#include "hyperface/c_api/hyperface.h"
#include "opencv2/opencv.hpp"
#include "hyperface/log.h"

int main() {

    HResult ret;

    {
        // 测试ImageStream
        cv::Mat image = cv::imread("test_res/images/kun.jpg");
        HF_ImageData imageData = {0};
        imageData.data = image.data;
        imageData.height = image.rows;
        imageData.width = image.cols;
        imageData.rotation = CAMERA_ROTATION_0;
        imageData.format = STREAM_BGR;

        HImageHandle imageSteamHandle;
        ret = HF_CreateImageStream(&imageData, &imageSteamHandle);
        if (ret == HSUCCEED) {
            LOGD("image handle: %ld", (long )imageSteamHandle);
        }
        HF_DeBugImageStreamImShow(imageSteamHandle);

        ret = HF_ReleaseImageStream(imageSteamHandle);
        if (ret == HSUCCEED) {
            imageSteamHandle = nullptr;
            LOGD("image released");
        } else {
            LOGE("image release error: %ld", ret);
        }

    }



}