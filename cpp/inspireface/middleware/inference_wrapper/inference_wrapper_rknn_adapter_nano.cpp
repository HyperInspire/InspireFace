/**
 * Created by Jingyu Yan
 * @date 2024-10-01
 */

#ifdef INFERENCE_WRAPPER_ENABLE_RKNN2

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <limits>
#include "inference_wrapper_rknn_adapter_nano.h"
#include "inference_wrapper_log.h"
#include "log.h"

#define TAG "InferenceWrapperRKNNAdapter"
#define PRINT(...) INFERENCE_WRAPPER_LOG_PRINT(TAG, __VA_ARGS__)
#define PRINT_E(...) INFERENCE_WRAPPER_LOG_PRINT_E(TAG, __VA_ARGS__)

namespace {

bool ExactInputContract(const InputTensorInfo &input, const rknn_tensor_attr &normal) {
    if (normal.n_dims != 4 || normal.fmt != RKNN_TENSOR_NHWC || normal.type != RKNN_TENSOR_INT8 ||
        normal.dims[0] != 1 || input.name != normal.name || input.data == nullptr || input.is_nchw ||
        input.tensor_type != TensorInfo::TensorTypeUint8 || input.tensor_dims.size() != normal.n_dims) {
        return false;
    }
    for (size_t index = 0; index < input.tensor_dims.size(); ++index) {
        if (input.tensor_dims[index] <= 0 || static_cast<uint32_t>(input.tensor_dims[index]) != normal.dims[index]) {
            return false;
        }
    }
    return true;
}

bool ExactOutputContracts(const std::vector<OutputTensorInfo> &outputs,
                          const std::vector<rknn_tensor_attr> &normal_outputs) {
    if (outputs.size() != normal_outputs.size()) {
        return false;
    }
    for (size_t index = 0; index < outputs.size(); ++index) {
        if (outputs[index].name != normal_outputs[index].name || outputs[index].tensor_type != TensorInfo::TensorTypeFp32) {
            return false;
        }
    }
    return true;
}

void ClearOutputs(std::vector<OutputTensorInfo> &outputs) {
    for (auto &output : outputs) {
        output.data = nullptr;
        output.tensor_dims.clear();
        output.quant.scale = 1.0f;
        output.quant.zero_point = 0;
    }
}

}  // namespace

InferenceWrapperRKNNAdapter::InferenceWrapperRKNNAdapter() {
    num_threads_ = 1;
}

InferenceWrapperRKNNAdapter::~InferenceWrapperRKNNAdapter() {
    Finalize();
}

int32_t InferenceWrapperRKNNAdapter::SetNumThreads(const int32_t num_threads) {
    num_threads_ = num_threads;
    return WrapperOk;
}

int32_t InferenceWrapperRKNNAdapter::ParameterInitialization(std::vector<InputTensorInfo> &input_tensor_info_list,
                                                             std::vector<OutputTensorInfo> &output_tensor_info_list) {
    (void)input_tensor_info_list;
    if (net_ == nullptr || !ExactOutputContracts(output_tensor_info_list, net_->GetNormalOutputAttrs())) {
        return WrapperError;
    }
    return WrapperOk;
}

