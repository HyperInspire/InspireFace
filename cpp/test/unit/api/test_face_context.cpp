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
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("Test the new context egative processn") {
        HResult ret;
        HPath path = "test_res/model_zip/abc";     // Use error path
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(parameter, detMode, 3, &ctxHandle);
        CHECK(ret != HSUCCEED);
    }


}