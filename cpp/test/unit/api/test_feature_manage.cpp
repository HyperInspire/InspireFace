//
// Created by tunm on 2023/10/11.
//

#include <iostream>
#include "settings/test_settings.h"
#include "inspireface/c_api/inspireface.h"
#include "opencv2/opencv.hpp"
#include "unit/test_helper/simple_csv_writer.h"
#include "unit/test_helper/test_help.h"

TEST_CASE("test_FeatureManage", "[feature_manage]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("Face feature management basic functions") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HFSession ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureBlockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        cv::Mat kunImage = cv::imread(GET_DATA("images/kun.jpg"));
        HF_ImageData imageData = {0};
        imageData.data = kunImage.data;
        imageData.height = kunImage.rows;
        imageData.width = kunImage.cols;
        imageData.format = STREAM_BGR;
        imageData.rotation = CAMERA_ROTATION_0;
        HFImageStream imgHandle;
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
        ret = HF_FeatureHubInsertFeature(identity);
        REQUIRE(ret == HSUCCEED);

        // Check number
        HInt32 num;
        ret = HF_FeatureHubGetFaceCount(&num);
        REQUIRE(ret == HSUCCEED);
        CHECK(num == 1);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

        // Update Face info
        HF_FaceFeatureIdentity updatedIdentity = {0};
        updatedIdentity.feature = identity.feature;
        updatedIdentity.customId = identity.customId;
        updatedIdentity.tag = "iKun";
        ret = HF_FeatureHubFaceUpdate(updatedIdentity);
        REQUIRE(ret == HSUCCEED);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

        // Trying to update an identity that doesn't exist
        HF_FaceFeatureIdentity nonIdentity = {0};
        nonIdentity.customId = 234;
        nonIdentity.tag = "no";
        nonIdentity.feature = &feature;
        ret = HF_FeatureHubFaceUpdate(nonIdentity);
        REQUIRE(ret != HSUCCEED);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

        // Trying to delete an identity that doesn't exist
        ret = HF_FeatureHubFaceRemove(nonIdentity.customId);
        REQUIRE(ret != HSUCCEED);
//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);


        // Delete kunkun
        ret = HF_FeatureHubFaceRemove(identity.customId);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubGetFaceCount(&num);
        REQUIRE(ret == HSUCCEED);
        CHECK(num == 0);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);
        delete []dbPathStr;
    }

    SECTION("Import a large faces data") {
#ifdef ENABLE_USE_LFW_DATA
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureBlockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        auto lfwDir = getLFWFunneledDir();
        auto dataList = LoadLFWFunneledValidData(lfwDir, getTestLFWFunneledTxt());
        size_t numOfNeedImport = 1000;
        auto importStatus = ImportLFWFunneledValidData(ctxHandle, dataList, numOfNeedImport);
        REQUIRE(importStatus);
        HInt32 count;
        ret = HF_FeatureHubGetFaceCount(&count);
        REQUIRE(ret == HSUCCEED);
        CHECK(count == numOfNeedImport);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        delete []dbPathStr;

#else
        TEST_PRINT("The test case that uses LFW is not enabled, so it will be skipped.");
#endif
    }

    SECTION("Faces feature CURD") {
#ifdef ENABLE_USE_LFW_DATA
        // This section needs to be connected to the "Import a large faces data" section before it can be executed
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
//        HF_FaceHubConfiguration configuration = {0};
//        auto dbPath = GET_SAVE_DATA(".test");
//        HString dbPathStr = new char[dbPath.size() + 1];
//        std::strcpy(dbPathStr, dbPath.c_str());
//        configuration.enablePersistence = 1;
//        configuration.dbPath = dbPathStr;
//        configuration.featureBlockNum = 20;
//        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
//        configuration.searchThreshold = 0.48f;
//        ret = HF_FaceHubDataEnable(configuration);
//        REQUIRE(ret == HSUCCEED);

        // Face track
        cv::Mat dstImage = cv::imread(GET_DATA("data/bulk/Nathalie_Baye_0002.jpg"));
        HF_ImageData imageData = {0};
        imageData.data = dstImage.data;
        imageData.height = dstImage.rows;
        imageData.width = dstImage.cols;
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
        ret = HF_FeatureHubFaceSearch(feature, &confidence, &searchedIdentity);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedIdentity.customId == 898);
        CHECK(std::string(searchedIdentity.tag) == "Nathalie_Baye");

        // Delete kunkun and search
        ret = HF_FeatureHubFaceRemove(searchedIdentity.customId);
        REQUIRE(ret == HSUCCEED);
        // Search again
        ret = HF_FeatureHubFaceSearch(feature, &confidence, &searchedIdentity);
