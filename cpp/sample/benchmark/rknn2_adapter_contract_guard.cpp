#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>

#include "middleware/inference_wrapper/inference_wrapper_rknn_adapter_nano.h"

namespace {

using Clock = std::chrono::steady_clock;

enum class FakeFault {
    None,
    InputElementCount,
    InputStorageOverflow,
    NativeOutputElementCount,
    NativePaddingOverflow,
    NormalOutputElementCount,
    NonAffineIntegerOutput,
    NonFiniteScale,
    InvalidZeroPoint,
    Fp16Output,
    DfpOutput,
    NonFiniteResult,
};

enum class FakeOutputLayout {
    Default,
    NativeNhwcToNormalNchw,
    NativeNc1hwc2ToNormalNchw,
};

struct FakeRuntimeState {
    std::vector<rknn_tensor_mem *> memories;
    int init_calls{0};
    int destroy_calls{0};
    int create_calls{0};
    int destroy_memory_calls{0};
    int run_calls{0};
    int set_io_calls{0};
    int fail_create_call{-1};
    bool fail_init{false};
    bool fail_set_io{false};
    bool fail_run{false};
    bool rnet_input{false};
    bool legacy_fp32_nchw_input{false};
    FakeFault fault{FakeFault::None};
    FakeOutputLayout output_layout{FakeOutputLayout::Default};
    rknn_query_cmd fail_query{RKNN_QUERY_CMD_MAX};
} runtime;

void ResetRuntime() {
    runtime = {};
}

void FillAttribute(rknn_tensor_attr &attribute, bool input, bool native_output) {
    const uint32_t index = attribute.index;
    std::memset(&attribute, 0, sizeof(attribute));
    attribute.index = index;
    attribute.qnt_type = RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC;
    attribute.scale = 0.25f;
    attribute.zp = 1;
    attribute.fmt = RKNN_TENSOR_NHWC;
    if (input) {
        attribute.n_dims = 4;
        attribute.dims[0] = 1;
        attribute.dims[1] = 2;
        attribute.dims[2] = 3;
        attribute.dims[3] = 2;
        attribute.n_elems = 12;
        attribute.size = 12;
        attribute.w_stride = 4;
        attribute.size_with_stride = 16;
        // The model's queried contract is quantized int8; callers bind uint8
        // image bytes through RKNN's conversion mode.
        attribute.type = RKNN_TENSOR_INT8;
        std::strncpy(attribute.name, "input", sizeof(attribute.name) - 1);
        if (runtime.legacy_fp32_nchw_input) {
            attribute.fmt = RKNN_TENSOR_NCHW;
            attribute.type = RKNN_TENSOR_FLOAT32;
            attribute.qnt_type = RKNN_TENSOR_QNT_NONE;
            attribute.dims[0] = 1;
            attribute.dims[1] = 2;
            attribute.dims[2] = 2;
            attribute.dims[3] = 3;
            attribute.size = 12 * sizeof(float);
            attribute.w_stride = 4;
            attribute.size_with_stride = 16 * sizeof(float);
        }
        if (runtime.rnet_input) {
            attribute.dims[0] = 1;
            attribute.dims[1] = 24;
            attribute.dims[2] = 24;
            attribute.dims[3] = 3;
            attribute.n_elems = 1728;
            attribute.size = 1728;
            attribute.w_stride = 32;
            attribute.size_with_stride = 2304;
        }
        if (runtime.fault == FakeFault::InputElementCount) {
            ++attribute.n_elems;
        } else if (runtime.fault == FakeFault::InputStorageOverflow) {
            attribute.w_stride = std::numeric_limits<uint32_t>::max();
            attribute.size_with_stride = std::numeric_limits<uint32_t>::max();
        }
        return;
    }
    if (index == 0) {
        attribute.n_dims = 4;
        attribute.dims[0] = 1;
        attribute.dims[1] = 2;
        attribute.dims[2] = 2;
        attribute.dims[3] = 2;
        attribute.n_elems = 8;
        attribute.size = 8;
        attribute.w_stride = native_output ? 3 : 2;
        attribute.size_with_stride = native_output ? 12 : 8;
        attribute.type = RKNN_TENSOR_INT8;
        std::strncpy(attribute.name, "quantized", sizeof(attribute.name) - 1);
    } else {
        attribute.n_dims = 4;
        attribute.dims[0] = 1;
        attribute.dims[1] = 1;
        attribute.dims[2] = 1;
        attribute.dims[3] = 3;
        attribute.n_elems = 3;
        attribute.size = 3 * sizeof(float);
        attribute.w_stride = 1;
        attribute.size_with_stride = 3 * sizeof(float);
        attribute.type = RKNN_TENSOR_FLOAT32;
        attribute.qnt_type = RKNN_TENSOR_QNT_NONE;
        std::strncpy(attribute.name, "floating", sizeof(attribute.name) - 1);
    }

    if (index == 0 && runtime.output_layout == FakeOutputLayout::NativeNhwcToNormalNchw) {
        if (native_output) {
            attribute.fmt = RKNN_TENSOR_NHWC;
            attribute.dims[0] = 1;
            attribute.dims[1] = 1;
            attribute.dims[2] = 2;
            attribute.dims[3] = 2;
            attribute.n_elems = 4;
            attribute.size = 4;
            attribute.w_stride = 3;
            attribute.size_with_stride = 6;
        } else {
            attribute.fmt = RKNN_TENSOR_NCHW;
            attribute.dims[0] = 1;
            attribute.dims[1] = 2;
            attribute.dims[2] = 1;
            attribute.dims[3] = 2;
            attribute.n_elems = 4;
            attribute.size = 4;
            attribute.w_stride = 2;
            attribute.size_with_stride = 4;
        }
    }
    if (index == 0 && runtime.output_layout == FakeOutputLayout::NativeNc1hwc2ToNormalNchw) {
        if (native_output) {
            attribute.fmt = RKNN_TENSOR_NC1HWC2;
            attribute.n_dims = 5;
            attribute.dims[0] = 1;
            attribute.dims[1] = 2;
            attribute.dims[2] = 1;
            attribute.dims[3] = 2;
            attribute.dims[4] = 2;
            attribute.n_elems = 8;
            attribute.size = 8;
            attribute.w_stride = 2;
            attribute.size_with_stride = 8;
        } else {
            attribute.fmt = RKNN_TENSOR_NCHW;
            attribute.dims[0] = 1;
            attribute.dims[1] = 3;
            attribute.dims[2] = 1;
            attribute.dims[3] = 2;
            attribute.n_elems = 6;
            attribute.size = 6;
            attribute.w_stride = 2;
            attribute.size_with_stride = 6;
        }
    }
    if (index == 0 && native_output) {
        if (runtime.fault == FakeFault::NativeOutputElementCount) {
            ++attribute.n_elems;
        } else if (runtime.fault == FakeFault::NativePaddingOverflow) {
            attribute.fmt = RKNN_TENSOR_NC1HWC2;
            attribute.n_dims = 5;
            attribute.dims[0] = 1;
            attribute.dims[1] = std::numeric_limits<uint32_t>::max();
            attribute.dims[2] = std::numeric_limits<uint32_t>::max();
            attribute.dims[3] = std::numeric_limits<uint32_t>::max();
            attribute.dims[4] = 2;
            attribute.n_elems = 1;
            attribute.size = 1;
            attribute.w_stride = std::numeric_limits<uint32_t>::max();
            attribute.size_with_stride = 1;
        } else if (runtime.fault == FakeFault::NonAffineIntegerOutput) {
            attribute.qnt_type = RKNN_TENSOR_QNT_NONE;
        } else if (runtime.fault == FakeFault::NonFiniteScale) {
            attribute.scale = std::numeric_limits<float>::quiet_NaN();
        } else if (runtime.fault == FakeFault::InvalidZeroPoint) {
            attribute.zp = 256;
        } else if (runtime.fault == FakeFault::Fp16Output) {
            attribute.type = RKNN_TENSOR_FLOAT16;
        } else if (runtime.fault == FakeFault::DfpOutput) {
            attribute.qnt_type = RKNN_TENSOR_QNT_DFP;
        }
    }
    if (index == 0 && !native_output && runtime.fault == FakeFault::NormalOutputElementCount) {
        ++attribute.n_elems;
    }
}

double Percentile(std::vector<double> values, double ratio) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const size_t index = static_cast<size_t>(ratio * static_cast<double>(values.size() - 1));
    return values[index];
}

