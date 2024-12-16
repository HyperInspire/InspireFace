/**
 * @author Jingyu Yan
 * @date 2024-10-01
 */

#include "rnet_adapt.h"

namespace inspire {

float RNetAdapt::operator()(const inspirecv::Image &bgr_affine) {
    auto resized = bgr_affine.Resize(24, 24);

    AnyTensorOutputs outputs;
    Forward(resized, outputs);

    return outputs[0].second[1];
}

RNetAdapt::RNetAdapt() : AnyNetAdapter("RNetAdapt") {}

}  //  namespace inspire