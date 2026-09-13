// Compare the public RKNN reference path with InspireFace's RKNN2 Nano wrapper.
// One process handles one model.  The launcher owns aggregation so a failed model
// cannot prevent evidence being collected for the remaining selected models.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
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

// The runner is deliberately self-contained: the ARMHF deployment must not
// acquire an OpenSSL dependency merely to write compact diagnostic evidence.
class Sha256 {
public:
    void Update(const uint8_t* input, size_t size) {
        if (input == nullptr && size != 0) throw std::runtime_error("null SHA-256 input");
        bit_count_ += static_cast<uint64_t>(size) * 8;
        while (size != 0) {
            const size_t copied = std::min(size, block_.size() - block_size_);
            std::memcpy(block_.data() + block_size_, input, copied);
            block_size_ += copied;
            input += copied;
            size -= copied;
            if (block_size_ == block_.size()) {
                Transform(block_.data());
                block_size_ = 0;
            }
        }
    }

    std::array<uint8_t, 32> Final() {
        const uint64_t message_bits = bit_count_;
        block_[block_size_++] = 0x80;
        if (block_size_ > 56) {
            std::fill(block_.begin() + block_size_, block_.end(), 0);
            Transform(block_.data());
            block_size_ = 0;
        }
        std::fill(block_.begin() + block_size_, block_.begin() + 56, 0);
        for (size_t index = 0; index < 8; ++index)
            block_[63 - index] = static_cast<uint8_t>(message_bits >> (index * 8));
        Transform(block_.data());
        std::array<uint8_t, 32> result{};
        for (size_t index = 0; index < state_.size(); ++index) {
            result[index * 4] = static_cast<uint8_t>(state_[index] >> 24);
            result[index * 4 + 1] = static_cast<uint8_t>(state_[index] >> 16);
            result[index * 4 + 2] = static_cast<uint8_t>(state_[index] >> 8);
            result[index * 4 + 3] = static_cast<uint8_t>(state_[index]);
        }
        return result;
    }

private:
    static uint32_t RotateRight(uint32_t value, uint32_t amount) { return (value >> amount) | (value << (32 - amount)); }

    void Transform(const uint8_t* block) {
        static const uint32_t constants[] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };
        std::array<uint32_t, 64> words{};
        for (size_t index = 0; index < 16; ++index)
            words[index] = (static_cast<uint32_t>(block[index * 4]) << 24) | (static_cast<uint32_t>(block[index * 4 + 1]) << 16) |
                           (static_cast<uint32_t>(block[index * 4 + 2]) << 8) | block[index * 4 + 3];
        for (size_t index = 16; index < words.size(); ++index) {
            const uint32_t lower0 = RotateRight(words[index - 15], 7) ^ RotateRight(words[index - 15], 18) ^ (words[index - 15] >> 3);
            const uint32_t lower1 = RotateRight(words[index - 2], 17) ^ RotateRight(words[index - 2], 19) ^ (words[index - 2] >> 10);
            words[index] = words[index - 16] + lower0 + words[index - 7] + lower1;
        }
        uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3], e = state_[4], f = state_[5], g = state_[6], h = state_[7];
        for (size_t index = 0; index < words.size(); ++index) {
            const uint32_t sum1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
            const uint32_t choose = (e & f) ^ (~e & g);
            const uint32_t temporary1 = h + sum1 + choose + constants[index] + words[index];
            const uint32_t sum0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
            const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t temporary2 = sum0 + majority;
            h = g; g = f; f = e; e = d + temporary1; d = c; c = b; b = a; a = temporary1 + temporary2;
        }
        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    std::array<uint32_t, 8> state_{{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}};
    std::array<uint8_t, 64> block_{};
    size_t block_size_ = 0;
    uint64_t bit_count_ = 0;
};

std::string Sha256Hex(const uint8_t* data, size_t size) {
    Sha256 hash;
    hash.Update(data, size);
    const auto digest = hash.Final();
    std::ostringstream stream;
    for (const uint8_t byte : digest) stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    return stream.str();
}

