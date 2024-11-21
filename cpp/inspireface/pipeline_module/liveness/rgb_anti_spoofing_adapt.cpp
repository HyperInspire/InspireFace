/**
 * @author Jingyu Yan
 * @date 2024-10-01
 */

#include "rgb_anti_spoofing_adapt.h"

namespace inspire {

std::vector<float> RBGAntiSpoofingAdapt::Softmax(const std::vector<float>& input) {
    std::vector<float> result;
    float sum = 0.0;

    // Calculate the exponentials and the sum of exponentials
    for (float x : input) {
        float exp_x = std::exp(x);
        result.push_back(exp_x);
        sum += exp_x;
    }

    // Normalize by dividing each element by the sum
    for (float& value : result) {
        value /= sum;
    }

    return result;
}

RBGAntiSpoofingAdapt::RBGAntiSpoofingAdapt(int input_size, bool use_softmax) : AnyNetAdapter("RBGAntiSpoofingAdapt") {
    m_input_size_ = input_size;
    m_softmax_ = use_softmax;
}

float RBGAntiSpoofingAdapt::operator()(const inspirecv::Image& bgr_affine27) {
    AnyTensorOutputs outputs;
    if (bgr_affine27.Width() != m_input_size_ || bgr_affine27.Height() != m_input_size_) {
        auto resized = bgr_affine27.Resize(m_input_size_, m_input_size_);
        Forward(resized, outputs);
    } else {
        Forward(bgr_affine27, outputs);
    }
    if (m_softmax_) {
        auto sm = Softmax(outputs[0].second);
        return sm[1];
    } else {
        return outputs[0].second[1];
    }
}

}  // namespace inspire