int32_t InferenceWrapperRKNNAdapter::Process(std::vector<OutputTensorInfo> &output_tensor_info_list) {
    ClearOutputs(output_tensor_info_list);
    if (net_ != nullptr) {
        net_->ClearOutputData();
    }
    if (net_ == nullptr || !input_ready_ || !ExactOutputContracts(output_tensor_info_list, net_->GetNormalOutputAttrs())) {
        INSPIRE_LOGE("RKNN2 runtime or output metadata is not initialized.");
        input_ready_ = false;
        return WrapperError;
    }

    auto ret = net_->RunSession(true);
    if (ret != 0) {
        INSPIRE_LOGE("Run model error.");
        input_ready_ = false;
        return WrapperError;
    }
    auto outputs_size = net_->GetNormalOutputAttrs().size();
    if (outputs_size != output_tensor_info_list.size()) {
        INSPIRE_LOGE("RKNN2 output count mismatch: runtime=%zu, expected=%zu", outputs_size, output_tensor_info_list.size());
        input_ready_ = false;
        ClearOutputs(output_tensor_info_list);
        net_->ClearOutputData();
        return WrapperError;
    }

    std::vector<const float *> output_views(outputs_size, nullptr);
    std::vector<std::vector<int32_t>> output_dimensions(outputs_size);
    for (size_t index = 0; index < outputs_size; ++index) {
        const float *output_view = net_->GetOutputDataPtr(static_cast<int>(index));
        if (output_view == nullptr) {
            INSPIRE_LOGE("RKNN2 output %zu has no data", index);
            input_ready_ = false;
            ClearOutputs(output_tensor_info_list);
            net_->ClearOutputData();
            return WrapperError;
        }
        output_views[index] = output_view;

        auto dim = net_->GetOutputTensorSize(static_cast<int>(index));
        if (dim.empty()) {
            input_ready_ = false;
            ClearOutputs(output_tensor_info_list);
            net_->ClearOutputData();
            return WrapperError;
        }
        output_dimensions[index].reserve(dim.size());
        for (const auto dimension : dim) {
            if (dimension == 0 || dimension > static_cast<unsigned long>(std::numeric_limits<int32_t>::max())) {
                input_ready_ = false;
                ClearOutputs(output_tensor_info_list);
                net_->ClearOutputData();
                return WrapperError;
            }
            output_dimensions[index].push_back(static_cast<int32_t>(dimension));
        }
        TensorInfo checked;
        checked.tensor_dims = output_dimensions[index];
        if (checked.GetElementNum() <= 0) {
            input_ready_ = false;
            ClearOutputs(output_tensor_info_list);
            net_->ClearOutputData();
            return WrapperError;
        }
    }
    for (size_t index = 0; index < outputs_size; ++index) {
        output_tensor_info_list[index].data = const_cast<float *>(output_views[index]);
        output_tensor_info_list[index].tensor_dims = std::move(output_dimensions[index]);
    }
    return WrapperOk;
}

int32_t InferenceWrapperRKNNAdapter::PreProcess(const std::vector<InputTensorInfo> &input_tensor_info_list) {
    input_ready_ = false;
    if (net_ == nullptr) {
        return WrapperError;
    }
    net_->ClearOutputData();
    const auto &normal_inputs = net_->GetNormalInputAttrs();
    if (input_tensor_info_list.empty() || input_tensor_info_list.size() != normal_inputs.size()
#if defined(ISF_RKNPU_RV1126B)
        || normal_inputs.size() != 1 || !ExactInputContract(input_tensor_info_list.front(), normal_inputs.front())
#endif
        ) {
        return WrapperError;
    }
    for (size_t i = 0; i < input_tensor_info_list.size(); ++i) {
        const auto &input_tensor_info = input_tensor_info_list[i];
        const int width = input_tensor_info.GetWidth();
        const int height = input_tensor_info.GetHeight();
        const int channel = input_tensor_info.GetChannel();
        if (width <= 0 || height <= 0 || channel <= 0) {
            INSPIRE_LOGE("Invalid RKNN2 input tensor metadata.");
            return WrapperError;
        }
        rknn_tensor_type type = RKNN_TENSOR_UINT8;
        rknn_tensor_format format = input_tensor_info.is_nchw ? RKNN_TENSOR_NCHW : RKNN_TENSOR_NHWC;
#if defined(ISF_RKNPU_RV1126B)
        format = RKNN_TENSOR_NHWC;
#else
        if (input_tensor_info.tensor_type == TensorInfo::TensorTypeFp32) {
            type = RKNN_TENSOR_FLOAT32;
        } else if (input_tensor_info.tensor_type == TensorInfo::TensorTypeInt8) {
            type = RKNN_TENSOR_INT8;
        } else if (input_tensor_info.tensor_type != TensorInfo::TensorTypeUint8) {
            return WrapperError;
        }
#endif
        auto ret = net_->SetInputData(static_cast<int>(i), input_tensor_info.data, width, height, channel, type, format);
        if (ret != 0) {
            INSPIRE_LOGE("Set data error.");
            return ret;
        }
    }
    input_ready_ = true;
    return WrapperOk;
}

