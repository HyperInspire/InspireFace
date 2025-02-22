/* Copyright 2021 iwatake2222

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef INFERENCE_HELPER_COREML_
#define INFERENCE_HELPER_COREML_

/* for general */
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <memory>

/* for CoreML */
#include "coreml/CoreMLAdapter.h"

/* for MNN ImageProcess */
#include <MNN/ImageProcess.hpp>

/* for My modules */
#include "inference_helper.h"

class InferenceHelperCoreML : public InferenceHelper {
public:
    InferenceHelperCoreML();
    ~InferenceHelperCoreML() override;
    int32_t SetNumThreads(const int32_t num_threads) override;
    int32_t SetCustomOps(const std::vector<std::pair<const char*, const void*>>& custom_ops) override;
    int32_t Initialize(const std::string& model_filename, std::vector<InputTensorInfo>& input_tensor_info_list,
                       std::vector<OutputTensorInfo>& output_tensor_info_list) override;
    int32_t Initialize(char* model_buffer, int model_size, std::vector<InputTensorInfo>& input_tensor_info_list,
                       std::vector<OutputTensorInfo>& output_tensor_info_list) override;
    int32_t Finalize(void) override;
    int32_t PreProcess(const std::vector<InputTensorInfo>& input_tensor_info_list) override;
    int32_t Process(std::vector<OutputTensorInfo>& output_tensor_info_list) override;
    int32_t ParameterInitialization(std::vector<InputTensorInfo>& input_tensor_info_list,
                                    std::vector<OutputTensorInfo>& output_tensor_info_list) override;

    int32_t ResizeInput(const std::vector<InputTensorInfo>& input_tensor_info_list) override;

    std::vector<std::string> GetInputNames() override;

private:
    std::unique_ptr<CoreMLAdapter> net_;
    int32_t num_threads_;

    std::vector<std::string> input_names_;

    /** Using MNN imageprocess to do Image Preprocessing */
    std::unique_ptr<MNN::Tensor> input_tensor_;
};

#endif  // INFERENCE_HELPER_COREML_