bool AlmostEqual(float left, float right) {
    return std::fabs(left - right) <= 1e-6f;
}

bool OutputsAreCleared(const std::vector<OutputTensorInfo>& outputs) {
    for (const auto& output : outputs) {
        if (output.data != nullptr || !output.tensor_dims.empty() || output.quant.scale != 1.0f || output.quant.zero_point != 0) {
            return false;
        }
    }
    return true;
}

void SeedStaleOutputs(std::vector<OutputTensorInfo>& outputs) {
    for (auto& output : outputs) {
        output.data = reinterpret_cast<void*>(static_cast<uintptr_t>(1));
        output.tensor_dims = {1, 7};
        output.quant.scale = 0.25f;
        output.quant.zero_point = 17;
    }
}

bool RejectInput(InferenceWrapperRKNNAdapter& adapter, const InputTensorInfo& input) {
    const int set_io_before = runtime.set_io_calls;
    const int run_before = runtime.run_calls;
    return adapter.PreProcess({input}) == InferenceWrapper::WrapperError && runtime.set_io_calls == set_io_before &&
           runtime.run_calls == run_before;
}

bool TestExactCallerContracts(InferenceWrapperRKNNAdapter& adapter, const std::array<uint8_t, 12>& input_pixels,
                              std::vector<OutputTensorInfo>& prior_success_outputs) {
    InputTensorInfo valid("input", TensorInfo::TensorTypeUint8, false);
    valid.tensor_dims = {1, 2, 3, 2};
    valid.data = const_cast<uint8_t*>(input_pixels.data());

    bool passed = true;
    InputTensorInfo wrong_height = valid;
    wrong_height.tensor_dims = {1, 1, 3, 2};
    passed = passed && RejectInput(adapter, wrong_height);

    InputTensorInfo wrong_batch = valid;
    wrong_batch.tensor_dims = {2, 2, 3, 2};
    passed = passed && RejectInput(adapter, wrong_batch);

    InputTensorInfo wrong_name = valid;
    wrong_name.name = "wrong";
    passed = passed && RejectInput(adapter, wrong_name);

    InputTensorInfo wrong_layout = valid;
    wrong_layout.is_nchw = true;
    wrong_layout.tensor_dims = {1, 2, 2, 3};
    passed = passed && RejectInput(adapter, wrong_layout);

    InputTensorInfo wrong_type = valid;
    wrong_type.tensor_type = TensorInfo::TensorTypeInt8;
    passed = passed && RejectInput(adapter, wrong_type);

    InputTensorInfo null_data = valid;
    null_data.data = nullptr;
    passed = passed && RejectInput(adapter, null_data);

    const int set_io_before = runtime.set_io_calls;
    const int run_before = runtime.run_calls;
    passed = passed && adapter.PreProcess({valid, valid}) == InferenceWrapper::WrapperError &&
             runtime.set_io_calls == set_io_before && runtime.run_calls == run_before;

    prior_success_outputs[0].name = "floating";
    prior_success_outputs[1].name = "quantized";
    passed = passed && adapter.Process(prior_success_outputs) == InferenceWrapper::WrapperError &&
             OutputsAreCleared(prior_success_outputs) && runtime.run_calls == run_before;
    prior_success_outputs[0].name = "quantized";
    prior_success_outputs[1].name = "floating";

    std::vector<OutputTensorInfo> wrong_type_outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeInt8, false),
      OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
    };
    passed = passed && adapter.Process(wrong_type_outputs) == InferenceWrapper::WrapperError &&
             OutputsAreCleared(wrong_type_outputs) && runtime.run_calls == run_before;

    std::vector<OutputTensorInfo> missing_output = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
    };
    passed = passed && adapter.Process(missing_output) == InferenceWrapper::WrapperError && OutputsAreCleared(missing_output) &&
             runtime.run_calls == run_before;
    passed = passed && adapter.PreProcess({valid}) == InferenceWrapper::WrapperOk;

    std::cout << "RKNN2_ADAPTER_EXACT_CONTRACTS,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestQueriedAndBindingAttributeRoles() {
    ResetRuntime();
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::array<uint8_t, 12> input_pixels = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    RKNNAdapterNano adapter;
    bool passed = adapter.Initialize(model.data(), static_cast<unsigned int>(model.size())) == 0;
    if (passed) {
        const auto& normal_inputs = adapter.GetNormalInputAttrs();
        const auto& bindings = adapter.GetInputBindingAttrs();
        const auto& normal_outputs = adapter.GetNormalOutputAttrs();
        const auto& native_outputs = adapter.GetNativeOutputAttrs();
        passed = normal_inputs.size() == 1 && bindings.size() == 1 && normal_outputs.size() == 2 && native_outputs.size() == 2 &&
                 normal_inputs[0].type == RKNN_TENSOR_INT8 && normal_inputs[0].fmt == RKNN_TENSOR_NHWC &&
                 bindings[0].type == RKNN_TENSOR_UINT8 && bindings[0].fmt == RKNN_TENSOR_NHWC &&
                 bindings[0].pass_through == 0 && std::string(normal_outputs[0].name) == "quantized" &&
                 std::string(normal_outputs[1].name) == "floating" && native_outputs[0].w_stride == 3;
        passed = passed && adapter.SetInputData(0, input_pixels.data(), 3, 2, 2) == 0 &&
                 adapter.GetNormalInputAttrs()[0].type == RKNN_TENSOR_INT8 &&
                 adapter.GetNormalInputAttrs()[0].fmt == RKNN_TENSOR_NHWC;
    }
    adapter.Release();
    passed = passed && runtime.memories.empty();
    std::cout << "RKNN2_ADAPTER_ATTRIBUTE_ROLES,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestLegacyFp32NchwInput() {
    ResetRuntime();
    runtime.legacy_fp32_nchw_input = true;
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::array<float, 12> input_pixels = {{0.0f}};
    std::vector<OutputTensorInfo> outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
      OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
    };
    InputTensorInfo input("input", TensorInfo::TensorTypeFp32, true);
    input.tensor_dims = {1, 2, 2, 3};
    input.data = input_pixels.data();
    std::vector<InputTensorInfo> inputs = {input};
    InferenceWrapperRKNNAdapter adapter;
    bool passed = adapter.Initialize(model.data(), static_cast<int>(model.size()), inputs, outputs) == InferenceWrapper::WrapperOk &&
                  adapter.PreProcess(inputs) == InferenceWrapper::WrapperOk && runtime.set_io_calls == 1;
    adapter.Finalize();
    passed = passed && runtime.memories.empty();
    std::cout << "RKNN2_ADAPTER_LEGACY_FP32_NCHW,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestRNetStrideCopyClearsPadding() {
    ResetRuntime();
    runtime.rnet_input = true;
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::vector<uint8_t> first(1728, 0xff);
    std::vector<uint8_t> second(1728, 3);
    RKNNAdapterNano adapter;
    bool passed = adapter.Initialize(model.data(), static_cast<unsigned int>(model.size())) == 0 && runtime.memories.size() == 3;
    if (passed) {
        auto* storage = static_cast<uint8_t*>(runtime.memories[0]->virt_addr);
        std::memset(storage, 0xa5, runtime.memories[0]->size);
        passed = adapter.SetInputData(0, first.data(), 24, 24, 3) == 0;
        for (size_t row = 0; passed && row < 24; ++row) {
            const size_t offset = row * 96;
            passed = std::equal(first.begin() + row * 72, first.begin() + (row + 1) * 72, storage + offset) &&
                     std::all_of(storage + offset + 72, storage + offset + 96, [](uint8_t value) { return value == 0; });
        }
        std::memset(storage, 0x5a, runtime.memories[0]->size);
        passed = passed && adapter.SetInputData(0, second.data(), 24, 24, 3) == 0;
        for (size_t row = 0; passed && row < 24; ++row) {
            const size_t offset = row * 96;
            passed = std::equal(second.begin() + row * 72, second.begin() + (row + 1) * 72, storage + offset) &&
                     std::all_of(storage + offset + 72, storage + offset + 96, [](uint8_t value) { return value == 0; });
        }
    }
    adapter.Release();
    passed = passed && runtime.memories.empty();
    std::cout << "RKNN2_ADAPTER_RNET_STRIDE,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestRejectsMalformedNativeContracts() {
    std::array<char, 4> model = {{1, 2, 3, 4}};
    bool passed = true;
    for (const auto fault : {FakeFault::InputElementCount, FakeFault::InputStorageOverflow, FakeFault::NativeOutputElementCount,
                             FakeFault::NativePaddingOverflow, FakeFault::NormalOutputElementCount,
                             FakeFault::NonAffineIntegerOutput, FakeFault::NonFiniteScale, FakeFault::InvalidZeroPoint,
                             FakeFault::Fp16Output, FakeFault::DfpOutput}) {
        ResetRuntime();
        runtime.fault = fault;
        RKNNAdapterNano adapter;
        passed = adapter.Initialize(model.data(), static_cast<unsigned int>(model.size())) != 0 && runtime.memories.empty() && passed;
    }
    std::cout << "RKNN2_ADAPTER_NATIVE_CONTRACTS,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestNativeLayoutsAndFailureCleanup() {
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::array<uint8_t, 12> input = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    bool passed = true;
    for (const auto layout : {FakeOutputLayout::NativeNhwcToNormalNchw, FakeOutputLayout::NativeNc1hwc2ToNormalNchw}) {
        ResetRuntime();
        runtime.output_layout = layout;
        RKNNAdapterNano adapter;
        passed = adapter.Initialize(model.data(), static_cast<unsigned int>(model.size())) == 0 &&
                 adapter.SetInputData(0, input.data(), 3, 2, 2) == 0 && adapter.RunSession(true) == 0 && passed;
        const std::vector<float> expected = layout == FakeOutputLayout::NativeNhwcToNormalNchw
                                              ? std::vector<float>{2.25f, 7.25f, 4.75f, 9.75f}
                                              : std::vector<float>{2.25f, 2.5f, 4.75f, 5.0f, 7.25f, 7.5f};
        const auto* output = adapter.GetOutputDataPtr(0);
        passed = output != nullptr && std::equal(expected.begin(), expected.end(), output, AlmostEqual) && passed;
        adapter.Release();
        passed = runtime.memories.empty() && passed;
    }

    ResetRuntime();
    runtime.fault = FakeFault::NonFiniteResult;
    RKNNAdapterNano adapter;
    passed = adapter.Initialize(model.data(), static_cast<unsigned int>(model.size())) == 0 &&
             adapter.SetInputData(0, input.data(), 3, 2, 2) == 0 && adapter.RunSession(true) != 0 &&
             adapter.GetOutputDataPtr(0) == nullptr && passed;
    adapter.Release();
    std::cout << "RKNN2_ADAPTER_NATIVE_LAYOUTS,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestExecutionFailuresClearResults() {
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::array<uint8_t, 12> input_bytes = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    bool passed = true;
    for (int mode = 0; mode < 3; ++mode) {
        ResetRuntime();
        runtime.fault = mode == 2 ? FakeFault::NonFiniteResult : FakeFault::None;
        std::vector<InputTensorInfo> inputs;
        std::vector<OutputTensorInfo> outputs = {
          OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
          OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
        };
        InputTensorInfo input("input", TensorInfo::TensorTypeUint8, false);
        input.tensor_dims = {1, 2, 3, 2};
        input.data = input_bytes.data();
        InferenceWrapperRKNNAdapter adapter;
        passed = adapter.Initialize(model.data(), static_cast<int>(model.size()), inputs, outputs) == InferenceWrapper::WrapperOk &&
                 adapter.PreProcess({input}) == InferenceWrapper::WrapperOk && passed;
        if (mode == 0) {
            runtime.fail_set_io = true;
        } else if (mode == 1) {
            runtime.fail_run = true;
        }
        passed = adapter.Process(outputs) == InferenceWrapper::WrapperError && OutputsAreCleared(outputs) && passed;
        adapter.Finalize();
        passed = runtime.memories.empty() && passed;
    }
    std::cout << "RKNN2_ADAPTER_EXECUTION_FAILURES,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestInitializationRejectsOutputDeclarations() {
    std::array<char, 4> model = {{1, 2, 3, 4}};
    bool passed = true;
    for (const auto& outputs : {
           std::vector<OutputTensorInfo>{OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
                                         OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false)},
           std::vector<OutputTensorInfo>{OutputTensorInfo("quantized", TensorInfo::TensorTypeInt8, false),
                                         OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false)},
         }) {
        ResetRuntime();
        std::vector<InputTensorInfo> empty_inputs;
        auto declared_outputs = outputs;
        InferenceWrapperRKNNAdapter adapter;
        passed = passed && adapter.Initialize(model.data(), static_cast<int>(model.size()), empty_inputs, declared_outputs) ==
                             InferenceWrapper::WrapperError &&
                 runtime.set_io_calls == 0 && runtime.run_calls == 0 && runtime.memories.empty();
    }
    std::cout << "RKNN2_ADAPTER_INITIALIZATION_CONTRACTS,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestInputReadinessAndExternalOutputInvalidation() {
    ResetRuntime();
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::array<uint8_t, 12> pixels = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    std::vector<InputTensorInfo> empty_inputs;
    std::vector<OutputTensorInfo> outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
      OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
    };
    InferenceWrapperRKNNAdapter adapter;
    InputTensorInfo valid("input", TensorInfo::TensorTypeUint8, false);
    valid.tensor_dims = {1, 2, 3, 2};
    valid.data = pixels.data();

    bool passed = adapter.Initialize(model.data(), static_cast<int>(model.size()), empty_inputs, outputs) == InferenceWrapper::WrapperOk &&
                  adapter.PreProcess({valid}) == InferenceWrapper::WrapperOk &&
                  adapter.Process(outputs) == InferenceWrapper::WrapperOk && !OutputsAreCleared(outputs);

    const int runs_after_success = runtime.run_calls;
    InputTensorInfo wrong_name = valid;
    wrong_name.name = "wrong";
    passed = passed && adapter.PreProcess({wrong_name}) == InferenceWrapper::WrapperError &&
             adapter.Process(outputs) == InferenceWrapper::WrapperError && OutputsAreCleared(outputs) &&
             runtime.run_calls == runs_after_success;
    passed = passed && adapter.PreProcess({valid}) == InferenceWrapper::WrapperOk &&
             adapter.Process(outputs) == InferenceWrapper::WrapperOk && runtime.run_calls == runs_after_success + 1;

    const int runs_after_recovery = runtime.run_calls;
    InputTensorInfo wrong_shape = valid;
    wrong_shape.tensor_dims = {1, 1, 3, 2};
    passed = passed && adapter.PreProcess({wrong_shape}) == InferenceWrapper::WrapperError &&
             adapter.Process(outputs) == InferenceWrapper::WrapperError && OutputsAreCleared(outputs) &&
             runtime.run_calls == runs_after_recovery;
    passed = passed && adapter.Finalize() == InferenceWrapper::WrapperOk && OutputsAreCleared(outputs);
    std::cout << "RKNN2_ADAPTER_INPUT_READINESS,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestFinalizeDoesNotBorrowExpiredOutputMetadata() {
    ResetRuntime();
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::array<uint8_t, 12> pixels = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    std::vector<InputTensorInfo> empty_inputs;
    std::vector<OutputTensorInfo> initialization_outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
      OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
    };
    InputTensorInfo valid("input", TensorInfo::TensorTypeUint8, false);
    valid.tensor_dims = {1, 2, 3, 2};
    valid.data = pixels.data();
    bool passed = true;
    {
        InferenceWrapperRKNNAdapter adapter;
        passed = adapter.Initialize(model.data(), static_cast<int>(model.size()), empty_inputs, initialization_outputs) ==
                     InferenceWrapper::WrapperOk &&
                 adapter.PreProcess({valid}) == InferenceWrapper::WrapperOk;
        {
            std::vector<OutputTensorInfo> local_outputs = {
              OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
              OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
            };
            passed = passed && adapter.Process(local_outputs) == InferenceWrapper::WrapperOk && !OutputsAreCleared(local_outputs);
        }
        passed = passed && adapter.Finalize() == InferenceWrapper::WrapperOk;
    }
    passed = passed && runtime.memories.empty() && runtime.destroy_calls == 1;
    std::cout << "RKNN2_ADAPTER_EXPIRED_OUTPUT_METADATA,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestInitializeClearsCallerOutputMetadata() {
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::vector<InputTensorInfo> empty_inputs;
    std::vector<OutputTensorInfo> outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
      OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
    };
    bool passed = true;

    ResetRuntime();
    SeedStaleOutputs(outputs);
    {
        InferenceWrapperRKNNAdapter adapter;
        passed = passed && adapter.Initialize("unsupported.rknn", empty_inputs, outputs) == InferenceWrapper::WrapperError &&
                 OutputsAreCleared(outputs) && runtime.memories.empty();
    }

    ResetRuntime();
    SeedStaleOutputs(outputs);
    {
        InferenceWrapperRKNNAdapter adapter;
        passed = passed && adapter.Initialize(nullptr, 0, empty_inputs, outputs) == InferenceWrapper::WrapperError &&
                 OutputsAreCleared(outputs) && runtime.memories.empty();
    }

    ResetRuntime();
    runtime.fail_init = true;
    SeedStaleOutputs(outputs);
    {
        InferenceWrapperRKNNAdapter adapter;
        passed = passed && adapter.Initialize(model.data(), static_cast<int>(model.size()), empty_inputs, outputs) ==
                             InferenceWrapper::WrapperError &&
                 OutputsAreCleared(outputs) && runtime.memories.empty();
    }
    std::cout << "RKNN2_ADAPTER_INITIALIZE_CLEARS_OUTPUTS,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestWrapperContracts() {
    ResetRuntime();
    bool passed = true;
    std::vector<InputTensorInfo> initialization_inputs;
    std::vector<OutputTensorInfo> outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
      OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
    };

    InferenceWrapperRKNNAdapter adapter;
    passed = passed && adapter.Initialize(nullptr, 0, initialization_inputs, outputs) == InferenceWrapper::WrapperError;
    passed = passed && adapter.Initialize("unsupported.rknn", initialization_inputs, outputs) == InferenceWrapper::WrapperError;
    passed = passed && adapter.ResizeInput(initialization_inputs) == InferenceWrapper::WrapperError;
    passed = passed && adapter.PreProcess(initialization_inputs) == InferenceWrapper::WrapperError;
    std::vector<OutputTensorInfo> empty_outputs;
    passed = passed && adapter.Process(empty_outputs) == InferenceWrapper::WrapperError;

    std::array<char, 8> model = {{1, 2, 3, 4, 5, 6, 7, 8}};
    passed = passed && adapter.Initialize(model.data(), static_cast<int>(model.size()), initialization_inputs, outputs) ==
                           InferenceWrapper::WrapperOk;

    std::array<uint8_t, 12> input_pixels = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    InputTensorInfo input("input", TensorInfo::TensorTypeUint8, false);
    input.tensor_dims = {1, 2, 3, 2};
    input.data = input_pixels.data();
    passed = passed && adapter.PreProcess({input}) == InferenceWrapper::WrapperOk;
    passed = passed && runtime.memories.size() == 3;
    if (runtime.memories.size() == 3) {
        const auto *copied = static_cast<const uint8_t *>(runtime.memories[0]->virt_addr);
        passed = passed && std::equal(input_pixels.begin(), input_pixels.begin() + 6, copied);
        passed = passed && copied[6] == 0 && copied[7] == 0;
        passed = passed && std::equal(input_pixels.begin() + 6, input_pixels.end(), copied + 8);
        passed = passed && copied[14] == 0 && copied[15] == 0;
    }

    passed = passed && adapter.Process(outputs) == InferenceWrapper::WrapperOk;
    const std::vector<float> expected_quantized = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
    const std::vector<float> expected_floating = {0.25f, -1.5f, 2.0f};
    passed = passed && outputs[0].tensor_dims == std::vector<int32_t>({1, 2, 2, 2});
    passed = passed && outputs[1].tensor_dims == std::vector<int32_t>({1, 1, 1, 3});
    for (size_t index = 0; index < expected_quantized.size() && outputs[0].data != nullptr; ++index) {
        passed = passed && AlmostEqual(static_cast<float *>(outputs[0].data)[index], expected_quantized[index]);
    }
    for (size_t index = 0; index < expected_floating.size() && outputs[1].data != nullptr; ++index) {
        passed = passed && AlmostEqual(static_cast<float *>(outputs[1].data)[index], expected_floating[index]);
    }

    passed = passed && TestExactCallerContracts(adapter, input_pixels, outputs);

    std::vector<double> latency_us;
    latency_us.reserve(256);
    for (int iteration = 0; iteration < 256; ++iteration) {
        const auto begin = Clock::now();
        if (adapter.Process(outputs) != InferenceWrapper::WrapperOk) {
            passed = false;
            break;
        }
        const auto end = Clock::now();
        latency_us.push_back(std::chrono::duration<double, std::micro>(end - begin).count());
        passed = passed && AlmostEqual(static_cast<float *>(outputs[0].data)[7], 7.0f);
        passed = passed && AlmostEqual(static_cast<float *>(outputs[1].data)[1], -1.5f);
    }
    const double mean_us = latency_us.empty()
                             ? 0.0
                             : std::accumulate(latency_us.begin(), latency_us.end(), 0.0) / latency_us.size();
    const double p95_us = Percentile(latency_us, 0.95);
    passed = passed && p95_us < 1000.0;

    std::vector<OutputTensorInfo> mismatched_outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
    };
    passed = passed && adapter.Process(mismatched_outputs) == InferenceWrapper::WrapperError;
    passed = passed && mismatched_outputs[0].data == nullptr;
    passed = passed && adapter.Finalize() == InferenceWrapper::WrapperOk;
    passed = passed && adapter.Finalize() == InferenceWrapper::WrapperOk;
    passed = passed && runtime.destroy_memory_calls == 3 && runtime.destroy_calls == 1 && runtime.memories.empty();

    std::cout << "RKNN2_ADAPTER_ACCURACY,mean_us=" << mean_us << ",p95_us=" << p95_us
              << ",runs=" << latency_us.size() << ",status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestFailureCleanup() {
    ResetRuntime();
    runtime.fail_create_call = 2;
    std::array<char, 4> model = {{1, 2, 3, 4}};
    std::vector<InputTensorInfo> inputs;
    std::vector<OutputTensorInfo> outputs = {
      OutputTensorInfo("quantized", TensorInfo::TensorTypeFp32, false),
      OutputTensorInfo("floating", TensorInfo::TensorTypeFp32, false),
    };
    InferenceWrapperRKNNAdapter adapter;
    const bool initialization_failed =
      adapter.Initialize(model.data(), static_cast<int>(model.size()), inputs, outputs) == InferenceWrapper::WrapperError;
    const bool cleaned = runtime.memories.empty() && runtime.destroy_memory_calls == 1 && runtime.destroy_calls == 1;
    const bool idempotent = adapter.Finalize() == InferenceWrapper::WrapperOk && runtime.destroy_calls == 1;
    const bool passed = initialization_failed && cleaned && idempotent;
    std::cout << "RKNN2_ADAPTER_FAILURE_CLEANUP,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

