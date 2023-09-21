//
// Created by Tunm-Air13 on 2023/9/20.
//
#include "opencv2/opencv.hpp"
#include "hyperface/middleware/model_loader/ModelLoader.h"
#include "hyperface/track_module/face_detect/all.h"
#include "model_index.h"
#include "hyperface/middleware/timer.h"

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
    param.set<int>("data_type", InputTensorInfo::kDataTypeImage);
    param.set<int>("input_tensor_type", InputTensorInfo::kTensorTypeUint8);
    param.set<int>("output_tensor_type", InputTensorInfo::kTensorTypeFp32);
    param.set<bool>("nchw", false);

    auto model = loader.ReadModel(ModelIndex::_00_fdet_160);
    m_face_detector_ = std::make_shared<FaceDetect>(320);
    m_face_detector_->LoadParam(param, model, InferenceHelper::kRknn);

    // Load a image
    cv::Mat image = cv::imread("test_res/images/face_sample.png");

    Timer timer;
    FaceLocList locs = (*m_face_detector_)(image);
    LOGD("耗时: %f", timer.GetCostTimeUpdate());

    LOGD("Faces: %ld", locs.size());

    for (auto &loc: locs) {
        cv::rectangle(image, cv::Point2f(loc.x1, loc.y1), cv::Point2f(loc.x2, loc.y2), cv::Scalar(0, 0, 255), 3);
    }
    cv::imwrite("det.jpg", image);


    return 0;
}