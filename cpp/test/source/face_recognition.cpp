//
// Created by Tunm-Air13 on 2023/9/12.
//

#include "common/test_settings.h"
#include "hyperface/FaceContext.h"
#include "herror.h"
#include "common/face_data/DataTools.h"

using namespace hyper;

TEST_CASE("test_FaceRecognition", "[face_rec]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);


    SECTION("FaceContextInit") {
        FaceContext ctx;
        CustomPipelineParameter param;
        param.enable_recognition = true;
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("FaceRecognitionOption") {
        FaceContext ctx;
        CustomPipelineParameter param;
        param.enable_recognition = false;       // 关闭识别功能
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
        REQUIRE(ret == HSUCCEED);

        auto image = cv::imread(GET_DATA("images/cxk.jpg"));
        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);
        stream.SetDataBuffer(image.data, image.rows, image.cols);
        ret = ctx.FaceDetectAndTrack(stream);
        REQUIRE(ret == HSUCCEED);
        ctx.FaceDetectAndTrack(stream);
        const auto &faces = ctx.GetTrackingFaceList();
        REQUIRE(faces.size() > 0);
        Embedded feature;
        ret = ctx.FaceRecognitionModule()->FaceExtract(stream, faces[0], feature);
        CHECK(ret == HERR_CTX_REC_EXTRACT_FAILURE);
    }

    SECTION("FaceRecognition1v1") {
        FaceContext ctx;
        CustomPipelineParameter param;
        param.enable_recognition = true;
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
        REQUIRE(ret == HSUCCEED);

        std::vector<std::string> list = {
                GET_DATA("images/kun.jpg"),
                GET_DATA("images/Kunkun.jpg"),
        };
        EmbeddedList vectors;

        for (int i = 0; i < 2; ++i) {
            auto image = cv::imread(list[i]);
            REQUIRE(!image.empty());
            CameraStream stream;
            stream.SetDataFormat(BGR);
            stream.SetRotationMode(ROTATION_0);
            stream.SetDataBuffer(image.data, image.rows, image.cols);
            ret = ctx.FaceDetectAndTrack(stream);
            REQUIRE(ret == HSUCCEED);
            ctx.FaceDetectAndTrack(stream);
            const auto &faces = ctx.GetTrackingFaceList();
            REQUIRE(faces.size() > 0);
            Embedded feature;
            HyperFaceData data = FaceObjectToHyperFaceData(faces[0]);
            ret = ctx.FaceRecognitionModule()->FaceExtract(stream, data, feature);
            REQUIRE(ret == HSUCCEED);
            vectors.push_back(feature);
        }
        float score;
        ret = ctx.FaceRecognitionModule()->CosineSimilarity(vectors[1], vectors[0], score);
        REQUIRE(ret == HSUCCEED);
//        spdlog::info("score: {}", score);
        CHECK(0.7623623013 == Approx(score).epsilon(1e-3));
    }


}