bool TestNumericHelpers() {
    bool passed = softmax({}).empty();
    const std::vector<float> probabilities = softmax({1000.0f, 1001.0f, 1002.0f});
    passed = passed && probabilities.size() == 3;
    passed = passed && AlmostEqual(std::accumulate(probabilities.begin(), probabilities.end(), 0.0f), 1.0f);

    const int dimensions[5] = {2, 2, 1, 2, 2};
    const std::array<int8_t, 16> source = {{1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18}};
    std::array<float, 12> destination = {};
    NC1HWC2_int8_to_NCHW_float(source.data(), destination.data(), const_cast<int *>(dimensions), 3, 1, 2, 0, 1.0f);
    const std::array<float, 12> expected = {{1, 3, 2, 4, 5, 7, 11, 13, 12, 14, 15, 17}};
    passed = passed && std::equal(destination.begin(), destination.end(), expected.begin());
    std::cout << "RKNN2_ADAPTER_NUMERIC_HELPERS,status=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

}  // namespace

extern "C" {

int rknn_init(rknn_context *context, void *model, uint32_t size, uint32_t, rknn_init_extend *) {
    ++runtime.init_calls;
    if (runtime.fail_init || context == nullptr || model == nullptr || size == 0) {
        return -1;
    }
    *context = 1;
    return RKNN_SUCC;
}

int rknn_destroy(rknn_context) {
    ++runtime.destroy_calls;
    return RKNN_SUCC;
}

int rknn_query(rknn_context, rknn_query_cmd command, void *information, uint32_t) {
    if (command == runtime.fail_query || information == nullptr) {
        return -1;
    }
    switch (command) {
        case RKNN_QUERY_SDK_VERSION: {
            auto *version = static_cast<rknn_sdk_version *>(information);
            std::strncpy(version->api_version, "mock-api", sizeof(version->api_version) - 1);
            std::strncpy(version->drv_version, "mock-driver", sizeof(version->drv_version) - 1);
            return RKNN_SUCC;
        }
        case RKNN_QUERY_IN_OUT_NUM: {
            auto *count = static_cast<rknn_input_output_num *>(information);
            count->n_input = 1;
            count->n_output = 2;
            return RKNN_SUCC;
        }
        case RKNN_QUERY_INPUT_ATTR:
            FillAttribute(*static_cast<rknn_tensor_attr *>(information), true, false);
            return RKNN_SUCC;
        case RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR:
            FillAttribute(*static_cast<rknn_tensor_attr *>(information), false, true);
            return RKNN_SUCC;
        case RKNN_QUERY_OUTPUT_ATTR:
            FillAttribute(*static_cast<rknn_tensor_attr *>(information), false, false);
            return RKNN_SUCC;
        case RKNN_QUERY_CUSTOM_STRING:
            std::strncpy(static_cast<rknn_custom_string *>(information)->string, "mock", 4);
            return RKNN_SUCC;
        default:
            return -1;
    }
}

rknn_tensor_mem *rknn_create_mem(rknn_context, uint32_t size) {
    ++runtime.create_calls;
    if (runtime.create_calls == runtime.fail_create_call || size == 0 || size > 16 * 1024 * 1024) {
        return nullptr;
    }
    auto *memory = static_cast<rknn_tensor_mem *>(std::calloc(1, sizeof(rknn_tensor_mem)));
    if (memory == nullptr) {
        return nullptr;
    }
    memory->virt_addr = std::calloc(1, size);
    if (memory->virt_addr == nullptr) {
        std::free(memory);
        return nullptr;
    }
    memory->size = size;
    runtime.memories.push_back(memory);
    return memory;
}

int rknn_destroy_mem(rknn_context, rknn_tensor_mem *memory) {
    if (memory == nullptr) {
        return -1;
    }
    ++runtime.destroy_memory_calls;
    const auto position = std::find(runtime.memories.begin(), runtime.memories.end(), memory);
    if (position == runtime.memories.end()) {
        return -1;
    }
    runtime.memories.erase(position);
    std::free(memory->virt_addr);
    std::free(memory);
    return RKNN_SUCC;
}

int rknn_set_io_mem(rknn_context, rknn_tensor_mem *memory, rknn_tensor_attr *) {
    ++runtime.set_io_calls;
    return memory == nullptr || runtime.fail_set_io ? -1 : RKNN_SUCC;
}

int rknn_run(rknn_context, rknn_run_extend *) {
    ++runtime.run_calls;
    if (runtime.memories.size() != 3 || runtime.fail_run) {
        return -1;
    }
    if (runtime.output_layout == FakeOutputLayout::NativeNhwcToNormalNchw) {
        const std::array<int8_t, 6> quantized = {{10, 20, 30, 40, 99, 99}};
        std::memcpy(runtime.memories[1]->virt_addr, quantized.data(), quantized.size());
        const std::array<float, 3> floating = {{0.25f, -1.5f, 2.0f}};
        std::memcpy(runtime.memories[2]->virt_addr, floating.data(), sizeof(floating));
        return RKNN_SUCC;
    }
    if (runtime.output_layout == FakeOutputLayout::NativeNc1hwc2ToNormalNchw) {
        const std::array<int8_t, 8> quantized = {{10, 20, 11, 21, 30, 99, 31, 99}};
        std::memcpy(runtime.memories[1]->virt_addr, quantized.data(), quantized.size());
        const std::array<float, 3> floating = {{0.25f, -1.5f, 2.0f}};
        std::memcpy(runtime.memories[2]->virt_addr, floating.data(), sizeof(floating));
        return RKNN_SUCC;
    }
    const std::array<int8_t, 12> quantized = {{1, 5, 9, 13, 99, 99, 17, 21, 25, 29, 99, 99}};
    std::memcpy(runtime.memories[1]->virt_addr, quantized.data(), quantized.size());
    const std::array<float, 3> floating = {{runtime.fault == FakeFault::NonFiniteResult ? std::numeric_limits<float>::quiet_NaN() : 0.25f,
                                             -1.5f, 2.0f}};
    std::memcpy(runtime.memories[2]->virt_addr, floating.data(), sizeof(floating));
    return RKNN_SUCC;
}

}  // extern "C"

int main() {
#if defined(ISF_RKNN2_LEGACY_GUARD_ONLY)
    return TestLegacyFp32NchwInput() ? 0 : 1;
#else
    const bool passed = TestWrapperContracts() && TestQueriedAndBindingAttributeRoles() &&
                        TestRNetStrideCopyClearsPadding() && TestRejectsMalformedNativeContracts() &&
                        TestNativeLayoutsAndFailureCleanup() && TestExecutionFailuresClearResults() &&
                        TestInitializationRejectsOutputDeclarations() && TestInputReadinessAndExternalOutputInvalidation() &&
                        TestFinalizeDoesNotBorrowExpiredOutputMetadata() && TestInitializeClearsCallerOutputMetadata() &&
                        TestFailureCleanup() && TestNumericHelpers();
    return passed ? 0 : 1;
#endif
}
