//
// Created by tunm on 2023/10/11.
//

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
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
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
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        CHECK(ret != HSUCCEED);
    }

    SECTION("Configure the positive flow for connecting to the database") {
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_DatabaseConfiguration configuration = {0};
        configuration.enableUseDb = 1;
        configuration.dbPath = "./.test";
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FaceContextDataPersistence(ctxHandle, configuration);
        REQUIRE(ret == HSUCCEED);
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

    }

}