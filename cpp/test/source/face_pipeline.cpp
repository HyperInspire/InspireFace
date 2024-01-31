//
// Created by tunm on 2023/9/13.
//

#include "common/test_settings.h"
#include "inspireface/face_context.h"
#include "herror.h"

using namespace inspire;

TEST_CASE("test_FacePipeline", "[face_pipe") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);


    SECTION("FaceContextInit") {
        FaceContext ctx;
        CustomPipelineParameter param;
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("FaceContextMaskPredict") {
        FaceContext ctx;
        CustomPipelineParameter param;
        param.enable_mask_detect = true;
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
        REQUIRE(ret == HSUCCEED);

        {
            // 提前准备一个不佩戴口罩的人脸
            auto image = cv::imread(GET_DATA("images/kun.jpg"));
            CameraStream stream;
            stream.SetDataFormat(BGR);
            stream.SetRotationMode(ROTATION_0);
            stream.SetDataBuffer(image.data, image.rows, image.cols);
            ret = ctx.FaceDetectAndTrack(stream);
            REQUIRE(ret == HSUCCEED);
            // 检测人脸
            ctx.FaceDetectAndTrack(stream);
            auto &faces = ctx.GetTrackingFaceList();
            REQUIRE(faces.size() > 0);
            auto &face = faces[0];
            ctx.FacePipelineModule()->Process(stream, face);
            CHECK(face.faceProcess.maskInfo == MaskInfo::UNMASKED);
        }
        {
            // 提前准备一个佩戴口罩的人脸
            auto image = cv::imread(GET_DATA("images/mask.png"));
            CameraStream stream;
            stream.SetDataFormat(BGR);
            stream.SetRotationMode(ROTATION_0);
            stream.SetDataBuffer(image.data, image.rows, image.cols);
            ret = ctx.FaceDetectAndTrack(stream);
            REQUIRE(ret == HSUCCEED);
            // 检测人脸
            ctx.FaceDetectAndTrack(stream);
            auto &faces = ctx.GetTrackingFaceList();
            REQUIRE(faces.size() > 0);
            auto &face = faces[0];
            ctx.FacePipelineModule()->Process(stream, face);
            CHECK(face.faceProcess.maskInfo == MaskInfo::MASKED);
        }


        SECTION("FaceContextLiveness") {
            FaceContext ctx;
            CustomPipelineParameter param;
            param.enable_liveness = true;
            auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
            REQUIRE(ret == HSUCCEED);

            {
                // 准备真实拍摄的人脸图像
                auto image = cv::imread(GET_DATA("images/face_sample.png"));
                CameraStream stream;
                stream.SetDataFormat(BGR);
                stream.SetRotationMode(ROTATION_0);
                stream.SetDataBuffer(image.data, image.rows, image.cols);
                ret = ctx.FaceDetectAndTrack(stream);
                REQUIRE(ret == HSUCCEED);
                // 检测人脸
                ctx.FaceDetectAndTrack(stream);
                auto &faces = ctx.GetTrackingFaceList();
                REQUIRE(faces.size() > 0);
                auto &face = faces[0];
                ctx.FacePipelineModule()->Process(stream, face);
                CHECK(face.faceProcess.rgbLivenessInfo == RGBLivenessInfo::LIVENESS_REAL);
            }

            {
                // 准备一个非真实拍摄的虚假照片
                auto image = cv::imread(GET_DATA("images/rgb_fake.jpg"));
                CameraStream stream;
                stream.SetDataFormat(BGR);
                stream.SetRotationMode(ROTATION_0);
                stream.SetDataBuffer(image.data, image.rows, image.cols);
                ret = ctx.FaceDetectAndTrack(stream);
                REQUIRE(ret == HSUCCEED);
                // 检测人脸
                ctx.FaceDetectAndTrack(stream);
                auto &faces = ctx.GetTrackingFaceList();
                REQUIRE(faces.size() > 0);
                auto &face = faces[0];
                ctx.FacePipelineModule()->Process(stream, face);
                CHECK(face.faceProcess.rgbLivenessInfo == RGBLivenessInfo::LIVENESS_FAKE);
            }

        }
    }

}