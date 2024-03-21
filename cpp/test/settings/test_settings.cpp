//
// Created by Tunm-Air13 on 2023/5/24.
//

#include "test_settings.h"


std::string getTestDataDir() {
    return "./test_res";
}

std::string getTestData(const std::string& name) {
    return getTestDataDir() + "/" + name;
}

std::string getTestSaveDir() {
    return getTestData("save");
}

std::string getTestSaveData(const std::string& name) {
    return getTestSaveDir() + "/" + name;
}

std::string getTestModelsFile() {
    std::string path = "model_zip/";
    path = path + TEST_MODEL_FILE;
    return getTestData(path);
}

std::string getTestLFWFunneledTxt() {
    return getTestData(TEST_LFW_FUNNELED_TXT);
}

std::string getLFWFunneledDir() {
    const char* testLfwFunneledDir = LFW_FUNNELED_DIR;
    std::string lfwFunneledPath;
    if (testLfwFunneledDir != nullptr && std::string(testLfwFunneledDir) != "") {
        lfwFunneledPath = testLfwFunneledDir;
    } else {
        const char* lfwFunneled = std::getenv("LFW_FUNNELED_DIR");
        if (lfwFunneled != nullptr) {
            lfwFunneledPath = lfwFunneled;
        }
    }
    if (lfwFunneledPath.empty()) {
        spdlog::warn("lfw funneled dir is empty!");
    }
    return lfwFunneledPath;
}