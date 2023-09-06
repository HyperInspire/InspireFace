//
// Created by Tunm-Air13 on 2021/9/11.
//

#define WIDTH 112
#define HEIGHT 112
#include "Landmark.h"


Landmark::Landmark() {
}

void Landmark::Reset(const std::string& model_path) {
    float mean[] = {127.5f, 127.5f, 127.5f};
    float normal[] = {0.0078125, 0.0078125, 0.0078125};
    m_infer = std::make_shared<ModelInfer>(model_path, 1, mean, normal);
    m_infer->Init("input_1", "prelu1/add", WIDTH, HEIGHT);
}

std::vector<cv::Point2f> Landmark::predict(const cv::Mat &crop_bgr) {
    std::vector<float> out = this->infer(crop_bgr);
    std::vector<cv::Point2f> landmarks_output;
    landmarks_output.resize(num_lmk_);
    for (int j = 0; j < num_lmk_; j++) {
        float x = out[j * 2 + 0] * 112;
        float y = out[j * 2 + 1] * 112;
        landmarks_output[j] = cv::Point2f(x, y);
    }
    return landmarks_output;
}

std::vector<float> Landmark::infer(const cv::Mat &crop_bgr) {
    std::vector<float> out = m_infer->Infer(crop_bgr);
    return out;
}

//void Landmark::Reset(ml::Model *model) {
//    float mean[] = {127.5f, 127.5f, 127.5f};
//    float normal[] = {0.0078125, 0.0078125, 0.0078125};
//    m_infer = std::make_shared<ModelInfer>(model, 1, mean, normal);
//    m_infer->Init("input_1", "prelu1/add", WIDTH, HEIGHT);
//}

//Landmark::Landmark(ml::Model *model) {
//    Reset(model);
//}
