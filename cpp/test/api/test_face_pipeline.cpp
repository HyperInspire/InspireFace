//
// Created by tunm on 2023/10/12.
//

#include <iostream>
#include "common/test_settings.h"
#include "hyperface/c_api/hyperface.h"
#include "test_helper/test_tools.h"

TEST_CASE("test_FacePipeline", "[face_pipeline]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("rgb liveness detect") {
        HResult ret;
        HString path = "test_res/model_zip/T1";
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_liveness = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        HImageHandle img1Handle;
        ret = ReadImageToImageStream("test_res/images/kun.jpg", img1Handle);
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
        REQUIRE(ret == HSUCCEED);
        CHECK(confidence.num > 0);
        CHECK(confidence.confidence[0] > 0.9);

        ret = HF_ReleaseImageStream(img1Handle);
        REQUIRE(ret == HSUCCEED);
        img1Handle = nullptr;


        // fake face
        HImageHandle img2Handle;
        ret = ReadImageToImageStream("test_res/images/rgb_fake.jpg", img2Handle);
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
        HString path = "test_res/model_zip/T1";
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_mask_detect = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        HImageHandle img1Handle;
        ret = ReadImageToImageStream("test_res/images/mask.png", img1Handle);
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
        HImageHandle img2Handle;
        ret = ReadImageToImageStream("test_res/images/face_sample.png", img2Handle);
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
        HString path = "test_res/model_zip/T1";
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HInt32 option = HF_ENABLE_QUALITY;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFileOptional(path, option, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        HImageHandle superiorHandle;
        ret = ReadImageToImageStream("test_res/images/yifei.jpg", superiorHandle);
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
        HImageHandle blurHandle;
        ret = ReadImageToImageStream("test_res/images/blur.jpg", blurHandle);
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