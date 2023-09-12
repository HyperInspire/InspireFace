//
// Created by Tunm-Air13 on 2023/5/24.
//

#include "test_settings.h"


std::string getTestDataDir() {
    return "./resource";
}

std::string getTestData(const std::string& name) {
    return getTestDataDir() + "/" + name;
}

std::string getTestSaveDir() {
    return "./resource/save";
}

std::string getTestSaveData(const std::string& name) {
    return getTestSaveDir() + "/" + name;
}