//
// Created by tunm on 2023/10/3.
//

#ifndef INSPIREFACE_INTERNAL_H
#define INSPIREFACE_INTERNAL_H

#include "face_session.h"

typedef struct HF_FaceAlgorithmSession {
    inspire::FaceSession impl; ///< Implementation of the face context.
} HF_FaceAlgorithmSession; ///< Handle for managing face context.

typedef struct HF_CameraStream {
    inspirecv::InspireImageProcess impl; ///< Implementation of the camera stream.
} HF_CameraStream; ///< Handle for managing camera stream.

typedef struct HF_ImageBitmap {
    inspirecv::Image impl; ///< Implementation of the image bitmap.
} HF_ImageBitmap; ///< Handle for managing image bitmap.

#endif //INSPIREFACE_INTERNAL_H
