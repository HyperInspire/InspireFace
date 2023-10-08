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
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;   // 选择图像模式 即总是检测
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, &parameter, detMode, 3, &ctxHandle);
        if (ret != HSUCCEED) {
            LOGD("An error occurred while creating ctx: %ld", ret);
        }


        std::vector<std::string> names = {
                "test_res/images/kun.jpg",
                "test_res/images/Kunkun.jpg",
        };
        HInt32 featureNum;
        HF_FaceContextGetFeatureNum(ctxHandle, &featureNum);
        LOGD("特征长度: %d", featureNum);
        HFloat featuresCache[names.size()][featureNum];     // 存储缓存的向量

        for (int i = 0; i < names.size(); ++i) {
            auto &name = names[i];
            cv::Mat image = cv::imread(name);
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

            HF_MultipleFaceData multipleFaceData = {0};
            HF_FaceContextRunFaceTrack(ctxHandle, imageSteamHandle, &multipleFaceData);
            LOGD("检测到人脸数量: %d", multipleFaceData.detectedNum);

            for (int i = 0; i < multipleFaceData.detectedNum; ++i) {
                cv::Rect rect = cv::Rect(multipleFaceData.rects[i].x, multipleFaceData.rects[i].y, multipleFaceData.rects[i].width, multipleFaceData.rects[i].height);
                cv::rectangle(image, rect, cv::Scalar(0, 255, 200), 2);
                LOGD("%d, track_id: %d, pitch: %f, yaw: %f, roll: %f", i, multipleFaceData.trackIds[i], multipleFaceData.angles.pitch[i], multipleFaceData.angles.yaw[i], multipleFaceData.angles.roll[i]);
                LOGD("token size: %d", multipleFaceData.tokens->size);
            }

            cv::imshow("wq", image);
            cv::waitKey(0);


            HF_FaceFeature feature;
            ret = HF_FaceContextFaceExtract(ctxHandle, imageSteamHandle, multipleFaceData.tokens[0], &feature);
            if (ret != HSUCCEED) {
                LOGE("特征提取有问题: %d", ret);
                return -1;
            }

            memcpy(featuresCache[i], feature.feature, feature.size);

            ret = HF_ReleaseImageStream(imageSteamHandle);
            if (ret == HSUCCEED) {
                imageSteamHandle = nullptr;
                LOGD("image released");
            } else {
                LOGE("image release error: %ld", ret);
            }

        }

        HFloat compResult;
        HF_FaceFeature compFeature1 = {0};
        HF_FaceFeature compFeature2 = {0};
        compFeature1.size = featureNum;
        compFeature1.feature = featuresCache[0];
        compFeature2.size = featureNum;
        compFeature2.feature = featuresCache[1];
        ret = HF_FaceContextComparison(ctxHandle, compFeature1, compFeature2, &compResult);
        if (ret != HSUCCEED) {
            LOGE("对比失败: %d", ret);
            return -1;
        }
        LOGD("相似度: %f", compResult);

        ret = HF_ReleaseFaceContext(ctxHandle);
        if (ret != HSUCCEED) {
            LOGD("Release error");
        }



    }



}