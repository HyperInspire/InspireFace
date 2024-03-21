//
// Created by Tunm-Air13 on 2024/3/20.
//

#include <iostream>
#include "settings/test_settings.h"
#include "../test_helper/test_help.h"

TEST_CASE("test_HelpTools", "[help_tools]") {
        DRAW_SPLIT_LINE
        TEST_PRINT_OUTPUT(true);

    SECTION("Load lfw funneled data") {
#ifdef ENABLE_USE_LFW_DATA
        HResult ret;
        std::string modelPath = GET_MODEL_FILE();
        HPath path = modelPath.c_str();
        HF_ContextCustomParameter parameter = {0};
        parameter.enable_recognition = 1;
        HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
        HContextHandle ctxHandle;
        ret = HF_CreateFaceContextFromResourceFile(path, parameter, detMode, 3, &ctxHandle);
        REQUIRE(ret == HSUCCEED);
        HF_DatabaseConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enableUseDb = 1;
        configuration.dbPath = dbPathStr;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FaceContextDataPersistence(ctxHandle, configuration);
        REQUIRE(ret == HSUCCEED);

        auto lfwDir = getLFWFunneledDir();
        auto dataList = LoadLFWFunneledValidData(lfwDir, getTestLFWFunneledTxt());
        size_t numOfNeedImport = 100;
        TEST_PRINT("{}", dataList.size());
        auto importStatus = ImportLFWFunneledValidData(ctxHandle, dataList, numOfNeedImport);
        REQUIRE(importStatus);
        HInt32 count;
        ret = HF_FeatureGroupGetCount(ctxHandle, &count);
        REQUIRE(ret == HSUCCEED);
        CHECK(count == numOfNeedImport);

//        ret = HF_ViewFaceDBTable(ctxHandle);
//        REQUIRE(ret == HSUCCEED);

        // Finish
        ret = HF_ReleaseFaceContext(ctxHandle);
        REQUIRE(ret == HSUCCEED);

        delete []dbPathStr;

#else
        TEST_PRINT("The test case that uses LFW is not enabled, so it will be skipped.");
#endif
    }
}