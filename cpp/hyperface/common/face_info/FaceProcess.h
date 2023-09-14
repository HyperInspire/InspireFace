//
// Created by tunm on 2023/9/12.
//
#pragma once
#ifndef HYPERFACEREPO_FACEPROCESS_H
#define HYPERFACEREPO_FACEPROCESS_H

#include "DataType.h"
namespace hyper {

typedef enum MaskInfo {
    UNKNOWN_MASK = -1,
    UNMASKED = 0,
    MASKED = 1,
} MaskInfo;

typedef enum RGBLivenessInfo {
    UNKNOWN_RGB_LIVENESS = -1,
    LIVENESS_FAKE = 0,
    LIVENESS_REAL = 1,
} RGBLivenessInfo;

class HYPER_API FaceProcess {
public:
    MaskInfo maskInfo = UNKNOWN_MASK;
    RGBLivenessInfo rgbLivenessInfo = UNKNOWN_RGB_LIVENESS;

};

} // namespace hyper


#endif //HYPERFACEREPO_FACEPROCESS_H
