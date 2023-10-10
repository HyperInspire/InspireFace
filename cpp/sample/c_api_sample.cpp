//
// Created by tunm on 2023/10/3.
//
#include <iostream>
#include "hyperface/c_api/hyperface.h"
#include "opencv2/opencv.hpp"
#include "hyperface/log.h"

int compare() {
    HResult ret;
    // 初始化context
    HString path = "test_res/model_zip/T1";
    HF_ContextCustomParameter parameter = {0};
    parameter.enable_liveness = 1;
    parameter.enable_mask_detect = 1;
    parameter.enable_recognition = 1;
    HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;   // 选择图像模式 即总是检测
    HContextHandle ctxHandle;
    ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
    if (ret != HSUCCEED) {
        LOGD("An error occurred while creating ctx: %ld", ret);
    }

    std::vector<std::string> names = {
            "test_res/images/kun.jpg",
            "test_res/images/yifei.jpg",
    };
    HInt32 featureNum;
    HF_GetFeatureLength(ctxHandle, &featureNum);
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


        ret = HF_FaceFeatureExtractCpy(ctxHandle, imageSteamHandle, multipleFaceData.tokens[0], featuresCache[i]);

        std::cout << std::endl;
        if (ret != HSUCCEED) {
            LOGE("特征提取有问题: %d", ret);
            return -1;
        }

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
    compFeature1.data = featuresCache[0];
    compFeature2.size = featureNum;
    compFeature2.data = featuresCache[1];
    ret = HF_FaceComparison1v1(ctxHandle, compFeature1, compFeature2, &compResult);
    if (ret != HSUCCEED) {
        LOGE("对比失败: %d", ret);
        return -1;
    }
    LOGD("相似度: %f", compResult);

    ret = HF_ReleaseFaceContext(ctxHandle);
    if (ret != HSUCCEED) {
        LOGD("Release error");
    }

    return 0;
}

