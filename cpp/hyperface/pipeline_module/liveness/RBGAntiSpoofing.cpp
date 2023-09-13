//
// Created by Tunm-Air13 on 2023/9/8.
//

#include "RBGAntiSpoofing.h"

namespace hyper {


RBGAntiSpoofing::RBGAntiSpoofing(): AnyNet("RBGAntiSpoofing") {}


float RBGAntiSpoofing::operator()(const Matrix &bgr_affine27) {
    cv::Mat input;
    cv::resize(bgr_affine27, input, cv::Size(m_input_size_, m_input_size_));
    AnyTensorOutputs outputs;
    Forward(input, outputs);

    return outputs[0].second[1];
}


}