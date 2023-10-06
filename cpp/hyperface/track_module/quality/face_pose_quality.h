//
// Created by Tunm-Air13 on 2023/9/15.
//
#pragma once
#ifndef HYPERFACEREPO_FACEPOSEQUALITY_H
#define HYPERFACEREPO_FACEPOSEQUALITY_H

#include "../../data_type.h"
#include "middleware/any_net.h"

namespace hyper {

struct FacePoseQualityResult {
    float pitch;
    float yaw;
    float roll;
    std::vector<Point2f> lmk;
    std::vector<float> lmk_quality;
};

class HYPER_API FacePoseQuality : public AnyNet {
public:

    FacePoseQuality();

    FacePoseQualityResult operator()(const Matrix& bgr_affine);

    static cv::Mat ComputeCropMatrix(const cv::Rect2f &rect);

public:
    const static int INPUT_WIDTH = 96;
    const static int INPUT_HEIGHT = 96;

};

}   // namespace hyper

#endif //HYPERFACEREPO_FACEPOSEQUALITY_H
