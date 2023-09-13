//
// Created by Tunm-Air13 on 2023/9/8.
//
#pragma once
#ifndef HYPERFACEREPO_RBGANTISPOOFING_H
#define HYPERFACEREPO_RBGANTISPOOFING_H
#include "DataType.h"
#include "middleware/AnyNet.h"

namespace hyper {

class HYPER_API RBGAntiSpoofing: public AnyNet {
public:

    RBGAntiSpoofing();

    float operator()(const Matrix& bgr_affine27);

private:
    const int m_input_size_ = 112;

};

}   // namespace hyper

#endif //HYPERFACEREPO_RBGANTISPOOFING_H
