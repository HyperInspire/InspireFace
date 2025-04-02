/**
 * Created by Jingyu Yan
 * @date 2025-03-29
 */

#include <iostream>
#include "settings/test_settings.h"
#include "unit/test_helper/simple_csv_writer.h"
#include "unit/test_helper/test_help.h"
#include "unit/test_helper/test_tools.h"
#include <inspireface/include/inspireface/inspireface.hpp>

using namespace inspire;

TEST_CASE("test_SessionFaceTrack", "[session_face_track]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("Face detection from image") {
        CustomPipelineParameter param;
        int32_t ret;

        std::shared_ptr<Session> session = std::shared_ptr<Session>(Session::CreatePtr(DetectModuleMode::DETECT_MODE_ALWAYS_DETECT, 3, param));
        REQUIRE(session != nullptr);

        auto image = inspirecv::Image::Create(GET_DATA("data/bulk/kun.jpg"));
        auto process = inspirecv::FrameProcess::Create(image, inspirecv::DATA_FORMAT::BGR, inspirecv::ROTATION_MODE::ROTATION_0);

        std::vector<FaceTrackWrap> results;
        ret = session->FaceDetectAndTrack(process, results);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(results.size() == 1);
    }
}