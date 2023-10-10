//
// Created by Tunm-Air13 on 2023/9/6.
//
#pragma once
#ifndef HYPERFACEREPO_RNET_H
#define HYPERFACEREPO_RNET_H
#include "../../data_type.h"
#include "middleware/any_net.h"


namespace hyper {

class HYPER_API RNet: public AnyNet {
public:

    RNet();

    float operator()(const Matrix& bgr_affine);

};

}   //  namespace hyper

#endif //HYPERFACEREPO_RNET_H