//        spdlog::info("{}", confidence);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedIdentity.customId == -1);

        // Insert again
        HF_FaceFeatureIdentity againIdentity = {0};
        againIdentity.customId = 898;
        againIdentity.tag = "Cover";
        againIdentity.feature = &feature;
        ret = HF_FeatureHubInsertFeature(againIdentity);
        REQUIRE(ret == HSUCCEED);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

        // Search again
        HF_FaceFeatureIdentity searchedAgainIdentity = {0};
        ret = HF_FeatureHubFaceSearch(feature, &confidence, &searchedAgainIdentity);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedAgainIdentity.customId == 898);

        // Update any feature
        HInt32 updateId = 909;
        cv::Mat zyImage = cv::imread(GET_DATA("data/bulk/woman.png"));
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
        ret = HF_FeatureHubFaceUpdate(updateIdentity);
        REQUIRE(ret == HSUCCEED);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

//
        // Prepare a zy query image
        cv::Mat zyImageQuery = cv::imread(GET_DATA("data/bulk/woman_search.jpeg"));
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
        ret = HF_FeatureHubFaceSearch(featureZyQuery, &confidenceQuery, &searchedIdentityQuery);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchedIdentityQuery.customId == updateId);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);
//        delete []dbPathStr;

#else
        TEST_PRINT("The test case that uses LFW is not enabled, so it will be skipped.");
#endif
    }

    // Test the search time at 1k, 5k and 10k of the face library (the target face is at the back).
    SECTION("Search face benchmark from 1k") {
#if defined(ENABLE_BENCHMARK) && defined(ENABLE_USE_LFW_DATA)
        size_t loop = 1000;
        size_t numOfNeedImport = 1000;
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureBlockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        auto lfwDir = getLFWFunneledDir();
        auto dataList = LoadLFWFunneledValidData(lfwDir, getTestLFWFunneledTxt());
//        TEST_PRINT("{}", dataList.size());
        auto importStatus = ImportLFWFunneledValidData(ctxHandle, dataList, numOfNeedImport);
        REQUIRE(importStatus);
        HInt32 count;
        ret = HF_FeatureHubGetFaceCount(&count);
        REQUIRE(ret == HSUCCEED);
        CHECK(count == numOfNeedImport);

        // Face track
        cv::Mat dstImage = cv::imread(GET_DATA("data/search/Teresa_Williams_0001_1k.jpg"));
        HF_ImageData imageData = {0};
        imageData.data = dstImage.data;
        imageData.height = dstImage.rows;
        imageData.width = dstImage.cols;
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
        auto start = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            ret = HF_FeatureHubFaceSearch(feature, &confidence, &searchedIdentity);
        }
        auto cost = ((double) cv::getTickCount() - start) / cv::getTickFrequency() * 1000;

        REQUIRE(ret == HSUCCEED);
        REQUIRE(searchedIdentity.customId == 999);
        REQUIRE(std::string(searchedIdentity.tag) == "Teresa_Williams");

        TEST_PRINT("<Benchmark> Search Face from 1k -> Loop: {}, Total Time: {:.5f}ms, Average Time: {:.5f}ms", loop, cost, cost / loop);

        BenchmarkRecord record(getBenchmarkRecordFile());
        record.insertBenchmarkData("Search Face from 1k", loop, cost, cost / loop);
        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);
        delete []dbPathStr;
