//
// Created by tunm on 2023/8/29.
//
#pragma once
#ifndef HYPERFACEREPO_CONFIG_H
#define HYPERFACEREPO_CONFIG_H
#include <iostream>
#include "middleware/configurable.h"

namespace inspire {

const nlohmann::json Pikachu_t1 = R"({
    "extract_input_name": "data",
    "extract_output_name": "fc1_scale"
})"_json;


const nlohmann::json Optimus_t1 = R"({
    "extract_input_name": "input.1",
    "extract_output_name": "267"
})"_json;

/**
 * It doesn't look that good, it's a temporary option for some subtle differences in the different models,
 * let's optimize it later
 * */
class ModelConfigManager {
public:

    static Configurable loadConfig(const std::string& modelName) {
        Configurable config;

        if (modelName == "Pikachu_t1") {
            config.load(Pikachu_t1);
        } else if (modelName == "Optimus_t1") {
            config.load(Optimus_t1);
        }
            // ...

        else {
            throw std::runtime_error("Unknown model name: " + modelName);
        }

        return config;
    }

    static Configurable loadConfig(const int magicNumber) {
        Configurable config;

        if (magicNumber == 1128) {
            config.load(Pikachu_t1);
        } else if (magicNumber == 1129) {
            config.load(Optimus_t1);
        }
            // ...

        else {
            throw std::runtime_error("Unknown magic number: " + magicNumber);
        }

        return config;
    }

};

}   // namespace inspire
#endif //HYPERFACEREPO_CONFIG_H
