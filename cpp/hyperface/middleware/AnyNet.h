//
// Created by tunm on 2023/5/6.
//
#pragma once
#ifndef BIGGUYSMAIN_ANYNET_H
#define BIGGUYSMAIN_ANYNET_H
#include "../DataType.h"
#include "inference_helper/inference_helper.h"
#include "Parameter.h"
#include "model_loader/ModelLoader.h"
#include <assert.h>
#include "opencv2/opencv.hpp"
#include "../log.h"

namespace hyper {

using AnyTensorOutputs = std::vector<std::pair<std::string, std::vector<float>>>;

class HYPER_API AnyNet {
    PARAMETERIZATION_SUPPORT  // NOSONAR

public:

    explicit AnyNet(const std::string &name):m_name_(name) {}

    int32_t LoadParam(const Parameter &param, Model *model, InferenceHelper::HelperType type = InferenceHelper::kMnn) {
        // must
        _initSingleParam<int>(param, "model_index", 0);
        _initSingleParam<std::string>(param, "input_layer", "");
        _initSingleParam<std::vector<std::string>>(param, "outputs_layers", {"", });
        _initSingleParam<std::vector<int>>(param, "input_size", {320, 320});
        _initSingleParam<std::vector<float>>(param, "mean", {127.5f, 127.5f, 127.5f});
        _initSingleParam<std::vector<float>>(param, "norm", {0.0078125f, 0.0078125f, 0.0078125f});
        // rarely
        _initSingleParam<int>(param, "input_channel", 3);
        _initSingleParam<int>(param, "input_image_channel", 3);
        _initSingleParam<bool>(param, "nchw", true);
        _initSingleParam<bool>(param, "swap_color", false);
        _initSingleParam<int>(param, "data_type", InputTensorInfo::kDataTypeImage);
        _initSingleParam<int>(param, "tensor_type", InputTensorInfo::kTensorTypeFp32);
        _initSingleParam<int>(param, "threads", 1);

        int model_index = getParam<int>("model_index");
        m_nn_inference_.reset(InferenceHelper::Create(type));
        m_nn_inference_->SetNumThreads(getParam<int>("threads"));

        m_output_tensor_info_list_.clear();
        std::vector<std::string> outputs_layers = getParam<std::vector<std::string>>("outputs_layers");
        int tensor_type = getParam<int>("tensor_type");
        for (auto &name: outputs_layers) {
            m_output_tensor_info_list_.push_back(OutputTensorInfo(name, tensor_type));
        }
        auto ret = m_nn_inference_->Initialize(model->caffemodelBuffer, model->modelsize.caffemodel_size, m_input_tensor_info_list_, m_output_tensor_info_list_);
        if (ret != InferenceHelper::kRetOk) {
            LOGE("NN Initialize fail");
        }

        m_input_tensor_info_list_.clear();
        InputTensorInfo input_tensor_info(getParam<std::string>("input_layer"), tensor_type, getParam<bool>("nchw"));
        std::vector<int> input_size = getParam<std::vector<int>>("input_size");
        int width = input_size[0];
        int height = input_size[1];
        m_input_image_size_ = {width, height};
        int channel = getParam<int>("input_channel");
        input_tensor_info.tensor_dims =  { 1, channel, m_input_image_size_.height, m_input_image_size_.width };
        input_tensor_info.data_type = getParam<int>("data_type");
        int image_channel = getParam<int>("input_image_channel");
        input_tensor_info.image_info.channel = image_channel;

        std::vector<float> mean = getParam<std::vector<float>>("mean");
        std::vector<float> norm = getParam<std::vector<float>>("norm");
        input_tensor_info.normalize.mean[0] = mean[0];
        input_tensor_info.normalize.mean[1] = mean[1];
        input_tensor_info.normalize.mean[2] = mean[2];
        input_tensor_info.normalize.norm[0] = norm[0];
        input_tensor_info.normalize.norm[1] = norm[1];
        input_tensor_info.normalize.norm[2] = norm[2];

        input_tensor_info.data_type = getParam<int>("data_type");
        input_tensor_info.image_info.width = width;
        input_tensor_info.image_info.height = height;
        input_tensor_info.image_info.channel = channel;
        input_tensor_info.image_info.crop_x = 0;
        input_tensor_info.image_info.crop_y = 0;
        input_tensor_info.image_info.crop_width = width;
        input_tensor_info.image_info.crop_height = height;
        input_tensor_info.image_info.is_bgr = getParam<bool>("nchw");
        input_tensor_info.image_info.swap_color = getParam<bool>("swap_color");

        m_input_tensor_info_list_.push_back(input_tensor_info);

        return 0;
    }

    void Forward(const Matrix &data, AnyTensorOutputs& outputs) {
        InputTensorInfo& input_tensor_info = getMInputTensorInfoList()[0];
        input_tensor_info.data = data.data;
        Forward(outputs);
    }

    void Forward(AnyTensorOutputs& outputs) {

        if (m_nn_inference_->PreProcess(m_input_tensor_info_list_) != InferenceHelper::kRetOk) {
            LOGD("PreProcess error");
        }
        if (m_nn_inference_->Process(m_output_tensor_info_list_) != InferenceHelper::kRetOk) {
            LOGD("Process error");
        }
        for (int i = 0; i < m_output_tensor_info_list_.size(); ++i) {
            std::vector<float> output_score_raw_list(m_output_tensor_info_list_[i].GetDataAsFloat(),
                                                     m_output_tensor_info_list_[i].GetDataAsFloat() +
                                                     m_output_tensor_info_list_[i].GetElementNum());
            outputs.push_back(std::make_pair(m_output_tensor_info_list_[i].name, output_score_raw_list));
        }
    }


public:

    vector<InputTensorInfo> &getMInputTensorInfoList() {
        return m_input_tensor_info_list_;
    }

    vector<OutputTensorInfo> &getMOutputTensorInfoList() {
        return m_output_tensor_info_list_;
    }

    cv::Size &getMInputImageSize() {
        return m_input_image_size_;
    }

protected:
    std::string m_name_;

private:

    std::shared_ptr<InferenceHelper> m_nn_inference_;

    std::vector<InputTensorInfo> m_input_tensor_info_list_;

    std::vector<OutputTensorInfo> m_output_tensor_info_list_;

    cv::Size m_input_image_size_{};

};

template <typename ImageT, typename TensorT>
AnyTensorOutputs ForwardService(
        std::shared_ptr<AnyNet> net,
        const ImageT &input,
        std::function<void(const ImageT&, TensorT&)> transform) {
    InputTensorInfo& input_tensor_info = net->getMInputTensorInfoList()[0];
    TensorT transform_tensor;
    transform(input, transform_tensor);
    input_tensor_info.data = transform_tensor.data;      // input tensor only support cv2::Mat

    AnyTensorOutputs outputs;
    net->Forward(outputs);

    return outputs;
}


template <typename ImageT, typename TensorT, typename PreprocessCallbackT>
AnyTensorOutputs ForwardService(
        std::shared_ptr<AnyNet> net,
        const ImageT &input,
        PreprocessCallbackT &callback,
        std::function<void(const ImageT&, TensorT&, PreprocessCallbackT&)> transform) {
    InputTensorInfo& input_tensor_info = net->getMInputTensorInfoList()[0];
    TensorT transform_tensor;
    transform(input, transform_tensor, callback);
    input_tensor_info.data = transform_tensor.data;      // input tensor only support cv2::Mat

    AnyTensorOutputs outputs;
    net->Forward(outputs);

    return outputs;
}



} // namespace

#endif //BIGGUYSMAIN_ANYNET_H
