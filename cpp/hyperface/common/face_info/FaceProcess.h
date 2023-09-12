//
// Created by tunm on 2023/9/12.
//
#pragma once
#ifndef HYPERFACEREPO_FACEPROCESS_H
#define HYPERFACEREPO_FACEPROCESS_H

#include "DataType.h"
namespace hyper {

typedef enum MaskInfo {
    UNKNOWN = -1,
    UNMASKED = 0,
    MASKED = 1,
} MaskInfo;

class HYPER_API FaceProcess {
public:
    MaskInfo maskInfo = UNKNOWN;

};

} // namespace hyper


#endif //HYPERFACEREPO_FACEPROCESS_H
