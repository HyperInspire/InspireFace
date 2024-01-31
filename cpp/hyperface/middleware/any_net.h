//
// Created by tunm on 2023/5/6.
//
#pragma once
#ifndef BIGGUYSMAIN_ANYNET_H
#define BIGGUYSMAIN_ANYNET_H
#include "../data_type.h"
#include "inference_helper/inference_helper.h"
#include "parameter.h"
#include "model_loader/model_loader.h"
#include "opencv2/opencv.hpp"
#include "../log.h"

namespace inspire {

using AnyTensorOutputs = std::vector<std::pair<std::string, std::vector<float>>>;

/**
 * @class AnyNet
 * @brief Generic neural network class for various inference tasks.
 *
 * This class provides a general interface for different types of neural networks,
 * facilitating loading parameters, initializing models, and executing forward passes.
 */
class HYPER_API AnyNet {
    PARAMETERIZATION_SUPPORT

public:
    /**
     * @brief Constructor for AnyNet.
     * @param name Name of the neural network.
     */
    explicit AnyNet(const std::string &name):m_name_(name) {}

    /**
     * @brief Loads parameters and initializes the model for inference.
     * @param param Parameters for network configuration.
     * @param model Pointer to the model.
     * @param type Type of the inference helper (default: kMnn).
     * @return int32_t Status of the loading and initialization process.
     */
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
        _initSingleParam<int>(param, "data_type", InputTensorInfo::InputTensorInfo::kDataTypeImage);
        _initSingleParam<int>(param, "input_tensor_type", InputTensorInfo::TensorInfo::kTensorTypeFp32);
        _initSingleParam<int>(param, "output_tensor_type", InputTensorInfo::TensorInfo::kTensorTypeFp32);
        _initSingleParam<int>(param, "threads", 1);

        int model_index = getParam<int>("model_index");
        m_nn_inference_.reset(InferenceHelper::Create(type));
        m_nn_inference_->SetNumThreads(getParam<int>("threads"));

        m_output_tensor_info_list_.clear();
        std::vector<std::string> outputs_layers = getParam<std::vector<std::string>>("outputs_layers");
        int tensor_type = getParam<int>("input_tensor_type");
        int out_tensor_type = getParam<int>("output_tensor_type");
        for (auto &name: outputs_layers) {
            m_output_tensor_info_list_.push_back(OutputTensorInfo(name, out_tensor_type));
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
        if (getParam<bool>("nchw")) {
            input_tensor_info.tensor_dims =  { 1, channel, m_input_image_size_.height, m_input_image_size_.width };
        } else {
            input_tensor_info.tensor_dims =  { 1, m_input_image_size_.height, m_input_image_size_.width, channel };
        }

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

    /**
     * @brief Performs a forward pass of the network with given input data.
     * @param data The input matrix (image) to process.
     * @param outputs Outputs of the network (tensor outputs).
     */
    void Forward(const Matrix &data, AnyTensorOutputs& outputs) {
        InputTensorInfo& input_tensor_info = getMInputTensorInfoList()[0];
#ifdef INFERENCE_HELPER_ENABLE_RKNN
        // Start by simply implementing a temporary color shift on the outside
        if (getParam<bool>("swap_color")) {
            cv::cvtColor(data, m_cache_, cv::COLOR_BGR2RGB);
            input_tensor_info.data = m_cache_.data;
        } else {
            input_tensor_info.data = data.data;
        }
#else
        input_tensor_info.data = data.data;
#endif
        Forward(outputs);
    }

    /**
     * @brief Performs a forward pass of the network.
     * @param outputs Outputs of the network (tensor outputs).
     */
    void Forward(AnyTensorOutputs& outputs) {

//        LOGD("ppPreProcess");
        if (m_nn_inference_->PreProcess(m_input_tensor_info_list_) != InferenceHelper::kRetOk) {
            LOGD("PreProcess error");
        }
//        LOGD("PreProcess");
        if (m_nn_inference_->Process(m_output_tensor_info_list_) != InferenceHelper::kRetOk) {
            LOGD("Process error");
        }
//        LOGD("Process");
        for (int i = 0; i < m_output_tensor_info_list_.size(); ++i) {
            std::vector<float> output_score_raw_list(m_output_tensor_info_list_[i].GetDataAsFloat(),
                                                     m_output_tensor_info_list_[i].GetDataAsFloat() +
                                                     m_output_tensor_info_list_[i].GetElementNum());
//            LOGE("m_output_tensor_info_list_[i].GetElementNum(): %d",m_output_tensor_info_list_[i].GetElementNum());
            outputs.push_back(std::make_pair(m_output_tensor_info_list_[i].name, output_score_raw_list));
        }

        m_cache_.release();
    }


public:
    /**
     * @brief Gets a reference to the input tensor information list.
     * @return Reference to the vector of input tensor information.
     */
    std::vector<InputTensorInfo> &getMInputTensorInfoList() {
        return m_input_tensor_info_list_;
    }

    /**
     * @brief Gets a reference to the output tensor information list.
     * @return Reference to the vector of output tensor information.
     */
    std::vector<OutputTensorInfo> &getMOutputTensorInfoList() {
        return m_output_tensor_info_list_;
    }

    /**
     * @brief Gets the size of the input image.
     * @return Size of the input image.
     */
    cv::Size &getMInputImageSize() {
        return m_input_image_size_;
    }

protected:
    std::string m_name_;                                                ///< Name of the neural network.

private:

    std::shared_ptr<InferenceHelper> m_nn_inference_;                   ///< Shared pointer to the inference helper.
    std::vector<InputTensorInfo> m_input_tensor_info_list_;             ///< List of input tensor information.
    std::vector<OutputTensorInfo> m_output_tensor_info_list_;           ///< List of output tensor information.
    cv::Size m_input_image_size_{};                                     ///< Size of the input image.
    cv::Mat m_cache_;                                                   ///< Cached matrix for image data.

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


/**
 * @brief Executes a forward pass through the neural network for a given input, with preprocessing.
 * @tparam ImageT Type of the input image.
 * @tparam TensorT Type of the transformed tensor.
 * @tparam PreprocessCallbackT Type of the preprocessing callback function.
 * @param net Shared pointer to the AnyNet neural network object.
 * @param input The input image to be processed.
 * @param callback Preprocessing callback function to be applied to the input.
 * @param transform Transformation function to convert the input image to a tensor.
 * @return AnyTensorOutputs Outputs of the network (tensor outputs).
 *
 * This template function handles the preprocessing of the input image, transformation to tensor,
 * and then passes it through the neural network to get the output. The function is generic and
 * can work with different types of images and tensors, as specified by the template parameters.
 */
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
