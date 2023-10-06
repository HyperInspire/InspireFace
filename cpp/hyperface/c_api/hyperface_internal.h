//
// Created by tunm on 2023/10/3.
//

#ifndef HYPERFACEREPO_HYPERFACE_INTERNAL_H
#define HYPERFACEREPO_HYPERFACE_INTERNAL_H

#include "face_context.h"

typedef struct HF_FaceContext {
    hyper::FaceContext impl;
} HF_FaceContext;

typedef struct HF_CameraStream {
    CameraStream impl;
} HF_CameraStream;

#endif //HYPERFACEREPO_HYPERFACE_INTERNAL_H
