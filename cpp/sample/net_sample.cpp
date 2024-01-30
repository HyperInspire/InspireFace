//
// Created by tunm on 2023/9/8.
//
#include <iostream>
#include "track_module/face_detect/face_pose.h"
#include "model_index.h"

using namespace inspire;

int main(int argc, char** argv) {
    ModelLoader loader("resource/model_zip/T1");

    Parameter param;
    param.set<std::string>("input_layer", "data");
    param.set<std::vector<std::string>>("outputs_layers", {"ip3_pose", });
    param.set<std::vector<int>>("input_size", {112, 112});
    param.set<std::vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<std::vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<int>("input_channel", 1);        // Input Gray
    param.set<int>("input_image_channel", 1);        // BGR 2 Gray

    auto m_pose_net_ = std::make_shared<FacePose>();
    m_pose_net_->LoadParam(param, loader.ReadModel(ModelIndex::_02_pose_fp16));

    auto image = cv::imread("resource/images/crop.png");

    cv::Mat gray;
    cv::resize(image, gray, cv::Size(112, 112));

    auto res = (*m_pose_net_)(gray);
    LOGD("%f", res[0]);
    LOGD("%f", res[1]);
    LOGD("%f", res[2]);

    return 0;
}