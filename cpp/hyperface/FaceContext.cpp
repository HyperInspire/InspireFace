//
// Created by Tunm-Air13 on 2023/9/7.
//

#include "FaceContext.h"
#include "log.h"

namespace hyper {


FaceContext::FaceContext() {

}

int32_t FaceContext::Configuration(const String &model_file_path, DetectMode detect_mode, int32_t max_detect_face,
                                   CustomPipelineParameter param) {
    m_detect_mode_ = detect_mode;
    m_max_detect_face_ = max_detect_face;
    m_parameter_ = param;
    ModelLoader loader(model_file_path);
    if (loader.GetStatusCode() != 0) {
        LOGE("Model loading error.");
        return -1;
    }
    m_face_track_ = std::make_shared<FaceTrack>(m_max_detect_face_);
    m_face_track_->Configuration(loader);
    if (m_detect_mode_ == DetectMode::DETECT_MODE_IMAGE) {
        m_always_detect_ = true;
    }


    return 0;
}

int32_t FaceContext::InputUpdateStream(CameraStream &image) {
    m_face_track_->UpdateStream(image, m_always_detect_);
    auto &trackingFace = m_face_track_->trackingFace;
    for (auto &face: trackingFace) {
        auto const& affine = face.getTransMatrix();
        auto crop = image.GetAffineRGBImage(affine, 112, 112);
        cv::imshow("w", crop);
        cv::waitKey(0);
    }

    return 0;
}

}   // namespace hyper