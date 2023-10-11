//
// Created by tunm on 2023/10/11.
//

//
// Created by tunm on 2023/10/11.
//

#include <iostream>
#include "common/test_settings.h"
#include "hyperface/c_api/hyperface.h"

TEST_CASE("test_FeatureContext", "[face_context]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("Test the new context positive process") {
        HResult ret;
        HString path = "test_res/model_zip/T1";
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);
    }

    SECTION("Test the new context egative processn") {
        HResult ret;
        HString path = "test_res/model_zip/abc";     // Use error path
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        CHECK(ret != HSUCCEED);
    }

}