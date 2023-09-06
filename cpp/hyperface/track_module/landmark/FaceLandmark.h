//
// Created by Tunm-Air13 on 2023/9/6.
//
#pragma once
#ifndef HYPERFACEREPO_FACELANDMARK_H
#define HYPERFACEREPO_FACELANDMARK_H
#include "../../DataType.h"
#include "middleware/AnyNet.h"

namespace hyper {

class HYPER_API FaceLandmark {
public:
    std::vector<cv::Point2f> operator()(const Matrix& bgr_affine);

    explicit FaceLandmark(int input_size);

    const int getInputSize() const;

public:
    const static int LEFT_EYE_CENTER = 55;      // 左眼中心 55
    const static int RIGHT_EYE_CENTER = 105;    // 右眼中心 105
    const static int NOSE_CORNER = 69;          // 鼻尖 69
    const static int MOUTH_LEFT_CORNER = 45;    // 左嘴角 45
    const static int MOUTH_RIGHT_CORNER = 50;   // 右嘴角 50

private:
    const int m_input_size;
};

}   //  namespace hyper

#endif //HYPERFACEREPO_FACELANDMARK_H
