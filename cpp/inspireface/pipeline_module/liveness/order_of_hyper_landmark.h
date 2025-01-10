/**
 * Created by Jingyu Yan
 * @date 2024-10-01
 */
#pragma once
#ifndef INSPIREFACE_ORDER_HYPERLANDMARK_H
#define INSPIREFACE_ORDER_HYPERLANDMARK_H
#include <iostream>
#include <vector>

namespace inspire {

// HyperLandmark left eye contour points sequence of dense facial landmarks.
const std::vector<int> HLMK_LEFT_EYE_POINTS_INDEX = {1, 34, 53, 59, 67, 3, 12, 94};

// HyperLandmark right eye contour points sequence of dense facial landmarks.
const std::vector<int> HLMK_RIGHT_EYE_POINTS_INDEX = {27, 104, 41, 85, 20, 47, 43, 51};

}  // namespace inspire

#endif  // INSPIREFACE_ORDER_HYPERLANDMARK_H
