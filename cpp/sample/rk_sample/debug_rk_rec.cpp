//
// Created by tunm on 2023/9/23.
//

#include "opencv2/opencv.hpp"
#include "hyperface/middleware/model_loader/ModelLoader.h"
#include "model_index.h"
#include "hyperface/middleware/Timer.h"
#include "middleware/inference_helper/customized/rknn_adapter.h"
#include "hyperface/recognition_module/simd.h"
#include <memory>
#include "hyperface/recognition_module/extract/Extract.h"

using namespace hyper;

int main() {
    std::vector<std::string> names = {
            "test_res/images/test_data/0.jpg",
            "test_res/images/test_data/1.jpg",
            "test_res/images/test_data/2.jpg",
    };
    ModelLoader loader("test_res/model_zip/test_zip_rec");
    {

        auto net = make_shared<RKNNAdapter>();
        net->Initialize(loader.ReadModel(0));
        net->setOutputsWantFloat(1);



        EmbeddedList list;
        for (int i = 0; i < names.size(); ++i) {
            cv::Mat image = cv::imread(names[i]);
            cv::Mat rgb;
            cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
            net->SetInputData(0, rgb);
            net->RunModel();

            auto out = net->GetOutputData(0);
            auto dims = net->GetOutputTensorSize(0);
//        for (int i = 0; i < dims.size(); ++i) {
//            LOGD("%lu", dims[i]);
//        }
//
        for (int i = 0; i < 512; ++i) {
            std::cout << out[i] << ", ";
        }
        std::cout << std::endl;

            Embedded emb;
            for (int j = 0; j < 512; ++j) {
                emb.push_back(out[j]);
            }
            list.push_back(emb);

        }

        for (int i = 0; i < list.size(); ++i) {
            auto &embedded = list[i];
            float mse = 0.0f;
            for (const auto &one: embedded) {
                mse += one * one;
            }
            mse = sqrt(mse);
            for (float &one : embedded) {
                one /= mse;
            }
        }

        auto cos = simd_dot(list[0].data(), list[1].data(), 512);
        LOGD("COS: %f", cos);
    }

    {
        shared_ptr<Extract> m_extract_;

        Parameter param;
        param.set<int>("model_index", 0);
        param.set<string>("input_layer", "input");
        param.set<vector<string>>("outputs_layers", {"267", });
        param.set<vector<int>>("input_size", {112, 112});
        param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
        param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
        param.set<int>("data_type", InputTensorInfo::kDataTypeImage);
        param.set<int>("input_tensor_type", InputTensorInfo::kTensorTypeUint8);
        param.set<int>("output_tensor_type", InputTensorInfo::kTensorTypeFp32);
        param.set<bool>("nchw", false);
        param.set<bool>("swap_color", true);        // RK需要rgb输入

        m_extract_ = make_shared<Extract>();
        auto model = loader.ReadModel(0);
        m_extract_->LoadParam(param, model, InferenceHelper::kRknn);

        cv::Mat image = cv::imread(names[0]);
//        cv::Mat rgb;
//        cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
        auto feat = m_extract_->GetFaceFeature(image);
        for (int i = 0; i < 512; ++i) {
            std::cout << feat[i] << ", ";
        }
        std::cout << std::endl;
    }


    LOGD("End");

    return 0;
}