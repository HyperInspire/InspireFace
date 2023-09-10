//
// Created by tunm on 2023/9/7.
//
#pragma once
#ifndef HYPERFACEREPO_EXTRACT_H
#define HYPERFACEREPO_EXTRACT_H
#include "DataType.h"
#include "middleware/AnyNet.h"


namespace hyper {

class HYPER_API Extract: public AnyNet {
public:
    Extract();

    Embedded operator()(const Matrix& bgr_affine);

};

}   // namespace hyper

#endif //HYPERFACEREPO_EXTRACT_H
