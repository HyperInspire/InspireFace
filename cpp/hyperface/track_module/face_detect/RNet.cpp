//
// Created by Tunm-Air13 on 2023/9/6.
//

#include "RNet.h"

namespace hyper {


float RNet::operator()(const Matrix &bgr_affine) {
    cv::Mat out;
    cv::resize(bgr_affine, out, cv::Size(24, 24));

    AnyTensorOutputs outputs;
    Forward(out, outputs);

    return outputs[0].second[1];
}

RNet::RNet() : AnyNet("RNet") {}


}   //  namespace hyper