//
// Created by tunm on 2023/9/7.
//
#pragma once
#ifndef HYPERFACEREPO_RECOGNITION_H
#define HYPERFACEREPO_RECOGNITION_H
#include "../../DataType.h"
#include "middleware/AnyNet.h"

namespace hyper {

class HYPER_API Recognition: public AnyNet {
public:
    Recognition();

    Embedded operator()(const Matrix& bgr_affine);

};

}   // namespace hyper

#endif //HYPERFACEREPO_RECOGNITION_H