#else
        TEST_PRINT("Skip face search benchmark test, you need to enable both lfw and benchmark test.");
#endif
    }

    SECTION("Search face benchmark from 5k") {
#if defined(ENABLE_BENCHMARK) && defined(ENABLE_USE_LFW_DATA)
        size_t loop = 1000;
        size_t numOfNeedImport = 5000;
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureBlockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        auto lfwDir = getLFWFunneledDir();
        auto dataList = LoadLFWFunneledValidData(lfwDir, getTestLFWFunneledTxt());
        auto importStatus = ImportLFWFunneledValidData(ctxHandle, dataList, numOfNeedImport);
        REQUIRE(importStatus);
        HInt32 count;
        ret = HF_FeatureHubGetFaceCount(&count);
        REQUIRE(ret == HSUCCEED);
        CHECK(count == numOfNeedImport);

        // Face track
        cv::Mat dstImage = cv::imread(GET_DATA("data/search/Mary_Katherine_Smart_0001_5k.jpg"));
        HF_ImageData imageData = {0};
        imageData.data = dstImage.data;
        imageData.height = dstImage.rows;
        imageData.width = dstImage.cols;
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
        auto start = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            ret = HF_FeatureHubFaceSearch(feature, &confidence, &searchedIdentity);
        }
        auto cost = ((double) cv::getTickCount() - start) / cv::getTickFrequency() * 1000;

        REQUIRE(ret == HSUCCEED);
        REQUIRE(searchedIdentity.customId == 4998);
        REQUIRE(std::string(searchedIdentity.tag) == "Mary_Katherine_Smart");

        TEST_PRINT("<Benchmark> Search Face from 5k -> Loop: {}, Total Time: {:.5f}ms, Average Time: {:.5f}ms", loop, cost, cost / loop);

        BenchmarkRecord record(getBenchmarkRecordFile());
        record.insertBenchmarkData("Search Face from 5k", loop, cost, cost / loop);
        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);
        delete []dbPathStr;
#else
        TEST_PRINT("Skip face search benchmark test, you need to enable both lfw and benchmark test.");
#endif
    }

    SECTION("Search face benchmark from 10k") {
#if defined(ENABLE_BENCHMARK) && defined(ENABLE_USE_LFW_DATA)
        size_t loop = 1000;
        size_t numOfNeedImport = 10000;
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureBlockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        auto lfwDir = getLFWFunneledDir();
        auto dataList = LoadLFWFunneledValidData(lfwDir, getTestLFWFunneledTxt());
//        TEST_PRINT("{}", dataList.size());
        auto importStatus = ImportLFWFunneledValidData(ctxHandle, dataList, numOfNeedImport);
        REQUIRE(importStatus);
        HInt32 count;
        ret = HF_FeatureHubGetFaceCount(&count);
        REQUIRE(ret == HSUCCEED);
        CHECK(count == numOfNeedImport);

        // Update any feature
        HInt32 updateId = numOfNeedImport - 1;
        cv::Mat zyImage = cv::imread(GET_DATA("data/bulk/woman.png"));
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
        ret = HF_FeatureHubFaceUpdate(updateIdentity);
        REQUIRE(ret == HSUCCEED);

        HF_ReleaseImageStream(imgHandleZy);

        // Face track
        cv::Mat dstImage = cv::imread(GET_DATA("data/bulk/woman_search.jpeg"));
        HF_ImageData imageData = {0};
        imageData.data = dstImage.data;
        imageData.height = dstImage.rows;
        imageData.width = dstImage.cols;
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
        auto start = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            ret = HF_FeatureHubFaceSearch(feature, &confidence, &searchedIdentity);
        }
        auto cost = ((double) cv::getTickCount() - start) / cv::getTickFrequency() * 1000;

        REQUIRE(ret == HSUCCEED);
        REQUIRE(searchedIdentity.customId == updateId);
        REQUIRE(std::string(searchedIdentity.tag) == "ZY");

        TEST_PRINT("<Benchmark> Search Face from 10k -> Loop: {}, Total Time: {:.5f}ms, Average Time: {:.5f}ms", loop, cost, cost / loop);

        BenchmarkRecord record(getBenchmarkRecordFile());
        record.insertBenchmarkData("Search Face from 10k", loop, cost, cost / loop);

        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);
        delete []dbPathStr;
