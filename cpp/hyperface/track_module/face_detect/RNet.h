//
// Created by Tunm-Air13 on 2023/9/6.
//
#pragma once
#ifndef HYPERFACEREPO_RNET_H
#define HYPERFACEREPO_RNET_H
#include "../../DataType.h"
#include "middleware/AnyNet.h"


namespace hyper {

class HYPER_API RNet: public AnyNet {
public:

    float operator()(const Matrix& bgr_affine);

};

}   //  namespace hyper

#endif //HYPERFACEREPO_RNET_H
