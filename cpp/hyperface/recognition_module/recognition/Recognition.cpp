//
// Created by tunm on 2023/9/7.
//

#include "Recognition.h"

namespace hyper {


Embedded Recognition::operator()(const Matrix &bgr_affine) {

    AnyTensorOutputs outputs;
    Forward(bgr_affine, outputs);

    return outputs[0].second;
}


Recognition::Recognition() : AnyNet("Recognition") {}

}   // namespace hyper