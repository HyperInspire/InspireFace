//
// Created by tunm on 2023/9/16.
//

#include "common/test_settings.h"
#include "hyperface/face_context.h"
#include "herror.h"

using namespace inspire;

TEST_CASE("test_FaceDetectTrack", "[face_track]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("TrackBenchmark") {
        // 初始化
        FaceContext ctx;
        CustomPipelineParameter param;
        param.enable_face_quality = true;
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_VIDEO, 1, param);
        REQUIRE(ret == HSUCCEED);

        // 准备一张人脸
        auto image = cv::imread(GET_DATA("images/face_sample.png"));
        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);
        stream.SetDataBuffer(image.data, image.rows, image.cols);

        const auto loop = 1000;
        double total = 0.0f;
        spdlog::info("开始执行{}次跟踪: ", loop);

        auto out = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {
            auto timeStart = (double) cv::getTickCount();
            // 检测人脸
            ctx.FaceDetectAndTrack(stream);
            auto &faces = ctx.GetTrackingFaceList();
            double cost = ((double) cv::getTickCount() - timeStart) / cv::getTickFrequency() * 1000;
            REQUIRE(ret == HSUCCEED);
            REQUIRE(faces.size() > 0);
            total += cost;
        }
        auto end = ((double) cv::getTickCount() - out) / cv::getTickFrequency() * 1000;

        spdlog::info("[人脸跟踪]{}次总耗时: {}ms, 平均耗时: {}ms", loop, end, total / loop);
    }

}