#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "rknn_api.h"

static std::vector<uint8_t> ReadFile(const char* path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error(std::string("cannot open ") + path);
    const auto size = stream.tellg();
    stream.seekg(0);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    if (!stream.read(reinterpret_cast<char*>(data.data()), size)) throw std::runtime_error("read failed");
    return data;
}

static void Check(const char* operation, int status) {
    if (status != RKNN_SUCC) throw std::runtime_error(std::string(operation) + " failed: " + std::to_string(status));
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: rknn_landmark_runner MODEL INPUT_BGR_U8 OUTPUT_F32\n";
        return 2;
    }
    rknn_context context = 0;
    try {
        auto model = ReadFile(argv[1]);
        auto input_data = ReadFile(argv[2]);
        if (input_data.size() != 112U * 112U * 3U) throw std::runtime_error("input must be 112x112 BGR uint8");
        Check("rknn_init", rknn_init(&context, model.data(), static_cast<uint32_t>(model.size()), 0, nullptr));
        rknn_sdk_version version{};
        Check("RKNN_QUERY_SDK_VERSION", rknn_query(context, RKNN_QUERY_SDK_VERSION, &version, sizeof(version)));
        std::cout << "API: " << version.api_version << " Driver: " << version.drv_version << "\n";
        rknn_input_output_num count{};
        Check("RKNN_QUERY_IN_OUT_NUM", rknn_query(context, RKNN_QUERY_IN_OUT_NUM, &count, sizeof(count)));
        if (count.n_input != 1 || count.n_output != 1) throw std::runtime_error("expected one input and one output");
        rknn_tensor_attr output_attr{};
        output_attr.index = 0;
        Check("RKNN_QUERY_OUTPUT_ATTR", rknn_query(context, RKNN_QUERY_OUTPUT_ATTR, &output_attr, sizeof(output_attr)));
        if (output_attr.n_elems != 212) throw std::runtime_error("expected 212 output values");
        rknn_input input{};
        input.index = 0;
        input.buf = input_data.data();
        input.size = static_cast<uint32_t>(input_data.size());
        input.type = RKNN_TENSOR_UINT8;
        input.fmt = RKNN_TENSOR_NHWC;
        input.pass_through = 0;
        Check("rknn_inputs_set", rknn_inputs_set(context, 1, &input));
        const auto start = std::chrono::steady_clock::now();
        Check("rknn_run", rknn_run(context, nullptr));
        rknn_output output{};
        output.index = 0;
        output.want_float = 1;
        Check("rknn_outputs_get", rknn_outputs_get(context, 1, &output, nullptr));
        const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::ofstream stream(argv[3], std::ios::binary);
        stream.write(static_cast<const char*>(output.buf), output_attr.n_elems * sizeof(float));
        if (!stream) throw std::runtime_error("output write failed");
        Check("rknn_outputs_release", rknn_outputs_release(context, 1, &output));
        std::cout << "Inference: " << elapsed << " ms, output values: " << output_attr.n_elems << "\n";
        Check("rknn_destroy", rknn_destroy(context));
        context = 0;
        return 0;
    } catch (const std::exception& error) {
        if (context) rknn_destroy(context);
        std::cerr << error.what() << "\n";
        return 1;
    }
}