int search() {
    HResult ret;
    // 初始化context
    HString path = "test_res/model_zip/T1";
    HF_ContextCustomParameter parameter = {0};
    parameter.enable_liveness = 1;
    parameter.enable_mask_detect = 1;
    parameter.enable_recognition = 1;
    HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;   // 选择图像模式 即总是检测
    HContextHandle ctxHandle;
    ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
    if (ret != HSUCCEED) {
        LOGD("An error occurred while creating ctx: %ld", ret);
    }

    std::vector<std::string> files_list = {
            "/Users/tunm/Downloads/face_rec/胡歌/胡歌1.jpg",
            "/Users/tunm/Downloads/face_rec/刘浩存/刘浩存1.jpg",
            "/Users/tunm/Downloads/face_rec/刘亦菲/刘亦菲1.jpg",
            "/Users/tunm/Downloads/face_rec/刘奕君/刘奕君1.jpg",
            "/Users/tunm/Downloads/face_rec/伍佰/伍佰1.jpg",
    };

    for (int i = 0; i < files_list.size(); ++i) {
        auto &name = files_list[i];
        cv::Mat image = cv::imread(name);
        HF_ImageData imageData = {0};
        imageData.data = image.data;
        imageData.height = image.rows;
        imageData.width = image.cols;
        imageData.rotation = CAMERA_ROTATION_0;
        imageData.format = STREAM_BGR;

        HImageHandle imageSteamHandle;
        ret = HF_CreateImageStream(&imageData, &imageSteamHandle);
        if (ret != HSUCCEED) {
            LOGE("image handle error: %ld", (long )imageSteamHandle);
            return -1;
        }

        HF_MultipleFaceData multipleFaceData = {0};
        HF_FaceContextRunFaceTrack(ctxHandle, imageSteamHandle, &multipleFaceData);

        if (multipleFaceData.detectedNum <= 0) {
            LOGE("%s 未检测到人脸", name.c_str());
            return -1;
        }

        HF_FaceFeature feature = {0};
        ret = HF_FaceFeatureExtract(ctxHandle, imageSteamHandle, multipleFaceData.tokens[0], &feature);
        if (ret != HSUCCEED) {
            LOGE("特征提取出错: %ld", ret);
            return -1;
        }

        char *tagName = new char[name.size() + 1];
        std::strcpy(tagName, name.c_str());
        HF_FaceFeatureIdentity identity = {0};
        identity.feature = &feature;
        identity.customId = i;
        identity.tag = tagName;

        ret = HF_FeaturesGroupInsertFeature(ctxHandle, identity);
        if (ret != HSUCCEED) {
            LOGE("插入失败: %ld", ret);
            return -1;
        }

        delete[] tagName;

        ret = HF_ReleaseImageStream(imageSteamHandle);
        if (ret == HSUCCEED) {
            imageSteamHandle = nullptr;
            LOGD("image released");
        } else {
            LOGE("image release error: %ld", ret);
        }
    }

    // 准备一个图像进行搜索
    cv::Mat image = cv::imread("test_res/images/kun.jpg");
    HF_ImageData imageData = {0};
    imageData.data = image.data;
    imageData.height = image.rows;
    imageData.width = image.cols;
    imageData.rotation = CAMERA_ROTATION_0;
    imageData.format = STREAM_BGR;

    HImageHandle imageSteamHandle;
    ret = HF_CreateImageStream(&imageData, &imageSteamHandle);
    if (ret != HSUCCEED) {
        LOGE("image handle error: %ld", (long )imageSteamHandle);
        return -1;
    }
    HF_MultipleFaceData multipleFaceData = {0};
    HF_FaceContextRunFaceTrack(ctxHandle, imageSteamHandle, &multipleFaceData);

    if (multipleFaceData.detectedNum <= 0) {
        LOGE("未检测到人脸");
        return -1;
    }

    HF_FaceFeature feature = {0};
    ret = HF_FaceFeatureExtract(ctxHandle, imageSteamHandle, multipleFaceData.tokens[0], &feature);
    if (ret != HSUCCEED) {
        LOGE("特征提取出错: %ld", ret);
        return -1;
    }

    // 删除测试
//    ret = HF_FaceContextFeatureRemove(ctxHandle, 3);
//    if (ret != HSUCCEED) {
//        LOGE("删除失败: %ld", ret);
//    }

    // 修改测试
    std::string newName = "老六";
    char *newTagName = new char[newName.size() + 1];
    std::strcpy(newTagName, newName.c_str());
    HF_FaceFeatureIdentity updateIdentity = {0};
    updateIdentity.customId = 1;
    updateIdentity.tag = newTagName;
    updateIdentity.feature = &feature;
    ret = HF_FeaturesGroupFeatureUpdate(ctxHandle, updateIdentity);
    if (ret != HSUCCEED) {
        LOGE("更新失败: %ld", ret);
    }
    delete[] newTagName;


    HF_FaceFeatureIdentity searchIdentity = {0};
//    HF_FaceFeature featureSearched = {0};
//    searchIdentity.feature = &featureSearched;
    HFloat confidence;
    ret = HF_FeaturesGroupFeatureSearch(ctxHandle, feature, &confidence, &searchIdentity);
    if (ret != HSUCCEED) {
        LOGE("搜索失败: %ld", ret);
        return -1;
    }

    LOGD("搜索置信度: %f", confidence);
    LOGD("匹配到的tag: %s", searchIdentity.tag);
    LOGD("匹配到的customId: %d", searchIdentity.customId);


    // Face Pipeline
    ret = HF_MultipleFacePipelineProcess(ctxHandle, imageSteamHandle, &multipleFaceData, parameter);
    if (ret != HSUCCEED) {
        LOGE("pipeline执行失败: %ld", ret);
        return -1;
    }

    HF_RGBLivenessConfidence livenessConfidence = {0};
    ret = HF_GetRGBLivenessConfidence(ctxHandle, &livenessConfidence);
    if (ret != HSUCCEED) {
        LOGE("获取活体数据失败");
        return -1;
    }
    LOGD("活体置信度: %f", livenessConfidence.confidence[0]);

    HF_FaceMaskConfidence maskConfidence = {0};
    ret = HF_GetFaceMaskConfidence(ctxHandle, &maskConfidence);
    if (ret != HSUCCEED) {
        LOGE("获取活体数据失败");
        return -1;
    }
    LOGD("口罩佩戴置信度: %f", maskConfidence.confidence[0]);

    LOGD("人脸特征数量: %d", HF_FeatureGroupGetCount(ctxHandle));

    ret = HF_ReleaseImageStream(imageSteamHandle);
    if (ret == HSUCCEED) {
        imageSteamHandle = nullptr;
        LOGD("image released");
    } else {
        LOGE("image release error: %ld", ret);
    }

    return 0;
}


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


//    compare();

    search();

}