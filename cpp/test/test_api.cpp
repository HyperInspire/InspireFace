//
// Created by tunm on 2023/10/11.
//


#define CATCH_CONFIG_RUNNER

#include <iostream>
#include "common/test_settings.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/spdlog.h"

int init_test_logger() {
    std::string name("TEST");
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>(name, stdout_sink);
#if ENABLE_TEST_MSG
    logger->set_level(spdlog::level::trace);
#else
    logger->set_level(spdlog::level::off);
#endif
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [test message] =====> %v");
    spdlog::register_logger(logger);
    return 0;
}

int main(int argc, char* argv[]) {
    init_test_logger();

    return Catch::Session().run(argc, argv);
}