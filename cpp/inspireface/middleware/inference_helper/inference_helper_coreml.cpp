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
/*** Include ***/
/* for general */
#ifdef INFERENCE_HELPER_ENABLE_COREML

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>

/* for MNN */
#include <MNN/ImageProcess.hpp>

/* for My modules */
#include "inference_helper_log.h"
#include "inference_helper_coreml.h"
#include "log.h"
/*** Macro ***/
#define TAG "InferenceHelperCoreML"
#define PRINT(...) INFERENCE_HELPER_LOG_PRINT(TAG, __VA_ARGS__)
#define PRINT_E(...) INFERENCE_HELPER_LOG_PRINT_E(TAG, __VA_ARGS__)

using namespace inspire;

/*** Function ***/
InferenceHelperCoreML::InferenceHelperCoreML() {
    num_threads_ = 1;
}

InferenceHelperCoreML::~InferenceHelperCoreML() {}

int32_t InferenceHelperCoreML::SetNumThreads(const int32_t num_threads) {
    num_threads_ = num_threads;
    return kRetOk;
}

int32_t InferenceHelperCoreML::SetCustomOps(const std::vector<std::pair<const char*, const void*>>& custom_ops) {
    PRINT("[WARNING] This method is not supported\n");
    return kRetOk;
}

int32_t InferenceHelperCoreML::ParameterInitialization(std::vector<InputTensorInfo>& input_tensor_info_list,
                                                       std::vector<OutputTensorInfo>& output_tensor_info_list) {
    return kRetOk;
}

int32_t InferenceHelperCoreML::Initialize(char* model_buffer, int model_size, std::vector<InputTensorInfo>& input_tensor_info_list,
                                          std::vector<OutputTensorInfo>& output_tensor_info_list) {
    PRINT_E("CoreML does not yet support buffer initialization of the model\n");
    return kRetErr;
}

int32_t InferenceHelperCoreML::Initialize(const std::string& model_filename, std::vector<InputTensorInfo>& input_tensor_info_list,
                                          std::vector<OutputTensorInfo>& output_tensor_info_list) {
    //    LOG_INFO("init MNN");
    /*** Create network ***/
    net_.reset(new CoreMLAdapter());
    auto ret = net_->readFromFile(model_filename);
    if (ret != kRetOk) {
        PRINT_E("Failed to load model file (%s)\n", model_filename.c_str());
        return kRetErr;
    }
    if (special_backend_ == kCoreMLCPU || special_backend_ == kDefaultCPU) {
        net_->setInferenceMode(CoreMLAdapter::InferenceMode::CPU);
    } else if (special_backend_ == kCoreMLGPU) {
        net_->setInferenceMode(CoreMLAdapter::InferenceMode::GPU);
    } else if (special_backend_ == kCoreMLANE) {
        net_->setInferenceMode(CoreMLAdapter::InferenceMode::ANE);
    } else {
        PRINT_E("Unsupported backend (%d)\n, defaulting to ANE", special_backend_);
        net_->setInferenceMode(CoreMLAdapter::InferenceMode::ANE);
    }
    if (!net_) {
        PRINT_E("Failed to load model file (%s)\n", model_filename.c_str());
        return kRetErr;
    }

    return ParameterInitialization(input_tensor_info_list, output_tensor_info_list);
};

int32_t InferenceHelperCoreML::Finalize(void) {
    net_.reset();
    return kRetOk;
}

