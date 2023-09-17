//
// Created by Tunm-Air13 on 2023/9/7.
//
#pragma once
#ifndef HYPERFACEREPO_FACECONTEXT_H
#define HYPERFACEREPO_FACECONTEXT_H
#include "track_module/FaceTrack.h"
#include "DataType.h"
#include "pipeline_module/FacePipeline.h"
#include "recognition_module/FaceRecognition.h"

using namespace std;

namespace hyper {

enum DetectMode {
    DETECT_MODE_IMAGE = 0,      ///< 图像检测模式 即 Always detect
    DETECT_MODE_VIDEO,          ///< 视频检测模式 即 Face track

};

typedef struct CustomPipelineParameter {
    bool enable_recognition = true;               ///< 人脸识别功能
    bool enable_liveness = false;                 ///< RGB活体检测功能
    bool enable_ir_liveness = false;              ///< IR活体检测功能
    bool enable_mask_detect = false;              ///< 口罩检测功能
    bool enable_age = false;                      ///< 年龄预测功能
    bool enable_gender = false;                   ///< 性别预测功能
    bool enable_face_quality = false;             ///< 人脸质量检测功能
    bool enable_interaction_liveness = false;     ///< 配合活体检测功能

} ContextCustomParameter;

class HYPER_API FaceContext {
public:

    explicit FaceContext();

    /** 初始化 临时方案 */
    int32_t Configuration(const String& model_file_path, DetectMode detect_mode, int32_t max_detect_face, CustomPipelineParameter param);

    int32_t FaceDetectAndTrack(CameraStream &image);

    FaceObjectList& GetTrackingFaceList();

    // 人脸识别相关
    const shared_ptr<FaceRecognition>& FaceRecognitionModule();

    // 人脸属性识别相关
    const shared_ptr<FacePipeline>& FacePipelineModule();

//    int32_t FaceProcess();

private:

    CustomPipelineParameter m_parameter_;

    int32_t m_max_detect_face_{};        ///< 可检测最大人脸数量

    DetectMode m_detect_mode_;

    bool m_always_detect_{};

    std::shared_ptr<FaceTrack> m_face_track_;

    std::shared_ptr<FaceRecognition> m_face_recognition_;

    std::shared_ptr<FacePipeline> m_face_pipeline_;

};

}   // namespace hyper

#endif //HYPERFACEREPO_FACECONTEXT_H
