//
// Created by Tunm-Air13 on 2023/9/15.
//
#pragma once
#ifndef HYPERFACEREPO_FACEPOSEQUALITY_H
#define HYPERFACEREPO_FACEPOSEQUALITY_H

#include "../../DataType.h"
#include "middleware/AnyNet.h"

namespace hyper {

struct FacePoseQualityResult {
    float pitch;
    float yaw;
    float roll;
    Point2f lmk[5];
    float lmk_quality[5];
};

class HYPER_API FacePoseQuality : public AnyNet {
public:

    std::vector<float> operator()(const Matrix& bgr_affine);

    static cv::Mat ComputeCropMatrix(const cv::Rect2f &rect);

public:
    const static int INPUT_WIDTH = 96;
    const static int INPUT_HEIGHT = 96;

};

}   // namespace hyper

#endif //HYPERFACEREPO_FACEPOSEQUALITY_H
