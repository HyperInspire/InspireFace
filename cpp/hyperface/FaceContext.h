//
// Created by Tunm-Air13 on 2023/9/7.
//
#pragma once
#ifndef HYPERFACEREPO_FACECONTEXT_H
#define HYPERFACEREPO_FACECONTEXT_H
#include "track_module/FaceTrack.h"
#include "DataType.h"
#include "pipeline_module/FacePipeline.h"

using namespace std;

namespace hyper {

enum DetectMode {
    DETECT_MODE_IMAGE = 0,      ///< 图像检测模式 即 Always detect
    DETECT_MODE_VIDEO,          ///< 视频检测模式 即 Face track

};



class HYPER_API FaceContext {
public:

    explicit FaceContext();

    /** 初始化 临时方案 */
    int32_t Configuration(const String& model_file_path, DetectMode detect_mode, int32_t max_detect_face, CustomPipelineParameter param);

    int32_t InputUpdateStream(CameraStream &image);

private:

    CustomPipelineParameter m_parameter_;

    int32_t m_max_detect_face_{};        ///< 可检测最大人脸数量

    DetectMode m_detect_mode_;

    bool m_always_detect_;

    std::shared_ptr<FaceTrack> m_face_track_;


};

}   // namespace hyper

#endif //HYPERFACEREPO_FACECONTEXT_H
