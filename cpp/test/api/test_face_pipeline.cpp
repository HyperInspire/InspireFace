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

}