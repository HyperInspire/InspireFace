//
// Created by tunm on 2023/10/11.
//

#include <iostream>
#include "settings/test_settings.h"
#include "inspireface/c_api/inspireface.h"
#include "opencv2/opencv.hpp"
#include "unit/test_helper/simple_csv_writer.h"
#include "unit/test_helper/test_help.h"
#include "unit/test_helper/test_tools.h"


TEST_CASE("test_FaceTrack", "[face_track]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("Face detection from image") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        spdlog::error("error ret :{}", ret);
        REQUIRE(ret == HSUCCEED);

        // Get a face picture
        HImageHandle imgHandle;
        auto image = cv::imread(GET_DATA("data/bulk/kun.jpg"));
        ret = CVImageToImageStream(image, imgHandle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceData = {0};
        ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);

        // Detect face position
        auto rect = multipleFaceData.rects[0];
        HFaceRect expect = {0};
        expect.x = 98;
        expect.y = 146;
        expect.width = 233 - expect.x;
        expect.height = 272 - expect.y;

        auto iou = CalculateOverlap(rect, expect);
        cv::Rect cvRect(rect.x, rect.y, rect.width, rect.height);
        cv::rectangle(image, cvRect, cv::Scalar(255, 0, 124), 2);
        cv::imwrite("ww.jpg", image);
        // The iou is allowed to have an error of 10%
        CHECK(iou == Approx(1.0f).epsilon(0.1));

        ret = HF_ReleaseImageStream(imgHandle);
        REQUIRE(ret == HSUCCEED);

        // Prepare non-face images
        HImageHandle viewHandle;
        auto view = cv::imread(GET_DATA("data/bulk/view.jpg"));
        ret = CVImageToImageStream(view, viewHandle);
        REQUIRE(ret == HSUCCEED);
        ret = HF_FaceContextRunFaceTrack(ctxHandle, viewHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 0);

        ret = HF_ReleaseImageStream(viewHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

    }

    SECTION("Face tracking stability from frames") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_VIDEO;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        auto expectedId = 1;
        int start = 1, end = 288;
        std::vector<std::string> filenames = generateFilenames("frame-%04d.jpg", start, end);
        auto count_loss = 0;
        for (int i = 0; i < filenames.size(); ++i) {
            auto filename = filenames[i];
            HImageHandle imgHandle;
            auto image = cv::imread(GET_DATA("video_frames/" + filename));
            ret = CVImageToImageStream(image, imgHandle);
            REQUIRE(ret == HSUCCEED);

            HF_MultipleFaceData multipleFaceData = {0};
            ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandle, &multipleFaceData);
            REQUIRE(ret == HSUCCEED);
//            CHECK(multipleFaceData.detectedNum == 1);
            if (multipleFaceData.detectedNum != 1) {
                count_loss++;
                continue;
            }
            auto rect = multipleFaceData.rects[0];
            cv::Rect cvRect(rect.x, rect.y, rect.width, rect.height);
            cv::rectangle(image, cvRect, cv::Scalar(255, 0, 124), 2);
            std::string save = GET_SAVE_DATA("video_frames") + "/" + std::to_string(i) + ".jpg";
            cv::imwrite(save, image);
            auto id = multipleFaceData.trackIds[0];
//            TEST_PRINT("{}", id);
            if (id != expectedId) {
                count_loss++;
            }

            ret = HF_ReleaseImageStream(imgHandle);
            REQUIRE(ret == HSUCCEED);
        }
        float loss = (float )count_loss / filenames.size();
        // The face track loss is allowed to have an error of 5%
