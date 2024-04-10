//
// Created by tunm on 2023/10/3.
//

#ifndef HYPERFACEREPO_INSPIREFACE_INTERNAL_H
#define HYPERFACEREPO_INSPIREFACE_INTERNAL_H

#include "face_context.h"

typedef struct HF_FaceContext {
    inspire::FaceContext impl; ///< Implementation of the face context.
} HF_FaceContext; ///< Handle for managing face context.

typedef struct HF_CameraStream {
    inspire::CameraStream impl; ///< Implementation of the camera stream.
} HF_CameraStream; ///< Handle for managing camera stream.


#endif //HYPERFACEREPO_INSPIREFACE_INTERNAL_H
