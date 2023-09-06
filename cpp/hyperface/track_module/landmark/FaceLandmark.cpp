//
// Created by Tunm-Air13 on 2023/9/6.
//

#include "FaceLandmark.h"

namespace hyper {


std::vector<float> FaceLandmark::operator()(const Matrix& bgr_affine) {
    AnyTensorOutputs outputs;
    Forward(bgr_affine, outputs);
    const auto &out = outputs[0].second;

    return out;
}

FaceLandmark::FaceLandmark(int input_size):
    AnyNet("FaceLandmark"),
    m_input_size(input_size) {}

 int FaceLandmark::getInputSize() const {
    return m_input_size;
}


}   //  namespace hyper