int32_t InferenceWrapperRKNNAdapter::Initialize(const std::string &model_filename, std::vector<InputTensorInfo> &input_tensor_info_list,
                                                std::vector<OutputTensorInfo> &output_tensor_info_list) {
    (void)model_filename;
    (void)input_tensor_info_list;
    input_ready_ = false;
    Finalize();
    ClearOutputs(output_tensor_info_list);
    INSPIRE_LOGE("NOT IMPL");

    return WrapperError;
}

int32_t InferenceWrapperRKNNAdapter::Initialize(char *model_buffer, int model_size, std::vector<InputTensorInfo> &input_tensor_info_list,
                                                std::vector<OutputTensorInfo> &output_tensor_info_list) {
    input_ready_ = false;
    Finalize();
    ClearOutputs(output_tensor_info_list);
    if (model_buffer == nullptr || model_size <= 0 || output_tensor_info_list.empty()) {
        return WrapperError;
    }
    net_ = std::make_shared<RKNNAdapterNano>();
    auto ret = net_->Initialize((unsigned char *)model_buffer, model_size);
    if (ret != 0) {
        INSPIRE_LOGE("Rknn init error.");
        net_.reset();
        return WrapperError;
    }
    ret = ParameterInitialization(input_tensor_info_list, output_tensor_info_list);
    if (ret != WrapperOk) {
        Finalize();
    }
    return ret;
}

int32_t InferenceWrapperRKNNAdapter::Finalize(void) {
    input_ready_ = false;
    if (net_ != nullptr) {
        net_->Release();
        net_.reset();
    }
    return WrapperOk;
}

std::vector<std::string> InferenceWrapperRKNNAdapter::GetInputNames() {
    return std::vector<std::string>();
}

bool InferenceWrapperRKNNAdapter::CopyNativeOutputBytes(std::vector<std::vector<uint8_t>>* logical_bytes,
                                                        std::vector<std::vector<uint8_t>>* storage_bytes) const {
    if (logical_bytes == nullptr || storage_bytes == nullptr) return false;
    logical_bytes->clear();
    storage_bytes->clear();
    if (net_ == nullptr) return false;
    const auto& attrs = net_->GetNativeOutputAttrs();
    logical_bytes->reserve(attrs.size());
    storage_bytes->reserve(attrs.size());
    for (size_t index = 0; index < attrs.size(); ++index) {
        const rknn_tensor_mem* memory = net_->GetOutputRawData(index);
        const rknn_tensor_attr& attr = attrs[index];
        if (memory == nullptr || memory->virt_addr == nullptr || attr.size == 0 || attr.size > attr.size_with_stride ||
            memory->size < attr.size_with_stride) {
            logical_bytes->clear();
            storage_bytes->clear();
            return false;
        }
        const auto* source = static_cast<const uint8_t*>(memory->virt_addr);
        logical_bytes->emplace_back(source, source + attr.size);
        storage_bytes->emplace_back(source, source + attr.size_with_stride);
    }
    return true;
}

int32_t InferenceWrapperRKNNAdapter::ResizeInput(const std::vector<InputTensorInfo> &input_tensor_info_list) {
    // The function is not supported
    (void)input_tensor_info_list;
    return WrapperError;
}

#endif  // INFERENCE_WRAPPER_ENABLE_RKNN2
