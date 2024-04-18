//
// Created by tunm on 2023/10/11.
//

#include <iostream>
#include "settings/test_settings.h"
#include "inspireface/c_api/inspireface.h"
#include <cstdio>

TEST_CASE("test_FeatureContext", "[face_context]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("Test the new context positive process") {
        HResult ret;
        HF_SessionCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HFSession session;
        ret = HF_CreateInspireFaceSession(parameter, detMode, 3, &session);
        REQUIRE(ret == HSUCCEED);
        ret = HF_ReleaseInspireFaceSession(session);
        REQUIRE(ret == HSUCCEED);
    }


}