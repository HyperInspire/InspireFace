#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "rknn_api.h"

static void Check(const char* operation, int status) {
    if (status != RKNN_SUCC) throw std::runtime_error(std::string(operation) + ": " + std::to_string(status));
}
static std::string Quote(const std::string& value) {
    std::ostringstream s;
    s << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') s << '\\' << c;
        else if (c < 32) s << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c);
        else s << c;
    }
    s << '"';
    return s.str();
}
static std::vector<uint8_t> Read(const std::string& path, uint32_t expected = 0) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("cannot open " + path);
    auto length = f.tellg();
    if (length <= 0 || uint64_t(length) > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("invalid file length: " + path);
    if (expected && uint64_t(length) != expected) throw std::runtime_error("queried input byte size mismatch: " + path);
    std::vector<uint8_t> data(static_cast<size_t>(length));
    f.seekg(0);
    if (!f.read(reinterpret_cast<char*>(data.data()), length)) throw std::runtime_error("read failed: " + path);
    return data;
}
struct Context {
    rknn_context value = 0;
    ~Context() { if (value) rknn_destroy(value); }
};
struct OutputLease {
    rknn_context context;
    std::vector<rknn_output>& outputs;
    bool held = false;
    ~OutputLease() { if (held) rknn_outputs_release(context, outputs.size(), outputs.data()); }
    void Release() {
        const int status = rknn_outputs_release(context, outputs.size(), outputs.data());
        held = false;
        Check("rknn_outputs_release", status);
    }
};
static void Attr(std::ostream& s, const rknn_tensor_attr& a) {
    s << "\"index\":" << a.index << ",\"name\":" << Quote(a.name)
      << ",\"dtype\":" << Quote(get_type_string(a.type))
      << ",\"layout\":" << Quote(get_format_string(a.fmt)) << ",\"dims\":[";
    for (uint32_t i = 0; i < a.n_dims; ++i) s << (i ? "," : "") << a.dims[i];
    s << "],\"n_elems\":" << a.n_elems << ",\"size\":" << a.size
      << ",\"size_with_stride\":" << a.size_with_stride << ",\"w_stride\":" << a.w_stride
      << ",\"qnt_type\":" << Quote(get_qnt_type_string(a.qnt_type))
      << ",\"zp\":" << a.zp << ",\"scale\":" << a.scale << ",\"fl\":" << int(a.fl);
}
static void Validate(const rknn_tensor_attr& a) {
    if (!a.n_dims || a.n_dims > RKNN_MAX_DIMS || !a.n_elems || !a.size || !std::isfinite(a.scale))
        throw std::runtime_error("invalid queried tensor attributes");
    uint64_t elements = 1;
    for (uint32_t i = 0; i < a.n_dims; ++i) {
        if (!a.dims[i] || elements > std::numeric_limits<uint32_t>::max() / a.dims[i])
            throw std::runtime_error("invalid tensor dimensions");
        elements *= a.dims[i];
    }
    if (elements != a.n_elems) throw std::runtime_error("inconsistent tensor element count");
}
int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: rknn_contract_runner MODEL OUTPUT_DIRECTORY --query|INPUT [INPUT ...]\n";
        return 2;
    }
    Context context;
    rknn_sdk_version version{};
    rknn_input_output_num count{};
    std::vector<rknn_tensor_attr> input_attrs, output_attrs;
    std::vector<double> times;
    const bool query = argc == 4 && std::string(argv[3]) == "--query";
    std::string status = "failed", error;
    try {
        auto model = Read(argv[1]);
        Check("rknn_init", rknn_init(&context.value, model.data(), model.size(), 0, nullptr));
        Check("RKNN_QUERY_SDK_VERSION", rknn_query(context.value, RKNN_QUERY_SDK_VERSION, &version, sizeof(version)));
        if (std::string(version.api_version).find("2.3.2") != 0 || std::string(version.drv_version) != "0.9.8")
            throw std::runtime_error("requires API 2.3.2 and driver 0.9.8");
        Check("RKNN_QUERY_IN_OUT_NUM", rknn_query(context.value, RKNN_QUERY_IN_OUT_NUM, &count, sizeof(count)));
        if (!count.n_input || !count.n_output || count.n_input > 4096 || count.n_output > 4096)
            throw std::runtime_error("invalid tensor counts");
        input_attrs.resize(count.n_input);
        output_attrs.resize(count.n_output);
        for (uint32_t i = 0; i < count.n_input; ++i) {
            input_attrs[i].index = i;
            Check("RKNN_QUERY_INPUT_ATTR", rknn_query(context.value, RKNN_QUERY_INPUT_ATTR, &input_attrs[i], sizeof(rknn_tensor_attr)));
            Validate(input_attrs[i]);
        }
        for (uint32_t i = 0; i < count.n_output; ++i) {
            output_attrs[i].index = i;
            Check("RKNN_QUERY_OUTPUT_ATTR", rknn_query(context.value, RKNN_QUERY_OUTPUT_ATTR, &output_attrs[i], sizeof(rknn_tensor_attr)));
            Validate(output_attrs[i]);
        }
        if (!query) {
            if (uint32_t(argc - 3) != count.n_input) throw std::runtime_error("one binary file per queried input required");
            std::vector<std::vector<uint8_t>> data(count.n_input);
            std::vector<rknn_input> inputs(count.n_input);
            for (uint32_t i = 0; i < count.n_input; ++i) {
                data[i] = Read(argv[i + 3], input_attrs[i].size);
                inputs[i].index = i;
                inputs[i].buf = data[i].data();
                inputs[i].size = input_attrs[i].size;
                inputs[i].type = input_attrs[i].type;
                inputs[i].fmt = input_attrs[i].fmt;
                // Files already contain normalized/quantized values in the queried native layout.
                inputs[i].pass_through = 1;
            }
            std::vector<std::vector<float>> buffers(count.n_output);
            std::vector<rknn_output> outputs(count.n_output);
            for (uint32_t i = 0; i < count.n_output; ++i) {
                if (output_attrs[i].n_elems > std::numeric_limits<uint32_t>::max() / sizeof(float))
                    throw std::runtime_error("float output size overflow");
                buffers[i].resize(output_attrs[i].n_elems);
                outputs[i].index = i;
                outputs[i].want_float = 1;
                outputs[i].is_prealloc = 1;
                outputs[i].buf = buffers[i].data();
                outputs[i].size = buffers[i].size() * sizeof(float);
            }
            for (int iteration = 0; iteration < 11; ++iteration) {
                Check("rknn_inputs_set", rknn_inputs_set(context.value, count.n_input, inputs.data()));
                const auto start = std::chrono::steady_clock::now();
                Check("rknn_run", rknn_run(context.value, nullptr));
                OutputLease lease{context.value, outputs};
                Check("rknn_outputs_get", rknn_outputs_get(context.value, count.n_output, outputs.data(), nullptr));
                lease.held = true;
                double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                if (iteration) times.push_back(ms);
                for (uint32_t i = 0; i < count.n_output; ++i) {
                    if (outputs[i].size != buffers[i].size() * sizeof(float)) throw std::runtime_error("float output byte size mismatch");
                    for (float value : buffers[i]) if (!std::isfinite(value)) throw std::runtime_error("non-finite output");
                    if (iteration == 10) {
                        std::ofstream f(std::string(argv[2]) + "/output_" + std::to_string(i) + ".f32", std::ios::binary);
                        f.write(reinterpret_cast<const char*>(buffers[i].data()), outputs[i].size);
                        f.close();
                        if (!f) throw std::runtime_error("output write failed");
                    }
                }
                lease.Release();
            }
        }
        Check("rknn_destroy", rknn_destroy(context.value));
        context.value = 0;
        status = query ? "queried" : "success";
    } catch (const std::exception& e) { error = e.what(); }
    std::ostringstream s;
    s << std::setprecision(10) << "{\"status\":" << Quote(status) << ",\"error\":" << Quote(error)
      << ",\"api_version\":" << Quote(version.api_version) << ",\"driver_version\":" << Quote(version.drv_version)
      << ",\"inputs\":[";
    for (size_t i = 0; i < input_attrs.size(); ++i) { s << (i ? ",{" : "{"); Attr(s, input_attrs[i]); s << '}'; }
    s << "],\"outputs\":[";
    for (size_t i = 0; i < output_attrs.size(); ++i) {
        s << (i ? ",{" : "{"); Attr(s, output_attrs[i]);
        s << ",\"file\":" << Quote("output_" + std::to_string(i) + ".f32") << ",\"finite\":" << (status == "success" ? "true" : "false") << '}';
    }
    s << "],\"warmup_iterations\":1,\"latency_ms\":[";
    for (size_t i = 0; i < times.size(); ++i) s << (i ? "," : "") << times[i];
    s << "]}";
    std::ofstream report(std::string(argv[2]) + (query ? "/contract.json" : "/result.json"));
    report << s.str() << '\n';
    report.close();
    std::cout << s.str() << '\n';
    return status == "failed" || !report ? 1 : 0;
}
