//
// Created by Tunm-Air13 on 2023/9/8.
//
#pragma once
#ifndef HYPERFACEREPO_RBGANTISPOOFING_H
#define HYPERFACEREPO_RBGANTISPOOFING_H
#include "data_type.h"
#include "middleware/any_net.h"

namespace inspire {

std::vector<float> softmax(const std::vector<float>& input);

class HYPER_API RBGAntiSpoofing: public AnyNet {
public:

    RBGAntiSpoofing(int input_size = 112, bool use_softmax = false);

    float operator()(const Matrix& bgr_affine27);

private:
    int m_input_size_;

    bool m_softmax_ = false;

};

}   // namespace hyper

#endif //HYPERFACEREPO_RBGANTISPOOFING_H