#else
        TEST_PRINT("Skip face search benchmark test, you need to enable both lfw and benchmark test.");
#endif
    }

    SECTION("Face comparison benchmark") {
#ifdef ENABLE_BENCHMARK
        int loop = 1000;
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureBlockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        cv::Mat zyImage = cv::imread(GET_DATA("data/bulk/woman.png"));
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

        HInt32 featureNum;
        HF_GetFeatureLength(&featureNum);

        // Extract face feature
        HFloat featureCacheZy[featureNum];
        ret = HF_FaceFeatureExtractCpy(ctxHandle, imgHandleZy, multipleFaceDataZy.tokens[0], featureCacheZy);
        HF_FaceFeature featureZy = {0};
        featureZy.size = featureNum;
        featureZy.data = featureCacheZy;
        REQUIRE(ret == HSUCCEED);

        cv::Mat zyImageQuery = cv::imread(GET_DATA("data/bulk/woman_search.jpeg"));
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
        HFloat featureCacheZyQuery[featureNum];
        ret = HF_FaceFeatureExtractCpy(ctxHandle, imgHandleZyQuery, multipleFaceDataZyQuery.tokens[0], featureCacheZyQuery);
        HF_FaceFeature featureZyQuery = {0};
        featureZyQuery.data = featureCacheZyQuery;
        featureZyQuery.size = featureNum;
        REQUIRE(ret == HSUCCEED);

        auto start = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            HFloat compRes;
            ret = HF_FaceComparison1v1(featureZy, featureZyQuery, &compRes);
        }
        auto cost = ((double) cv::getTickCount() - start) / cv::getTickFrequency() * 1000;
        REQUIRE(ret == HSUCCEED);
        TEST_PRINT("<Benchmark> Face Comparison -> Loop: {}, Total Time: {:.5f}ms, Average Time: {:.5f}ms", loop, cost, cost / loop);

        BenchmarkRecord record(getBenchmarkRecordFile());
        record.insertBenchmarkData("Face Comparison", loop, cost, cost / loop);

        HF_ReleaseImageStream(imgHandleZy);
        HF_ReleaseImageStream(imgHandleZyQuery);

        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);
        delete []dbPathStr;
#else
        TEST_PRINT("The benchmark is not enabled, so all relevant test cases are skipped.");
#endif
    }

    SECTION("Face feature extract benchmark") {
#ifdef ENABLE_BENCHMARK
        int loop = 1000;
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureBlockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        // Face track
        cv::Mat dstImage = cv::imread(GET_DATA("data/search/Teresa_Williams_0001_1k.jpg"));
        HF_ImageData imageData = {0};
        imageData.data = dstImage.data;
        imageData.height = dstImage.rows;
        imageData.width = dstImage.cols;
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
        auto start = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            ret = HF_FaceFeatureExtract(ctxHandle, imgHandle, multipleFaceData.tokens[0], &feature);
        }
        auto cost = ((double) cv::getTickCount() - start) / cv::getTickFrequency() * 1000;
        REQUIRE(ret == HSUCCEED);
        TEST_PRINT("<Benchmark> Face Extract -> Loop: {}, Total Time: {:.5f}ms, Average Time: {:.5f}ms", loop, cost, cost / loop);

        BenchmarkRecord record(getBenchmarkRecordFile());
        record.insertBenchmarkData("Face Extract", loop, cost, cost / loop);

        HF_ReleaseImageStream(imgHandle);
        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);
        delete []dbPathStr;
#else
        TEST_PRINT("Skip the face feature extraction benchmark test. To run it, you need to turn on the benchmark test.");
#endif
    }
}