int32_t InferenceHelperCoreML::PreProcess(const std::vector<InputTensorInfo>& input_tensor_info_list) {
    // Currently only single-input models are supported
    for (const auto& input_tensor_info : input_tensor_info_list) {
        input_tensor_.reset(MNN::Tensor::create<float>(
          std::vector<int>{1, 3, input_tensor_info.image_info.height, input_tensor_info.image_info.width}, nullptr, MNN::Tensor::CAFFE));
        if (input_tensor_ == nullptr) {
            PRINT_E("Invalid input name (%s)\n", input_tensor_info.name.c_str());
            INSPIRE_LOGE("Invalid input name (%s)\n", input_tensor_info.name.c_str());
            return kRetErr;
        }
        if (input_tensor_info.data_type == InputTensorInfo::kDataTypeImage) {
            /* Crop */
            if ((input_tensor_info.image_info.width != input_tensor_info.image_info.crop_width) ||
                (input_tensor_info.image_info.height != input_tensor_info.image_info.crop_height)) {
                PRINT_E("Crop is not supported\n");
                return kRetErr;
            }

            MNN::CV::ImageProcess::Config image_processconfig;
            /* Convert color type */
            //            LOGD("input_tensor_info.image_info.channel: %d", input_tensor_info.image_info.channel);
            //            LOGD("input_tensor_info.GetChannel(): %d", input_tensor_info.GetChannel());

            // !!!!!! BUG !!!!!!!!!
            // When initializing, setting the image channel to 3 and the tensor channel to 1,
            // and configuring the processing to convert the color image to grayscale may cause some bugs.
            // For example, the image channel might automatically change to 1.
            // This issue has not been fully investigated,
            // so it's necessary to manually convert the image to grayscale before input.
            // !!!!!! BUG !!!!!!!!!

            if ((input_tensor_info.image_info.channel == 3) && (input_tensor_info.GetChannel() == 3)) {
                image_processconfig.sourceFormat = (input_tensor_info.image_info.is_bgr) ? MNN::CV::BGR : MNN::CV::RGB;
                if (input_tensor_info.image_info.swap_color) {
                    image_processconfig.destFormat = (input_tensor_info.image_info.is_bgr) ? MNN::CV::RGB : MNN::CV::BGR;
                } else {
                    image_processconfig.destFormat = (input_tensor_info.image_info.is_bgr) ? MNN::CV::BGR : MNN::CV::RGB;
                }
            } else if ((input_tensor_info.image_info.channel == 1) && (input_tensor_info.GetChannel() == 1)) {
                image_processconfig.sourceFormat = MNN::CV::GRAY;
                image_processconfig.destFormat = MNN::CV::GRAY;
            } else if ((input_tensor_info.image_info.channel == 3) && (input_tensor_info.GetChannel() == 1)) {
                image_processconfig.sourceFormat = (input_tensor_info.image_info.is_bgr) ? MNN::CV::BGR : MNN::CV::RGB;
                image_processconfig.destFormat = MNN::CV::GRAY;
                //                LOGD("2gray");
            } else if ((input_tensor_info.image_info.channel == 1) && (input_tensor_info.GetChannel() == 3)) {
                image_processconfig.sourceFormat = MNN::CV::GRAY;
                image_processconfig.destFormat = MNN::CV::BGR;
            } else {
                PRINT_E("Unsupported color conversion (%d, %d)\n", input_tensor_info.image_info.channel, input_tensor_info.GetChannel());
                return kRetErr;
            }

            /* Normalize image */
            std::memcpy(image_processconfig.mean, input_tensor_info.normalize.mean, sizeof(image_processconfig.mean));
            std::memcpy(image_processconfig.normal, input_tensor_info.normalize.norm, sizeof(image_processconfig.normal));

            /* Resize image */
            image_processconfig.filterType = MNN::CV::BILINEAR;
            MNN::CV::Matrix trans;
            trans.setScale(static_cast<float>(input_tensor_info.image_info.crop_width) / input_tensor_info.GetWidth(),
                           static_cast<float>(input_tensor_info.image_info.crop_height) / input_tensor_info.GetHeight());

            /* Do pre-process */
            std::shared_ptr<MNN::CV::ImageProcess> pretreat(MNN::CV::ImageProcess::create(image_processconfig));
            pretreat->setMatrix(trans);
            //            LOGD("k1");
            pretreat->convert(static_cast<uint8_t*>(input_tensor_info.data), input_tensor_info.image_info.crop_width,
                              input_tensor_info.image_info.crop_height, 0, input_tensor_.get());

        } else if ((input_tensor_info.data_type == InputTensorInfo::kDataTypeBlobNhwc) ||
                   (input_tensor_info.data_type == InputTensorInfo::kDataTypeBlobNchw)) {
            std::unique_ptr<MNN::Tensor> tensor;
            if (input_tensor_info.data_type == InputTensorInfo::kDataTypeBlobNhwc) {
                tensor.reset(new MNN::Tensor(input_tensor_.get(), MNN::Tensor::TENSORFLOW));
            } else {
                tensor.reset(new MNN::Tensor(input_tensor_.get(), MNN::Tensor::CAFFE));
            }
            if (tensor->getType().code == halide_type_float) {
                for (int32_t i = 0; i < input_tensor_info.GetWidth() * input_tensor_info.GetHeight() * input_tensor_info.GetChannel(); i++) {
                    tensor->host<float>()[i] = static_cast<float*>(input_tensor_info.data)[i];
                }
            } else {
                for (int32_t i = 0; i < input_tensor_info.GetWidth() * input_tensor_info.GetHeight() * input_tensor_info.GetChannel(); i++) {
                    tensor->host<uint8_t>()[i] = static_cast<uint8_t*>(input_tensor_info.data)[i];
                }
            }
            input_tensor_->copyFromHostTensor(tensor.get());
        } else {
            PRINT_E("Unsupported data type (%d)\n", input_tensor_info.data_type);
            return kRetErr;
        }
        auto p = input_tensor_->host<float>();
        net_->setInput(input_tensor_info.name.c_str(), reinterpret_cast<const char*>(p));
    }
    return kRetOk;
}

int32_t InferenceHelperCoreML::Process(std::vector<OutputTensorInfo>& output_tensor_info_list) {
    auto ret = net_->forward();
    if (ret != COREML_HSUCCEED) {
        PRINT_E("Failed to forward\n");
        return kRetErr;
    }

    // out_mat_list_.clear();
    for (auto& output_tensor_info : output_tensor_info_list) {
        // auto output_tensor = net_->getSessionOutput(session_, output_tensor_info.name.c_str());
        const char* output_tensor = net_->getOutput(output_tensor_info.name.c_str());
        if (output_tensor == nullptr) {
            PRINT_E("Invalid output name (%s)\n", output_tensor_info.name.c_str());
            return kRetErr;
        }
        output_tensor_info.data = (void*)output_tensor;

        output_tensor_info.tensor_dims.clear();

        const std::vector<int>& output_shape = net_->getOutputShapeByName(output_tensor_info.name);

        int size = 0;
        for (int32_t dim = 0; dim < output_shape.size(); dim++) {
            output_tensor_info.tensor_dims.push_back(output_shape[dim]);
            size *= output_shape[dim];
        }
    }

    return kRetOk;
}

std::vector<std::string> InferenceHelperCoreML::GetInputNames() {
    return input_names_;
}

int32_t InferenceHelperCoreML::ResizeInput(const std::vector<InputTensorInfo>& input_tensor_info_list) {
    PRINT_E("Currently, CoreML does not support input resizing\n");
    return 0;
}

#endif  // INFERENCE_HELPER_ENABLE_COREML
