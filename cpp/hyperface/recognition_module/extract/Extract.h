//
// Created by tunm on 2023/9/7.
//
#pragma once
#ifndef HYPERFACEREPO_EXTRACT_H
#define HYPERFACEREPO_EXTRACT_H
#include "data_type.h"
#include "middleware/any_net.h"


namespace hyper {

class HYPER_API Extract: public AnyNet {
public:
    Extract();

    Embedded operator()(const Matrix& bgr_affine);

    Embedded GetFaceFeature(const Matrix& bgr_affine);

};

}   // namespace hyper

#endif //HYPERFACEREPO_EXTRACT_H
