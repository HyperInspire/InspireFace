//
// Created by Tunm-Air13 on 2023/9/20.
//
#include "opencv2/opencv.hpp"
#include "hyperface/middleware/model_loader/ModelLoader.h"
#include "hyperface/track_module/face_detect/all.h"
#include "model_index.h"

using namespace hyper;

int main() {
    ModelLoader loader("test_res/model_zip/T1_rv1109rv1126");
    shared_ptr<FaceDetect> m_face_detector_;
    Parameter param;
    param.set<int>("model_index", ModelIndex::_00_fdet_160);
    param.set<string>("input_layer", "input.1");
    param.set<vector<string>>("outputs_layers", {"output", "output1", "output2", "output3", "output4", "output5", "output6", "output7", "output8"});
    param.set<vector<int>>("input_size", {320, 320});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<int>("data_type", InputTensorInfo::kTensorTypeUint8);
    param.set<int>("tensor_type", InputTensorInfo::kTensorTypeUint8);

    auto model = loader.ReadModel(ModelIndex::_00_fdet_160);
    m_face_detector_ = std::make_shared<FaceDetect>(320);
    m_face_detector_->LoadParam(param, model, InferenceHelper::kRknn);



    return 0;
}