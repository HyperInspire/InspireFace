//
// Created by tunm on 2023/10/11.
//

#include <iostream>
#include "test_helper/test_help.h"
#include "common/test_settings.h"
#include "inspireface/c_api/inspireface.h"
#include "opencv2/opencv.hpp"

TEST_CASE("test_FeatureManage", "[feature_manage]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("Face feature management basic functions") {
        HResult ret;
        HString path = const_cast<HString>(getTestModelsFile().c_str());
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_DatabaseConfiguration configuration = {0};
        configuration.enableUseDb = 1;
        configuration.dbPath = "./.test";
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FaceContextDataPersistence(ctxHandle, configuration);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        cv::Mat kunImage = cv::imread("test_res/images/kun.jpg");
        HF_ImageData imageData = {0};
        imageData.data = kunImage.data;
        imageData.height = kunImage.rows;
        imageData.width = kunImage.cols;
        imageData.format = STREAM_BGR;
        imageData.rotation = CAMERA_ROTATION_0;
        HImageHandle imgHandle;
        ret = HF_CreateImageStream(&imageData, &imgHandle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceData = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum > 0);

        // Extract face feature
        HF_FaceFeature feature = {0};
        ret = HF_FaceFeatureExtract(ctxHandle, imgHandle, multipleFaceData.tokens[0], &feature);
        REQUIRE(ret == HSUCCEED);

        // Insert data into feature management
        HF_FaceFeatureIdentity identity = {0};
        identity.feature = &feature;
        identity.tag = "chicken";
        identity.customId = 1234;
        ret = HF_FeaturesGroupInsertFeature(ctxHandle, identity);
        REQUIRE(ret == HSUCCEED);

        // Check number
        HInt32 num;
        ret = HF_FeatureGroupGetCount(ctxHandle, &num);
        REQUIRE(ret == HSUCCEED);
        CHECK(num == 1);

        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Update Face info
        HF_FaceFeatureIdentity updatedIdentity = {0};
        updatedIdentity.feature = identity.feature;
        updatedIdentity.customId = identity.customId;
        updatedIdentity.tag = "iKun";
        ret = HF_FeaturesGroupFeatureUpdate(ctxHandle, updatedIdentity);
        REQUIRE(ret == HSUCCEED);

        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Trying to update an identity that doesn't exist
        HF_FaceFeatureIdentity nonIdentity = {0};
        nonIdentity.customId = 234;
        nonIdentity.tag = "no";
        nonIdentity.feature = &feature;
        ret = HF_FeaturesGroupFeatureUpdate(ctxHandle, nonIdentity);
        REQUIRE(ret != HSUCCEED);

        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Trying to delete an identity that doesn't exist
        ret = HF_FeaturesGroupFeatureRemove(ctxHandle, nonIdentity.customId);
        REQUIRE(ret != HSUCCEED);
        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);


        // Delete kunkun
        ret = HF_FeaturesGroupFeatureRemove(ctxHandle, identity.customId);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureGroupGetCount(ctxHandle, &num);
        REQUIRE(ret == HSUCCEED);
        CHECK(num == 0);

        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("Import a large faces data") {
        HResult ret;
        HString path = const_cast<HString>(getTestModelsFile().c_str());
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_DatabaseConfiguration configuration = {0};
        configuration.enableUseDb = 1;
        configuration.dbPath = "./.test";
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FaceContextDataPersistence(ctxHandle, configuration);
        REQUIRE(ret == HSUCCEED);

        // 批量导入人脸特征向量
        std::string mat_path = GET_DATA("test_faceset/test_faces_A1.npy");
        std::string tags_path = GET_DATA("test_faceset/test_faces_A1.txt");
        auto result = LoadMatrixAndTags(mat_path, tags_path);
        // 获取特征矩阵和标签名称
        std::vector<std::vector<float>> featureMatrix = result.first;
        std::vector<std::string> tagNames = result.second;
        size_t totalNum = 3000;
        HInt32 featureLength;
        ret = HF_GetFeatureLength(ctxHandle, &featureLength);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(featureMatrix.size() == totalNum);
        REQUIRE(tagNames.size() == totalNum);
        REQUIRE(featureMatrix[0].size() == featureLength);

        HInt32 startId = 10000;
        for (int i = 0; i < totalNum; ++i) {
            auto &featureRaw = featureMatrix[i];
            auto &tagRaw = tagNames[i];
            HF_FaceFeature feature = {0};
            HF_FaceFeatureIdentity identity = {0};

            char *newTagName = new char[tagRaw.size() + 1];
            std::strcpy(newTagName, tagRaw.c_str());
            feature.size = featureRaw.size();
            feature.data = featureRaw.data();
            identity.customId = startId + i;
            identity.feature = &feature;
            identity.tag = newTagName;

            ret = HF_FeaturesGroupInsertFeature(ctxHandle, identity);
            REQUIRE(ret == HSUCCEED);
            delete[] newTagName;

        }

        HInt32 num;
        ret = HF_FeatureGroupGetCount(ctxHandle, &num);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(num == totalNum);

        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("Faces Feature CURD") {
        HResult ret;
        HString path = const_cast<HString>(getTestModelsFile().c_str());
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_DatabaseConfiguration configuration = {0};
        configuration.enableUseDb = 1;
        configuration.dbPath = "./.test";
        ret = HF_FaceContextDataPersistence(ctxHandle, configuration);
        REQUIRE(ret == HSUCCEED);

        // Face track
        cv::Mat kunImage = cv::imread("test_res/images/kun.jpg");
        HF_ImageData imageData = {0};
        imageData.data = kunImage.data;
        imageData.height = kunImage.rows;
        imageData.width = kunImage.cols;
        imageData.format = STREAM_BGR;
        imageData.rotation = CAMERA_ROTATION_0;
        HImageHandle imgHandle;
        ret = HF_CreateImageStream(&imageData, &imgHandle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceData = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum > 0);

        // Extract face feature
        HF_FaceFeature feature = {0};
        ret = HF_FaceFeatureExtract(ctxHandle, imgHandle, multipleFaceData.tokens[0], &feature);
        REQUIRE(ret == HSUCCEED);

        // Search for a face
        HFloat confidence;
        HF_FaceFeatureIdentity searchedIdentity = {0};
        ret = HF_FeaturesGroupFeatureSearch(ctxHandle, feature, &confidence, &searchedIdentity);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedIdentity.customId == 10795);
        CHECK(std::string(searchedIdentity.tag) == "Kunkun");

        // Delete kunkun and search
        ret = HF_FeaturesGroupFeatureRemove(ctxHandle, searchedIdentity.customId);
        REQUIRE(ret == HSUCCEED);
        // Search again
        ret = HF_FeaturesGroupFeatureSearch(ctxHandle, feature, &confidence, &searchedIdentity);
//        spdlog::info("{}", confidence);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedIdentity.customId == -1);

        // Insert again
        HF_FaceFeatureIdentity againIdentity = {0};
        againIdentity.customId = 10795;
        againIdentity.tag = "GG";
        againIdentity.feature = &feature;
        ret = HF_FeaturesGroupInsertFeature(ctxHandle, againIdentity);
        REQUIRE(ret == HSUCCEED);

        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Search again
        HF_FaceFeatureIdentity searchedAgainIdentity = {0};
        ret = HF_FeaturesGroupFeatureSearch(ctxHandle, feature, &confidence, &searchedAgainIdentity);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedAgainIdentity.customId == 10795);

        // Update any feature
        HInt32 updateId = 11297;
        cv::Mat zyImage = cv::imread("test_res/images/face_sample.png");
        HF_ImageData imageDataZy = {0};
        imageDataZy.data = zyImage.data;
        imageDataZy.height = zyImage.rows;
        imageDataZy.width = zyImage.cols;
        imageDataZy.format = STREAM_BGR;
        imageDataZy.rotation = CAMERA_ROTATION_0;
        HImageHandle imgHandleZy;
        ret = HF_CreateImageStream(&imageDataZy, &imgHandleZy);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceDataZy = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandleZy, &multipleFaceDataZy);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceDataZy.detectedNum > 0);

        // Extract face feature
        HF_FaceFeature featureZy = {0};
        ret = HF_FaceFeatureExtract(ctxHandle, imgHandleZy, multipleFaceDataZy.tokens[0], &featureZy);
        REQUIRE(ret == HSUCCEED);

        // Update id: 11297
        HF_FaceFeatureIdentity updateIdentity = {0};
        updateIdentity.customId = updateId;
        updateIdentity.tag = "ZY";
        updateIdentity.feature = &featureZy;
        ret = HF_FeaturesGroupFeatureUpdate(ctxHandle, updateIdentity);
        REQUIRE(ret == HSUCCEED);

        ret = HF_ViewFaceDBTable(ctxHandle);
        REQUIRE(ret == HSUCCEED);

