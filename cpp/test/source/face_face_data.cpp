//
// Created by tunm on 2023/9/17.
//

#include "common/test_settings.h"
#include "inspireface/common/face_data/data_tools.h"
#include "herror.h"
#include "inspireface/face_context.h"

using namespace inspire;

TEST_CASE("test_FaceData", "[face_data]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("DataConversion") {
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

        ctx.FaceDetectAndTrack(stream);
        auto &faces = ctx.GetTrackingFaceList();
        REQUIRE(ret == HSUCCEED);
        REQUIRE(faces.size() > 0);

        HyperFaceData faceData = FaceObjectToHyperFaceData(faces[0], 0);

        std::cout << faces[0].getTransMatrix() << std::endl;

        PrintHyperFaceData(faceData);

        ByteArray byteArray;
        LOGD("sizeof: %lu", sizeof(byteArray));
        ret = SerializeHyperFaceData(faceData, byteArray);
        CHECK(ret == HSUCCEED);
        LOGD("sizeof: %lu", sizeof(byteArray));


        HyperFaceData decode;
        ret = DeserializeHyperFaceData(byteArray, decode);
        CHECK(ret == HSUCCEED);
        PrintHyperFaceData(decode);

    }
}