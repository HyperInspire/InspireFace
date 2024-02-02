//
// Created by Tunm-Air13 on 2024/2/2.
//

#include "common/test_settings.h"
#include "inspireface/face_context.h"
#include "herror.h"
#include "common/face_data/data_tools.h"

using namespace inspire;

TEST_CASE("test_CameraStream", "[camera_stream") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("ConstructAnStreamObject") {
        cv::Mat image = cv::imread(getTestData("images/kun.jpg"));
        REQUIRE(!image.empty());

        CameraStream stream;
        stream.SetDataBuffer(image.data, image.rows, image.cols);
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);

        cv::Mat decode = stream.GetScaledImage(1.0f, true);
        cv::imshow("w", decode);
        cv::waitKey(0);
    }

}