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
    return "./test_res/save";
}

std::string getTestSaveData(const std::string& name) {
    return getTestSaveDir() + "/" + name;
}

std::string getTestModelsFile() {
    std::string path = "model_zip/";
    path = path + TEST_MODEL_FILE;
    return getTestData(path);
}