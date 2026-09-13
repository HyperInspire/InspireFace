// Compare the public RKNN reference path with InspireFace's RKNN2 Nano wrapper.
// One process handles one model.  The launcher owns aggregation so a failed model
// cannot prevent evidence being collected for the remaining selected models.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "rknn_api.h"
#include "inference_wrapper_rknn_adapter_nano.h"

namespace {
constexpr int kWarmupIterations = 10;
constexpr int kMeasuredIterations = 10;
constexpr double kMaxAbsLimit = 1e-5;
constexpr double kCosineLimit = 0.999999;

void Check(const char* operation, int status) {
    if (status != RKNN_SUCC) throw std::runtime_error(std::string(operation) + ": " + std::to_string(status));
}

std::string Quote(const std::string& value) {
    std::ostringstream stream;
    stream << '"';
    for (const unsigned char character : value) {
        if (character == '"' || character == '\\') stream << '\\' << character;
        else if (character < 32) stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(character) << std::dec;
        else stream << character;
    }
    stream << '"';
    return stream.str();
}

std::vector<uint8_t> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("cannot open " + path);
    const std::streamoff length = file.tellg();
    if (length <= 0 || static_cast<uint64_t>(length) > std::numeric_limits<size_t>::max())
        throw std::runtime_error("invalid input length");
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), length)) throw std::runtime_error("cannot read " + path);
    return bytes;
}

bool RuntimeAtLeast(const char* api_version, int required_major, int required_minor, int required_patch) {
    int major = -1, minor = -1, patch = -1;
    // RKNN appends a parenthesized build identifier, so parse only its first token.
    if (!api_version || std::sscanf(api_version, "%d.%d.%d", &major, &minor, &patch) != 3) return false;
    return major > required_major || (major == required_major &&
        (minor > required_minor || (minor == required_minor && patch >= required_patch)));
}