std::string Sha256Hex(const std::vector<uint8_t>& bytes) { return Sha256Hex(bytes.data(), bytes.size()); }

bool RuntimeAtLeast(const char* api_version, int required_major, int required_minor, int required_patch) {
    int major = -1, minor = -1, patch = -1;
    // RKNN appends a parenthesized build identifier, so parse only its first token.
    if (!api_version || std::sscanf(api_version, "%d.%d.%d", &major, &minor, &patch) != 3) return false;
    return major > required_major || (major == required_major &&
        (minor > required_minor || (minor == required_minor && patch >= required_patch)));
}

uint64_t PeakRssKb() {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss <= 0) return 0;
    // Linux reports ru_maxrss in KiB (unlike macOS, where the unit is bytes).
    return static_cast<uint64_t>(usage.ru_maxrss);
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

size_t TensorTypeBytes(rknn_tensor_type type) {
    if (type == RKNN_TENSOR_FLOAT32) return sizeof(float);
    if (type == RKNN_TENSOR_INT8 || type == RKNN_TENSOR_UINT8) return 1;
    return 0;
}

std::vector<float> DecodeRawOutput(const rknn_output& output, const rknn_tensor_attr& attr) {
    const size_t element_bytes = TensorTypeBytes(attr.type);
    if (!output.buf || element_bytes == 0 || attr.n_elems > std::numeric_limits<size_t>::max() / element_bytes ||
        output.size < attr.n_elems * element_bytes) {
        throw std::runtime_error("reference raw output byte count/type mismatch");
    }
    const auto* bytes = static_cast<const uint8_t*>(output.buf);
    std::vector<float> values(attr.n_elems);
    for (size_t index = 0; index < values.size(); ++index) {
        if (attr.type == RKNN_TENSOR_FLOAT32) {
            std::memcpy(&values[index], bytes + index * sizeof(float), sizeof(float));
        } else if (attr.type == RKNN_TENSOR_INT8) {
            values[index] = (static_cast<int32_t>(reinterpret_cast<const int8_t*>(bytes)[index]) - attr.zp) * attr.scale;
        } else {
            values[index] = (static_cast<int32_t>(bytes[index]) - attr.zp) * attr.scale;
        }
        if (!std::isfinite(values[index])) throw std::runtime_error("reference raw output is non-finite");
    }
    return values;
}

struct ReferenceRun {
    std::vector<std::vector<float>> values;
    std::vector<std::vector<float>> raw_values;
    std::vector<std::string> raw_hashes;
};

ReferenceRun RunReference(rknn_context context, std::vector<uint8_t>& input,
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

    ReferenceRun result;
    std::vector<rknn_output> raw_outputs(outputs.size());
    for (size_t index = 0; index < raw_outputs.size(); ++index) {
        raw_outputs[index].index = static_cast<uint32_t>(index);
        raw_outputs[index].want_float = 0;
    }
    ReferenceOutputLease lease{context, &raw_outputs, false};
    Check("rknn_outputs_get", rknn_outputs_get(context, raw_outputs.size(), raw_outputs.data(), nullptr));
    lease.acquired = true;
    result.raw_values.resize(outputs.size());
    result.raw_hashes.resize(outputs.size());
    for (size_t index = 0; index < outputs.size(); ++index) {
        result.raw_values[index] = DecodeRawOutput(raw_outputs[index], outputs[index]);
        // rknn_outputs_get(want_float=0) exposes the normal logical raw
        // region.  Hash exactly its declared elements, not any implementation
        // spare bytes returned by the API.
        result.raw_hashes[index] = Sha256Hex(static_cast<const uint8_t*>(raw_outputs[index].buf),
                                             outputs[index].n_elems * TensorTypeBytes(outputs[index].type));
    }
    Check("rknn_outputs_release", rknn_outputs_release(context, raw_outputs.size(), raw_outputs.data()));
    lease.acquired = false;

    for (size_t index = 0; index < raw_outputs.size(); ++index) raw_outputs[index] = {};
    for (size_t index = 0; index < raw_outputs.size(); ++index) {
        raw_outputs[index].index = static_cast<uint32_t>(index);
        raw_outputs[index].want_float = 1;
    }
    Check("rknn_outputs_get", rknn_outputs_get(context, raw_outputs.size(), raw_outputs.data(), nullptr));
    lease.acquired = true;
    result.values.resize(outputs.size());
    for (size_t index = 0; index < outputs.size(); ++index) {
        if (!raw_outputs[index].buf || raw_outputs[index].size != outputs[index].n_elems * sizeof(float))
            throw std::runtime_error("reference float output byte count mismatch");
        const float* source = static_cast<const float*>(raw_outputs[index].buf);
        result.values[index].assign(source, source + outputs[index].n_elems);
    }
    Check("rknn_outputs_release", rknn_outputs_release(context, raw_outputs.size(), raw_outputs.data()));
    lease.acquired = false;
    return result;
}

