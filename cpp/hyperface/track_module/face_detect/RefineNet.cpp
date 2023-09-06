//
// Created by Tunm-Air13 on 2021/9/22.
//

#include "RefineNet.h"

RefineNet::RefineNet() {}

//void RefineNet::Reset(ml::Model *refine_net) {
//    float mean[] = {127.5f, 127.5f, 127.5f};
//    float normal[] = {0.0078125, 0.0078125, 0.0078125};
//    m_infer = std::make_shared<ModelInfer>(refine_net, 1, mean, normal);
//    m_infer->Init("data", "prob1", input_width_, input_height_);
//}

void RefineNet::Reset(const std::string &path) {
    float mean[] = {127.5f, 127.5f, 127.5f};
    float normal[] = {0.0078125, 0.0078125, 0.0078125};
    m_infer = std::make_shared<ModelInfer>(path, 1, mean, normal);
    m_infer->Init("data", "prob1", input_width_, input_height_);
}

float RefineNet::Infer(const cv::Mat &raw_face_crop_bgr) {
    cv::Mat input_mat;
    cv::resize(raw_face_crop_bgr, input_mat, cv::Size(input_width_, input_height_));
    cv::cvtColor(input_mat, input_mat, cv::COLOR_BGR2RGB);
    std::vector<float> out = m_infer->Infer(input_mat);

    return out[1];
}



//RefineNet::RefineNet(ml::Model *refine_net) {
//    Reset(refine_net);
//}