uint64_t PeakRssKb() {
    std::ifstream status("/proc/self/status");
    std::string key;
    uint64_t value = 0;
    while (status >> key >> value) {
        if (key == "VmHWM:" || key == "VmRSS:") return value;
        status.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return 0;
}

uint64_t Product(const rknn_tensor_attr& attr) {
    if (!attr.n_dims || attr.n_dims > RKNN_MAX_DIMS) throw std::runtime_error("invalid tensor rank");
    uint64_t result = 1;
    for (uint32_t index = 0; index < attr.n_dims; ++index) {
        if (!attr.dims[index] || result > std::numeric_limits<uint32_t>::max() / attr.dims[index])
            throw std::runtime_error("invalid tensor dimensions");
        result *= attr.dims[index];
    }
    if (result != attr.n_elems) throw std::runtime_error("tensor element count does not match dimensions");
    return result;
}

void AppendAttr(std::ostream& stream, const rknn_tensor_attr& attr) {
    stream << "{\"index\":" << attr.index << ",\"name\":" << Quote(attr.name)
           << ",\"type\":" << Quote(get_type_string(attr.type))
           << ",\"layout\":" << Quote(get_format_string(attr.fmt))
           << ",\"logical_dims\":[";
    for (uint32_t index = 0; index < attr.n_dims; ++index) stream << (index ? "," : "") << attr.dims[index];
    stream << "],\"n_elems\":" << attr.n_elems << ",\"size\":" << attr.size
           << ",\"size_with_stride\":" << attr.size_with_stride << ",\"w_stride\":" << attr.w_stride
           << ",\"qnt_type\":" << Quote(get_qnt_type_string(attr.qnt_type))
           << ",\"scale\":" << attr.scale << ",\"zp\":" << attr.zp << "}";
}

void AppendAttrs(std::ostream& stream, const std::vector<rknn_tensor_attr>& attrs) {
    stream << '[';
    for (size_t index = 0; index < attrs.size(); ++index) {
        if (index) stream << ',';
        AppendAttr(stream, attrs[index]);
    }
    stream << ']';
}

std::vector<rknn_tensor_attr> QueryAttrs(rknn_context context, rknn_query_cmd command, uint32_t count) {
    std::vector<rknn_tensor_attr> attrs(count);
    for (uint32_t index = 0; index < count; ++index) {
        attrs[index].index = index;
        Check("rknn_query tensor attr", rknn_query(context, command, &attrs[index], sizeof(attrs[index])));
        Product(attrs[index]);
        if (!std::isfinite(attrs[index].scale)) throw std::runtime_error("non-finite tensor scale");
    }
    return attrs;
}

struct RKNNContext {
    rknn_context value = 0;
    ~RKNNContext() { if (value) rknn_destroy(value); }
};

struct ReferenceOutputLease {
    rknn_context context = 0;
    std::vector<rknn_output>* outputs = nullptr;
    bool acquired = false;
    ~ReferenceOutputLease() { if (acquired) rknn_outputs_release(context, outputs->size(), outputs->data()); }
};

std::vector<std::vector<float>> RunReference(rknn_context context, std::vector<uint8_t>& input,
                                              const std::vector<rknn_tensor_attr>& outputs) {
    rknn_input reference_input{};
    reference_input.index = 0;
    reference_input.buf = input.data();
    reference_input.size = static_cast<uint32_t>(input.size());
    reference_input.type = RKNN_TENSOR_UINT8;
    reference_input.fmt = RKNN_TENSOR_NHWC;
    reference_input.pass_through = 0;
    Check("rknn_inputs_set", rknn_inputs_set(context, 1, &reference_input));
    Check("rknn_run", rknn_run(context, nullptr));
    std::vector<rknn_output> raw_outputs(outputs.size());
    for (size_t index = 0; index < raw_outputs.size(); ++index) {
        raw_outputs[index].index = static_cast<uint32_t>(index);
        raw_outputs[index].want_float = 1;
    }
    ReferenceOutputLease lease{context, &raw_outputs, false};
    Check("rknn_outputs_get", rknn_outputs_get(context, raw_outputs.size(), raw_outputs.data(), nullptr));
    lease.acquired = true;
    std::vector<std::vector<float>> values(outputs.size());
    for (size_t index = 0; index < outputs.size(); ++index) {
        if (!raw_outputs[index].buf || raw_outputs[index].size != outputs[index].n_elems * sizeof(float))
            throw std::runtime_error("reference output byte count mismatch");
        const float* source = static_cast<const float*>(raw_outputs[index].buf);
        values[index].assign(source, source + outputs[index].n_elems);
    }
    Check("rknn_outputs_release", rknn_outputs_release(context, raw_outputs.size(), raw_outputs.data()));
    lease.acquired = false;
    return values;
}

std::vector<std::vector<float>> RunProduction(InferenceWrapperRKNNAdapter& adapter,
                                               const InputTensorInfo& input,
                                               std::vector<OutputTensorInfo>* declared_outputs) {
    if (adapter.PreProcess({input}) != InferenceWrapper::WrapperOk) throw std::runtime_error("production PreProcess failed");
    if (adapter.Process(*declared_outputs) != InferenceWrapper::WrapperOk) throw std::runtime_error("production Process failed");
    std::vector<std::vector<float>> values(declared_outputs->size());
    for (size_t index = 0; index < declared_outputs->size(); ++index) {
        const OutputTensorInfo& output = declared_outputs->at(index);
        const int32_t element_count = output.GetElementNum();
        const float* source = static_cast<const float*>(output.data);
        if (!source || element_count <= 0) throw std::runtime_error("production output has no logical float data");
        values[index].assign(source, source + element_count);
    }
    return values;
}

struct Metrics { double max_abs = 0; double mean_abs = 0; double cosine = 1; bool finite = true; };
Metrics Compare(const std::vector<float>& reference, const std::vector<float>& production) {
    if (reference.size() != production.size()) throw std::runtime_error("output logical element count mismatch");
    Metrics result;
    double sum = 0, dot = 0, reference_norm = 0, production_norm = 0;
    for (size_t index = 0; index < reference.size(); ++index) {
        const double left = reference[index], right = production[index];
        if (!std::isfinite(left) || !std::isfinite(right)) { result.finite = false; continue; }
        const double error = std::abs(left - right);
        result.max_abs = std::max(result.max_abs, error); sum += error; dot += left * right;
        reference_norm += left * left; production_norm += right * right;
    }
    if (!reference.empty()) result.mean_abs = sum / reference.size();
    if (reference_norm != 0 || production_norm != 0) {
        if (reference_norm == 0 || production_norm == 0) result.cosine = 0;
        else result.cosine = dot / std::sqrt(reference_norm * production_norm);
    }
    return result;
}

double Percentile(std::vector<double> samples, double percentile) {
    if (samples.empty()) return 0;
    std::sort(samples.begin(), samples.end());
    const size_t index = static_cast<size_t>(std::ceil(percentile * samples.size())) - 1;
    return samples[std::min(index, samples.size() - 1)];
}

struct Arguments { std::string id, model, input, result, model_sha256, input_sha256; };
Arguments Parse(int argc, char** argv) {
    Arguments result;
    for (int index = 1; index + 1 < argc; index += 2) {
        const std::string key(argv[index]), value(argv[index + 1]);
        if (key == "--id") result.id = value; else if (key == "--model") result.model = value;
        else if (key == "--input") result.input = value; else if (key == "--model-sha256") result.model_sha256 = value;
        else if (key == "--input-sha256") result.input_sha256 = value; else if (key == "--result") result.result = value;
        else throw std::runtime_error("unknown argument: " + key);
    }
    if (result.id.empty() || result.model.empty() || result.input.empty() || result.result.empty() || result.model_sha256.size() != 64 || result.input_sha256.size() != 64)
        throw std::runtime_error("usage: --id ID --model FILE --input FILE --result FILE --model-sha256 HEX --input-sha256 HEX");
    return result;
}

bool WriteReport(const std::string& path, const std::string& content) {
    const std::string partial = path + ".partial";
    {
        std::ofstream output(partial, std::ios::binary | std::ios::trunc);
        output << content << '\n';
        if (!output) return false;
    }
    return std::rename(partial.c_str(), path.c_str()) == 0;
}
}  // namespace