struct ProductionRun {
    std::vector<std::vector<float>> values;
    std::vector<std::string> native_logical_hashes;
    std::vector<std::string> native_storage_hashes;
};

ProductionRun RunProduction(InferenceWrapperRKNNAdapter& adapter, const InputTensorInfo& input,
                            std::vector<OutputTensorInfo>* declared_outputs, bool capture_native = true) {
    if (adapter.PreProcess({input}) != InferenceWrapper::WrapperOk) throw std::runtime_error("production PreProcess failed");
    if (adapter.Process(*declared_outputs) != InferenceWrapper::WrapperOk) throw std::runtime_error("production Process failed");
    ProductionRun result;
    result.values.resize(declared_outputs->size());
    for (size_t index = 0; index < declared_outputs->size(); ++index) {
        const OutputTensorInfo& output = declared_outputs->at(index);
        const int32_t element_count = output.GetElementNum();
        const float* source = static_cast<const float*>(output.data);
        if (!source || element_count <= 0) throw std::runtime_error("production output has no logical float data");
        result.values[index].assign(source, source + element_count);
    }
    if (!capture_native) return result;
    std::vector<std::vector<uint8_t>> logical_bytes, storage_bytes;
    if (!adapter.CopyNativeOutputBytes(&logical_bytes, &storage_bytes) || logical_bytes.size() != result.values.size() ||
        storage_bytes.size() != result.values.size()) {
        throw std::runtime_error("production native output snapshot failed");
    }
    result.native_logical_hashes.reserve(logical_bytes.size());
    result.native_storage_hashes.reserve(storage_bytes.size());
    for (size_t index = 0; index < logical_bytes.size(); ++index) {
        result.native_logical_hashes.push_back(Sha256Hex(logical_bytes[index]));
        result.native_storage_hashes.push_back(Sha256Hex(storage_bytes[index]));
    }
    return result;
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

void AccumulateMetrics(Metrics* aggregate, const Metrics& current, size_t sample_count) {
    if (aggregate == nullptr || sample_count == 0) throw std::runtime_error("invalid diagnostic metric accumulator");
    aggregate->max_abs = std::max(aggregate->max_abs, current.max_abs);
    aggregate->mean_abs += current.mean_abs / sample_count;
    aggregate->cosine = std::min(aggregate->cosine, current.cosine);
    aggregate->finite = aggregate->finite && current.finite;
}

void AppendMetrics(std::ostream& stream, const Metrics& metric) {
    stream << "{\"max_abs\":" << metric.max_abs << ",\"mean_abs\":" << metric.mean_abs
           << ",\"cosine\":" << metric.cosine << ",\"finite\":" << (metric.finite ? "true" : "false") << "}";
}

struct InputIntegrity {
    std::string initial_sha256, reference_before_sha256, reference_after_sha256;
    std::string production_before_sha256, production_after_sha256;
    bool reference_unchanged = true, production_unchanged = true;
};

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
    std::vector<Metrics> metrics, reference_self_repeat, production_self_repeat, reference_raw_vs_float, reference_raw_vs_production;
    std::vector<double> reference_latency_ms, production_latency_ms;
    std::vector<std::string> reference_raw_hashes, production_native_logical_hashes, production_native_storage_hashes;
    InputIntegrity input_integrity;
    bool all_finite = false, accepted = false;
    uint32_t rnet_width = 0, rnet_w_stride = 0, scrfd_output_count = 0, attitude_output_count = 0;
    uint64_t peak_rss_kb = 0;
    try {
        arguments = Parse(argc, argv);
        std::vector<uint8_t> model = ReadFile(arguments.model);
        std::vector<uint8_t> input_bytes = ReadFile(arguments.input);
        input_integrity.initial_sha256 = Sha256Hex(input_bytes);
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
        for (int iteration = 0; iteration < kWarmupIterations; ++iteration) {
            input_integrity.reference_before_sha256 = Sha256Hex(input_bytes);
            static_cast<void>(RunReference(reference.value, input_bytes, normal_outputs));
            input_integrity.reference_after_sha256 = Sha256Hex(input_bytes);
            input_integrity.reference_unchanged = input_integrity.reference_unchanged &&
                                                  input_integrity.reference_before_sha256 == input_integrity.reference_after_sha256;
            input_integrity.production_before_sha256 = Sha256Hex(input_bytes);
            RunProduction(production, declared_input, &declared_outputs, false);
            input_integrity.production_after_sha256 = Sha256Hex(input_bytes);
            input_integrity.production_unchanged = input_integrity.production_unchanged &&
                                                   input_integrity.production_before_sha256 == input_integrity.production_after_sha256;
        }
        metrics.assign(normal_outputs.size(), Metrics{});
        reference_self_repeat.assign(normal_outputs.size(), Metrics{});
        production_self_repeat.assign(normal_outputs.size(), Metrics{});
        reference_raw_vs_float.assign(normal_outputs.size(), Metrics{});
        reference_raw_vs_production.assign(normal_outputs.size(), Metrics{});
        all_finite = true;
        failure_stage = "parity";
        std::vector<std::vector<float>> previous_reference, previous_production;
        for (int iteration = 0; iteration < kMeasuredIterations; ++iteration) {
            const auto reference_started = std::chrono::steady_clock::now();
            input_integrity.reference_before_sha256 = Sha256Hex(input_bytes);
            const auto reference_run = RunReference(reference.value, input_bytes, normal_outputs);
            input_integrity.reference_after_sha256 = Sha256Hex(input_bytes);
            input_integrity.reference_unchanged = input_integrity.reference_unchanged &&
                                                  input_integrity.reference_before_sha256 == input_integrity.reference_after_sha256;
            reference_latency_ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - reference_started).count());
            const auto production_started = std::chrono::steady_clock::now();
            input_integrity.production_before_sha256 = Sha256Hex(input_bytes);
            const auto production_run = RunProduction(production, declared_input, &declared_outputs);
            input_integrity.production_after_sha256 = Sha256Hex(input_bytes);
            input_integrity.production_unchanged = input_integrity.production_unchanged &&
                                                   input_integrity.production_before_sha256 == input_integrity.production_after_sha256;
            production_latency_ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - production_started).count());
            if (reference_run.values.size() != production_run.values.size() || reference_run.raw_values.size() != production_run.values.size() ||
                reference_run.raw_hashes.size() != production_run.values.size() || production_run.native_logical_hashes.size() != production_run.values.size() ||
                production_run.native_storage_hashes.size() != production_run.values.size()) throw std::runtime_error("output count mismatch");
            for (size_t output = 0; output < metrics.size(); ++output) {
                const Metrics current = Compare(reference_run.values[output], production_run.values[output]);
                AccumulateMetrics(&metrics[output], current, kMeasuredIterations);
                AccumulateMetrics(&reference_raw_vs_float[output], Compare(reference_run.raw_values[output], reference_run.values[output]), kMeasuredIterations);
                AccumulateMetrics(&reference_raw_vs_production[output], Compare(reference_run.raw_values[output], production_run.values[output]), kMeasuredIterations);
                if (iteration != 0) {
                    AccumulateMetrics(&reference_self_repeat[output], Compare(previous_reference[output], reference_run.values[output]), kMeasuredIterations - 1);
                    AccumulateMetrics(&production_self_repeat[output], Compare(previous_production[output], production_run.values[output]), kMeasuredIterations - 1);
                }
                all_finite = all_finite && current.finite;
            }
            previous_reference = reference_run.values;
            previous_production = production_run.values;
            reference_raw_hashes = reference_run.raw_hashes;
            production_native_logical_hashes = production_run.native_logical_hashes;
            production_native_storage_hashes = production_run.native_storage_hashes;
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
        const rknn_tensor_attr& native = native_outputs[index];
        // Process publishes float views.  Keep the raw RKNN type and
        // quantization alongside that declaration so host evidence cannot
        // mistake INT8 native storage for a logical output type.
        report << "],\"type\":\"FP32\",\"logical_type\":\"FP32\",\"native_type\":" << Quote(get_type_string(native.type))
               << ",\"native_qnt_type\":" << Quote(get_qnt_type_string(native.qnt_type)) << ",\"native_scale\":" << native.scale
               << ",\"native_zp\":" << native.zp << ",\"qnt_type\":" << Quote(get_qnt_type_string(normal_outputs[index].qnt_type))
               << ",\"scale\":" << normal_outputs[index].scale << ",\"zp\":" << normal_outputs[index].zp
               << ",\"finite\":" << (metric.finite ? "true" : "false") << ",\"max_abs\":" << metric.max_abs
               << ",\"mean_abs\":" << metric.mean_abs << ",\"cosine\":" << metric.cosine << "}";
    }
    report << "],\"diagnostics\":{\"input_integrity\":{\"initial_sha256\":" << Quote(input_integrity.initial_sha256)
           << ",\"reference_before_sha256\":" << Quote(input_integrity.reference_before_sha256)
           << ",\"reference_after_sha256\":" << Quote(input_integrity.reference_after_sha256)
           << ",\"reference_unchanged\":" << (input_integrity.reference_unchanged ? "true" : "false")
           << ",\"production_before_sha256\":" << Quote(input_integrity.production_before_sha256)
           << ",\"production_after_sha256\":" << Quote(input_integrity.production_after_sha256)
           << ",\"production_unchanged\":" << (input_integrity.production_unchanged ? "true" : "false")
           << "},\"output_diagnostics\":[";
    for (size_t index = 0; index < normal_outputs.size(); ++index) {
        if (index) report << ',';
        report << "{\"index\":" << index << ",\"reference_self_repeat\":";
        AppendMetrics(report, index < reference_self_repeat.size() ? reference_self_repeat[index] : Metrics{});
        report << ",\"production_self_repeat\":";
        AppendMetrics(report, index < production_self_repeat.size() ? production_self_repeat[index] : Metrics{});
        report << ",\"reference_raw_vs_float\":";
        AppendMetrics(report, index < reference_raw_vs_float.size() ? reference_raw_vs_float[index] : Metrics{});
        report << ",\"reference_raw_vs_production\":";
        AppendMetrics(report, index < reference_raw_vs_production.size() ? reference_raw_vs_production[index] : Metrics{});
        report << ",\"reference_raw_logical_sha256\":" << Quote(index < reference_raw_hashes.size() ? reference_raw_hashes[index] : "")
               << ",\"production_native_logical_sha256\":" << Quote(index < production_native_logical_hashes.size() ? production_native_logical_hashes[index] : "")
               << ",\"production_native_storage_sha256\":" << Quote(index < production_native_storage_hashes.size() ? production_native_storage_hashes[index] : "") << '}';
    }
    report << "]},\"all_finite\":" << (all_finite ? "true" : "false") << ",\"accepted\":" << (accepted ? "true" : "false")
           << ",\"rnet_stride\":{\"width\":" << rnet_width << ",\"w_stride\":" << rnet_w_stride << "}"
           << ",\"scrfd_output_count\":" << scrfd_output_count << ",\"attitude_output_count\":" << attitude_output_count
           << ",\"peak_rss_kb\":" << peak_rss_kb << "}";
    if (arguments.result.empty() || !WriteReport(arguments.result, report.str())) {
        std::cerr << "unable to write parity result file\n";
        return 1;
    }
    return status == "success" ? 0 : 1;
}