//
        // Prepare a zy query image
        cv::Mat zyImageQuery = cv::imread("test_res/images/face_comp.jpeg");
        HF_ImageData imageDataZyQuery = {0};
        imageDataZyQuery.data = zyImageQuery.data;
        imageDataZyQuery.height = zyImageQuery.rows;
        imageDataZyQuery.width = zyImageQuery.cols;
        imageDataZyQuery.format = STREAM_BGR;
        imageDataZyQuery.rotation = CAMERA_ROTATION_0;
        HImageHandle imgHandleZyQuery;
        ret = HF_CreateImageStream(&imageDataZyQuery, &imgHandleZyQuery);
        REQUIRE(ret == HSUCCEED);
//
        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceDataZyQuery = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandleZyQuery, &multipleFaceDataZyQuery);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceDataZyQuery.detectedNum > 0);
//
        // Extract face feature
        HF_FaceFeature featureZyQuery = {0};
        ret = HF_FaceFeatureExtract(ctxHandle, imgHandleZyQuery, multipleFaceDataZyQuery.tokens[0], &featureZyQuery);
        REQUIRE(ret == HSUCCEED);

        // Search
        HFloat confidenceQuery;
        HF_FaceFeatureIdentity searchedIdentityQuery = {0};
        ret = HF_FeaturesGroupFeatureSearch(ctxHandle, featureZyQuery, &confidenceQuery, &searchedIdentityQuery);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedIdentityQuery.customId == 11297);

    }

}