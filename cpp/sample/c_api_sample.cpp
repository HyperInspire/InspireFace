//
// Created by tunm on 2023/10/3.
//
#include <iostream>
#include "hyperface/c_api/hyperface.h"
#include "opencv2/opencv.hpp"
#include "hyperface/log.h"

int main() {

    HResult ret;

//    {
//        // 测试ImageStream
//        cv::Mat image = cv::imread("test_res/images/kun.jpg");
//        HF_ImageData imageData = {0};
//        imageData.data = image.data;
//        imageData.height = image.rows;
//        imageData.width = image.cols;
//        imageData.rotation = CAMERA_ROTATION_0;
//        imageData.format = STREAM_BGR;
//
//        HImageHandle imageSteamHandle;
//        ret = HF_CreateImageStream(&imageData, &imageSteamHandle);
//        if (ret == HSUCCEED) {
//            LOGD("image handle: %ld", (long )imageSteamHandle);
//        }
//        HF_DeBugImageStreamImShow(imageSteamHandle);
//
//        ret = HF_ReleaseImageStream(imageSteamHandle);
//        if (ret == HSUCCEED) {
//            imageSteamHandle = nullptr;
//            LOGD("image released");
//        } else {
//            LOGE("image release error: %ld", ret);
//        }
//
//    }

    {
        // 初始化context
        HString path = "test_res/model_zip/T1";
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_liveness = 1;
        parameter.enable_mask_detect = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;   // 选择图像模式 即总是检测
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, &parameter, detMode, 3, &ctxHandle);
        if (ret != HSUCCEED) {
            LOGD("An error occurred while creating ctx: %ld", ret);
        }

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

        HInt32 detectedNum = 0;
        HF_FaceContextRunFaceTrack(ctxHandle, imageSteamHandle, &detectedNum);
        LOGD("检测到人脸数量: %d", detectedNum);


        ret = HF_ReleaseFaceContext(ctxHandle);
        if (ret != HSUCCEED) {
            LOGD("Release error");
        }

        ret = HF_ReleaseImageStream(imageSteamHandle);
        if (ret == HSUCCEED) {
            imageSteamHandle = nullptr;
            LOGD("image released");
        } else {
            LOGE("image release error: %ld", ret);
        }

    }



}