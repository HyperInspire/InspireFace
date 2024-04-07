//
// Created by Tunm-Air13 on 2024/4/7.
//

#define CATCH_CONFIG_RUNNER

#include <iostream>
#include "settings/test_settings.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/spdlog.h"

int main(int argc, char* argv[]) {
    Catch::Session session;
    // Pack file name
    std::string pack;

    // Add command line options
    auto cli = session.cli() | Catch::clara::Opt(pack, "value")["--pack"]("Resource pack filename");

    session.cli(cli);

    // Parse command line arguments
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) // Indicate an error
        return returnCode;

    // Check whether custom parameters are set
    if (!pack.empty()) {
        SET_PACK_NAME(pack);
        std::cout << "Updated global packName to: " << TEST_MODEL_FILE << std::endl;
    } else {
        std::cout << "Using default global packName: " << TEST_MODEL_FILE << std::endl;
    }

    return session.run();
}