int main(int argc, char** argv) {
    Arguments arguments{};
    std::string failure_stage = "argument", error, status = "failed";
    rknn_sdk_version version{};
    std::vector<rknn_tensor_attr> normal_inputs, normal_outputs, native_outputs;
    std::vector<Metrics> metrics;
    std::vector<double> reference_latency_ms, production_latency_ms;
    bool all_finite = false, accepted = false;
    uint32_t rnet_width = 0, rnet_w_stride = 0, scrfd_output_count = 0, attitude_output_count = 0;
    uint64_t peak_rss_kb = 0;
    try {
        arguments = Parse(argc, argv);
        std::vector<uint8_t> model = ReadFile(arguments.model);
        std::vector<uint8_t> input_bytes = ReadFile(arguments.input);
        if (model.size() > static_cast<size_t>(std::numeric_limits<int>::max())) throw std::runtime_error("model exceeds adapter size limit");
        RKNNContext reference;
        failure_stage = "reference_init";
        Check("rknn_init", rknn_init(&reference.value, model.data(), model.size(), 0, nullptr));
        Check("RKNN_QUERY_SDK_VERSION", rknn_query(reference.value, RKNN_QUERY_SDK_VERSION, &version, sizeof(version)));
        if (!RuntimeAtLeast(version.api_version, 2, 3, 2) || version.drv_version[0] == '\0')
            throw std::runtime_error("requires RKNN runtime >= 2.3.2 and a nonempty driver version");
        rknn_input_output_num counts{};
        Check("RKNN_QUERY_IN_OUT_NUM", rknn_query(reference.value, RKNN_QUERY_IN_OUT_NUM, &counts, sizeof(counts)));
        if (counts.n_input != 1 || !counts.n_output) throw std::runtime_error("only exactly one image input is supported");
        normal_inputs = QueryAttrs(reference.value, RKNN_QUERY_INPUT_ATTR, counts.n_input);
        normal_outputs = QueryAttrs(reference.value, RKNN_QUERY_OUTPUT_ATTR, counts.n_output);
        native_outputs = QueryAttrs(reference.value, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, counts.n_output);
        const rknn_tensor_attr& normal_input = normal_inputs.front();
        if (normal_input.fmt != RKNN_TENSOR_NHWC || normal_input.n_dims != 4 || normal_input.dims[0] != 1 || input_bytes.size() != normal_input.n_elems)
            throw std::runtime_error("input must be exact logical uint8 NHWC bytes");
        rnet_width = arguments.id == "rnet" ? normal_input.dims[2] : 0;
        rnet_w_stride = arguments.id == "rnet" ? normal_input.w_stride : 0;
        scrfd_output_count = arguments.id.rfind("scrfd_", 0) == 0 ? counts.n_output : 0;
        attitude_output_count = arguments.id == "attitude" ? counts.n_output : 0;
        if ((arguments.id == "rnet" && (rnet_width != 24 || rnet_w_stride != 32)) ||
            (arguments.id.rfind("scrfd_", 0) == 0 && scrfd_output_count != 9) ||
            (arguments.id == "attitude" && attitude_output_count != 3)) throw std::runtime_error("model-specific queried output/stride contract mismatch");

        failure_stage = "production_init";
        std::vector<InputTensorInfo> ignored_inputs;
        std::vector<OutputTensorInfo> declared_outputs;
        for (const auto& output : normal_outputs) declared_outputs.emplace_back(output.name, TensorInfo::TensorTypeFp32, false);
        InferenceWrapperRKNNAdapter production;
        if (production.Initialize(reinterpret_cast<char*>(model.data()), static_cast<int>(model.size()), ignored_inputs, declared_outputs) != InferenceWrapper::WrapperOk)
            throw std::runtime_error("production Initialize failed");
        InputTensorInfo declared_input(normal_input.name, TensorInfo::TensorTypeUint8, false);
        declared_input.tensor_dims.assign(normal_input.dims, normal_input.dims + normal_input.n_dims);
        declared_input.data = input_bytes.data();
        failure_stage = "warmup";
        for (int iteration = 0; iteration < kWarmupIterations; ++iteration) RunProduction(production, declared_input, &declared_outputs);
        metrics.assign(normal_outputs.size(), Metrics{});
        all_finite = true;
        failure_stage = "parity";
        for (int iteration = 0; iteration < kMeasuredIterations; ++iteration) {
            const auto reference_started = std::chrono::steady_clock::now();
            const auto reference_values = RunReference(reference.value, input_bytes, normal_outputs);
            reference_latency_ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - reference_started).count());
            const auto production_started = std::chrono::steady_clock::now();
            const auto production_values = RunProduction(production, declared_input, &declared_outputs);
            production_latency_ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - production_started).count());
            if (reference_values.size() != production_values.size()) throw std::runtime_error("output count mismatch");
            for (size_t output = 0; output < metrics.size(); ++output) {
                const Metrics current = Compare(reference_values[output], production_values[output]);
                metrics[output].max_abs = std::max(metrics[output].max_abs, current.max_abs);
                metrics[output].mean_abs += current.mean_abs / kMeasuredIterations;
                metrics[output].cosine = std::min(metrics[output].cosine, current.cosine);
                metrics[output].finite = metrics[output].finite && current.finite;
                all_finite = all_finite && current.finite;
            }
        }
        accepted = all_finite;
        for (const auto& metric : metrics) accepted = accepted && metric.max_abs <= kMaxAbsLimit && metric.cosine >= kCosineLimit;
        peak_rss_kb = PeakRssKb();
        if (peak_rss_kb == 0) throw std::runtime_error("unable to record peak RSS");
        production.Finalize();
        status = accepted ? "success" : "failed";
        if (!accepted) failure_stage = "acceptance";
    } catch (const std::exception& caught) { error = caught.what(); }

    std::ostringstream report;
    report << std::setprecision(10) << "{\"model_id\":" << Quote(arguments.id) << ",\"status\":" << Quote(status)
           << ",\"failure_stage\":" << Quote(status == "success" ? "" : failure_stage) << ",\"error\":" << Quote(error)
           << ",\"model_sha256\":" << Quote(arguments.model_sha256) << ",\"input_sha256\":" << Quote(arguments.input_sha256)
           << ",\"api_version\":" << Quote(version.api_version) << ",\"runtime_version\":" << Quote(version.api_version) << ",\"driver_version\":" << Quote(version.drv_version)
           << ",\"normal_inputs\":";
    AppendAttrs(report, normal_inputs);
    report << ",\"normal_outputs\":"; AppendAttrs(report, normal_outputs);
    report << ",\"native_outputs\":"; AppendAttrs(report, native_outputs);
    // Nano's binding is deliberately uint8/NHWC/pass_through=0 even when the normal query is int8.
    report << ",\"binding_inputs\":[{\"type\":\"UINT8\",\"layout\":\"NHWC\",\"pass_through\":0}]"
           << ",\"warmup_iterations\":" << kWarmupIterations << ",\"measured_iterations\":" << kMeasuredIterations
           << ",\"reference_latency_ms\":[";
    for (size_t index = 0; index < reference_latency_ms.size(); ++index) report << (index ? "," : "") << reference_latency_ms[index];
    report << "],\"production_latency_ms\":[";
    for (size_t index = 0; index < production_latency_ms.size(); ++index) report << (index ? "," : "") << production_latency_ms[index];
    report << "],\"latency_summary_ms\":{\"reference_mean\":" << (reference_latency_ms.empty() ? 0 : std::accumulate(reference_latency_ms.begin(), reference_latency_ms.end(), 0.0) / reference_latency_ms.size())
           << ",\"reference_median\":" << Percentile(reference_latency_ms, .5) << ",\"reference_p95\":" << Percentile(reference_latency_ms, .95)
           << ",\"production_mean\":" << (production_latency_ms.empty() ? 0 : std::accumulate(production_latency_ms.begin(), production_latency_ms.end(), 0.0) / production_latency_ms.size())
           << ",\"production_median\":" << Percentile(production_latency_ms, .5) << ",\"production_p95\":" << Percentile(production_latency_ms, .95) << "}"
           << ",\"outputs\":[";
    for (size_t index = 0; index < normal_outputs.size(); ++index) {
        if (index) report << ',';
        report << "{\"index\":" << index << ",\"name\":" << Quote(normal_outputs[index].name)
               << ",\"logical_dims\":[";
        for (uint32_t dimension = 0; dimension < normal_outputs[index].n_dims; ++dimension)
            report << (dimension ? "," : "") << normal_outputs[index].dims[dimension];
        const Metrics metric = index < metrics.size() ? metrics[index] : Metrics{};
        report << "],\"type\":" << Quote(get_type_string(normal_outputs[index].type)) << ",\"qnt_type\":" << Quote(get_qnt_type_string(normal_outputs[index].qnt_type))
               << ",\"scale\":" << normal_outputs[index].scale << ",\"zp\":" << normal_outputs[index].zp
               << ",\"finite\":" << (metric.finite ? "true" : "false") << ",\"max_abs\":" << metric.max_abs
               << ",\"mean_abs\":" << metric.mean_abs << ",\"cosine\":" << metric.cosine << "}";
    }
    report << "],\"all_finite\":" << (all_finite ? "true" : "false") << ",\"accepted\":" << (accepted ? "true" : "false")
           << ",\"rnet_stride\":{\"width\":" << rnet_width << ",\"w_stride\":" << rnet_w_stride << "}"
           << ",\"scrfd_output_count\":" << scrfd_output_count << ",\"attitude_output_count\":" << attitude_output_count
           << ",\"peak_rss_kb\":" << peak_rss_kb << "}";
    if (arguments.result.empty() || !WriteReport(arguments.result, report.str())) {
        std::cerr << "unable to write parity result file\n";
        return 1;
    }
    return status == "success" ? 0 : 1;
}
