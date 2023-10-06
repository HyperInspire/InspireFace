//
// Created by Tunm-Air13 on 2023/9/8.
//
#pragma once
#ifndef HYPERFACEREPO_MASKPREDICT_H
#define HYPERFACEREPO_MASKPREDICT_H
#include "data_type.h"
#include "middleware/any_net.h"

namespace hyper {

class HYPER_API MaskPredict: public AnyNet {
public:
    MaskPredict();

    float operator()(const Matrix& bgr_affine);

private:
    const int m_input_size_ = 96;
};

}   // namespace hyper


#endif //HYPERFACEREPO_MASKPREDICT_H
