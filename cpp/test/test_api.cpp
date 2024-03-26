//
// Created by tunm on 2023/10/11.
//


#define CATCH_CONFIG_RUNNER

#include <iostream>
#include "settings/test_settings.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/spdlog.h"
#include "unit/test_helper/simple_csv_writer.h"

int init_test_logger() {
    std::string name("TEST");
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>(name, stdout_sink);
#if ENABLE_TEST_MSG
    logger->set_level(spdlog::level::trace);
#else
    logger->set_level(spdlog::level::off);
#endif
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [Test Message] ===> %v");
    spdlog::register_logger(logger);
    return 0;
}

int init_test_benchmark_record() {
#if ENABLE_BENCHMARK
    if (std::remove(getBenchmarkRecordFile().c_str()) != 0) {
        spdlog::trace("Error deleting file");
    }
    BenchmarkRecord record(getBenchmarkRecordFile(), TEST_MODEL_FILE);
#endif
    return 0;
}

int init_test_evaluation_record() {
#if ENABLE_TEST_EVALUATION
    if (std::remove(getEvaluationRecordFile().c_str()) != 0) {
        spdlog::trace("Error deleting file");
    }
    EvaluationRecord record(getEvaluationRecordFile());
#endif
    return 0;
}

int main(int argc, char* argv[]) {
    init_test_logger();
    init_test_benchmark_record();
    init_test_evaluation_record();

    return Catch::Session().run(argc, argv);
}