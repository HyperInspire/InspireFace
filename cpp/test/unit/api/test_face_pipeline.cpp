//
// Created by tunm on 2023/10/12.
//

#include <iostream>
#include "settings/test_settings.h"
#include "inspireface/c_api/inspireface.h"
#include "../test_helper/test_tools.h"

TEST_CASE("test_FacePipeline", "[face_pipeline]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("rgb liveness detect") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_liveness = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HFSession ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        HFImageStream img1Handle;
        auto img1 = cv::imread(GET_DATA("images/image_T1.jpeg"));
        ret = CVImageToImageStream(img1, img1Handle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceData = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, img1Handle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum > 0);

        ret = HF_MultipleFacePipelineProcess(ctxHandle, img1Handle, &multipleFaceData, parameter);
        REQUIRE(ret == HSUCCEED);
        HF_RGBLivenessConfidence confidence;
        ret = HF_GetRGBLivenessConfidence(ctxHandle, &confidence);
        TEST_PRINT("{}", confidence.confidence[0]);
        REQUIRE(ret == HSUCCEED);
        CHECK(confidence.num > 0);
        CHECK(confidence.confidence[0] > 0.9);

        ret = HF_ReleaseImageStream(img1Handle);
        REQUIRE(ret == HSUCCEED);
        img1Handle = nullptr;

        // fake face
        HFImageStream img2Handle;
        auto img2 = cv::imread(GET_DATA("images/rgb_fake.jpg"));
        ret = CVImageToImageStream(img2, img2Handle);
        REQUIRE(ret == HSUCCEED);
        ret = HF_FaceContextRunFaceTrack(ctxHandle, img2Handle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        ret = HF_MultipleFacePipelineProcess(ctxHandle, img2Handle, &multipleFaceData, parameter);
        REQUIRE(ret == HSUCCEED);
        ret = HF_GetRGBLivenessConfidence(ctxHandle, &confidence);
        REQUIRE(ret == HSUCCEED);
        CHECK(confidence.num > 0);
        CHECK(confidence.confidence[0] < 0.9);

        ret = HF_ReleaseImageStream(img2Handle);
        REQUIRE(ret == HSUCCEED);
        img2Handle = nullptr;


        ret = HF_ReleaseFaceContext(ctxHandle);
        ctxHandle = nullptr;
        REQUIRE(ret == HSUCCEED);

    }


    SECTION("face mask detect") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_mask_detect = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HFSession ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        HFImageStream img1Handle;
        auto img1 = cv::imread(GET_DATA("images/mask2.jpg"));
        ret = CVImageToImageStream(img1, img1Handle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceData = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, img1Handle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum > 0);

        ret = HF_MultipleFacePipelineProcess(ctxHandle, img1Handle, &multipleFaceData, parameter);
        REQUIRE(ret == HSUCCEED);
        HF_FaceMaskConfidence confidence;
        ret = HF_GetFaceMaskConfidence(ctxHandle, &confidence);
        REQUIRE(ret == HSUCCEED);
        CHECK(confidence.num > 0);
        CHECK(confidence.confidence[0] > 0.9);

        ret = HF_ReleaseImageStream(img1Handle);
        REQUIRE(ret == HSUCCEED);
        img1Handle = nullptr;


        // no mask face
        HFImageStream img2Handle;
        auto img2 = cv::imread(GET_DATA("images/face_sample.png"));
        ret = CVImageToImageStream(img2, img2Handle);
        REQUIRE(ret == HSUCCEED);
        ret = HF_FaceContextRunFaceTrack(ctxHandle, img2Handle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        ret = HF_MultipleFacePipelineProcess(ctxHandle, img2Handle, &multipleFaceData, parameter);
        REQUIRE(ret == HSUCCEED);
        ret = HF_GetFaceMaskConfidence(ctxHandle, &confidence);
        REQUIRE(ret == HSUCCEED);
//        spdlog::info("mask {}", confidence.confidence[0]);
        CHECK(confidence.num > 0);
        CHECK(confidence.confidence[0] < 0.1);

//        HFloat quality;
//        ret = HF_FaceQualityDetect(ctxHandle, multipleFaceData.tokens[0], &quality);
//        REQUIRE(ret == HSUCCEED);
//        spdlog::info("quality: {}", quality);

        ret = HF_ReleaseImageStream(img2Handle);
        REQUIRE(ret == HSUCCEED);
        img2Handle = nullptr;


        ret = HF_ReleaseFaceContext(ctxHandle);
        ctxHandle = nullptr;
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("face quality") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HInt32 option = HF_ENABLE_QUALITY;
        HFSession ctxHandle;
        ret = HF_CreateFaceContextFromResourceFileOptional(option, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        HFImageStream superiorHandle;
        auto superior = cv::imread(GET_DATA("images/yifei.jpg"));
        ret = CVImageToImageStream(superior, superiorHandle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceData = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, superiorHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum > 0);

        ret = HF_MultipleFacePipelineProcessOptional(ctxHandle, superiorHandle, &multipleFaceData, option);
        REQUIRE(ret == HSUCCEED);

        HFloat quality;
        ret = HF_FaceQualityDetect(ctxHandle, multipleFaceData.tokens[0], &quality);
        REQUIRE(ret == HSUCCEED);
        CHECK(quality > 0.85);

        // blur image
        HFImageStream blurHandle;
        auto blur = cv::imread(GET_DATA("images/blur.jpg"));
        ret = CVImageToImageStream(blur, blurHandle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        ret = HF_FaceContextRunFaceTrack(ctxHandle, blurHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum > 0);

        ret = HF_MultipleFacePipelineProcessOptional(ctxHandle, blurHandle, &multipleFaceData, option);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FaceQualityDetect(ctxHandle, multipleFaceData.tokens[0], &quality);
        REQUIRE(ret == HSUCCEED);
        CHECK(quality < 0.85);


    }

}