//        CHECK(loss == Approx(0.0f).epsilon(0.05));

        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("Head pose estimation") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Extract basic face information from photos
        HF_MultipleFaceData multipleFaceData = {0};

        // Left side face
        HImageHandle leftHandle;
        auto left = cv::imread(GET_DATA("data/pose/left_face.jpeg"));
        ret = CVImageToImageStream(left, leftHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FaceContextRunFaceTrack(ctxHandle, leftHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);

        HFloat yaw, pitch, roll;
        bool checked;

        // Left-handed rotation
        yaw = multipleFaceData.angles.yaw[0];
        checked = (yaw > -90 && yaw < -10);
        CHECK(checked);

        HF_ReleaseImageStream(leftHandle);

        // Right-handed rotation
        HImageHandle rightHandle;
        auto right = cv::imread(GET_DATA("data/pose/right_face.png"));
        ret = CVImageToImageStream(right, rightHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FaceContextRunFaceTrack(ctxHandle, rightHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);
        yaw = multipleFaceData.angles.yaw[0];
        checked = (yaw > 10 && yaw < 90);
        CHECK(checked);

        HF_ReleaseImageStream(rightHandle);

        // Rise head
        HImageHandle riseHandle;
        auto rise = cv::imread(GET_DATA("data/pose/rise_face.jpeg"));
        ret = CVImageToImageStream(rise, riseHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FaceContextRunFaceTrack(ctxHandle, riseHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);
        pitch = multipleFaceData.angles.pitch[0];
        CHECK(pitch > 5);
        HF_ReleaseImageStream(riseHandle);

        // Lower head
        HImageHandle lowerHandle;
        auto lower = cv::imread(GET_DATA("data/pose/lower_face.jpeg"));
        ret = CVImageToImageStream(lower, lowerHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FaceContextRunFaceTrack(ctxHandle, lowerHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);
        pitch = multipleFaceData.angles.pitch[0];
        CHECK(pitch < -10);
        HF_ReleaseImageStream(lowerHandle);

        // Roll head
        HImageHandle leftWryneckHandle;
        auto leftWryneck = cv::imread(GET_DATA("data/pose/left_wryneck.png"));
        ret = CVImageToImageStream(leftWryneck, leftWryneckHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FaceContextRunFaceTrack(ctxHandle, leftWryneckHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);
        roll = multipleFaceData.angles.roll[0];
        CHECK(roll < -30);
        HF_ReleaseImageStream(leftWryneckHandle);

        // Roll head
        HImageHandle rightWryneckHandle;
        auto rightWryneck = cv::imread(GET_DATA("data/pose/right_wryneck.png"));
        ret = CVImageToImageStream(rightWryneck, rightWryneckHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FaceContextRunFaceTrack(ctxHandle, rightWryneckHandle, &multipleFaceData);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);
        roll = multipleFaceData.angles.roll[0];
        CHECK(roll > 30);
        HF_ReleaseImageStream(rightWryneckHandle);

        //  finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

    }

    SECTION("Face detection benchmark") {
#ifdef ENABLE_BENCHMARK
        int loop = 1000;
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);

        // Prepare an image
        HImageHandle imgHandle;
        auto image = cv::imread(GET_DATA("data/bulk/kun.jpg"));
        ret = CVImageToImageStream(image, imgHandle);
        REQUIRE(ret == HSUCCEED);
        BenchmarkRecord record(getBenchmarkRecordFile());

        // Case: Execute the benchmark using the IMAGE mode
        ret = HF_FaceContextSetFaceTrackMode(ctxHandle, HF_DETECT_MODE_IMAGE);
        REQUIRE(ret == HSUCCEED);
        HF_MultipleFaceData multipleFaceData = {0};
        auto start = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandle, &multipleFaceData);
        }
        auto cost = ((double) cv::getTickCount() - start) / cv::getTickFrequency() * 1000;
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);
        TEST_PRINT("<Benchmark> Face Detect -> Loop: {}, Total Time: {:.5f}ms, Average Time: {:.5f}ms", loop, cost, cost / loop);
        record.insertBenchmarkData("Face Detect", loop, cost, cost / loop);

        // Case: Execute the benchmark using the VIDEO mode(Track)
        ret = HF_FaceContextSetFaceTrackMode(ctxHandle, HF_DETECT_MODE_VIDEO);
        REQUIRE(ret == HSUCCEED);
        multipleFaceData = {0};
        start = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            ret = HF_FaceContextRunFaceTrack(ctxHandle, imgHandle, &multipleFaceData);
        }
        cost = ((double) cv::getTickCount() - start) / cv::getTickFrequency() * 1000;
        REQUIRE(ret == HSUCCEED);
        REQUIRE(multipleFaceData.detectedNum == 1);
        TEST_PRINT("<Benchmark> Face Track -> Loop: {}, Total Time: {:.5f}ms, Average Time: {:.5f}ms", loop, cost, cost / loop);
        record.insertBenchmarkData("Face Track", loop, cost, cost / loop);

        ret = HF_ReleaseImageStream(imgHandle);
        REQUIRE(ret == HSUCCEED);

        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
#else
        TEST_PRINT("Skip the face detection benchmark test. To run it, you need to turn on the benchmark test.");
#endif
    }

}