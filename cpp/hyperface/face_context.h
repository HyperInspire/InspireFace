//
// Created by Tunm-Air13 on 2023/9/7.
//
#pragma once
#ifndef HYPERFACEREPO_FACE_CONTEXT_H
#define HYPERFACEREPO_FACE_CONTEXT_H
#include "track_module/face_track.h"
#include "data_type.h"
#include "pipeline_module/face_pipeline.h"
#include "recognition_module/face_recognition.h"
#include <memory>

namespace hyper {

enum DetectMode {
    DETECT_MODE_IMAGE = 0,      ///< 图像检测模式 即 Always detect
    DETECT_MODE_VIDEO,          ///< 视频检测模式 即 Face track

};

typedef struct CustomPipelineParameter {
    bool enable_recognition = false;              ///< 人脸识别功能
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

    int32_t FacesProcess(CameraStream &image, const std::vector<HyperFaceData> &faces, const CustomPipelineParameter& param);

    // 人脸识别相关
    const std::shared_ptr<FaceRecognition>& FaceRecognitionModule();

    // 人脸属性识别相关
    const std::shared_ptr<FacePipeline>& FacePipelineModule();

    const int32_t GetNumberOfFacesCurrentlyDetected() const;

public:
    const std::vector<ByteArray>& GetDetectCache() const;
    const std::vector<FaceBasicData>& GetFaceBasicDataCache() const;
    const std::vector<FaceRect>& GetFaceRectsCache() const;
    const std::vector<float>& GetRollResultsCache() const;
    const std::vector<float>& GetYawResultsCache() const;
    const std::vector<float>& GetPitchResultsCache() const;
    const std::vector<FacePoseQualityResult>& GetQualityResultsCache() const;
    const std::vector<float>& GetMaskResultsCache() const;
    const std::vector<float>& GetRgbLivenessResultsCache() const;

private:

    CustomPipelineParameter m_parameter_;

    int32_t m_max_detect_face_{};        ///< 可检测最大人脸数量

    DetectMode m_detect_mode_;

    bool m_always_detect_{};

    std::shared_ptr<FaceTrack> m_face_track_;

    std::shared_ptr<FaceRecognition> m_face_recognition_;

    std::shared_ptr<FacePipeline> m_face_pipeline_;

private:
    // 缓存数据
    std::vector<ByteArray> m_detect_cache_;
    std::vector<FaceBasicData> m_face_basic_data_cache_;
    std::vector<FaceRect> m_face_rects_cache_;
    std::vector<float> m_roll_results_cache_;
    std::vector<float> m_yaw_results_cache_;
    std::vector<float> m_pitch_results_cache_;
    std::vector<FacePoseQualityResult> m_quality_results_cache_;
    std::vector<float> m_mask_results_cache_;
    std::vector<float> m_rgb_liveness_results_cache_;

};

}   // namespace hyper

#endif //HYPERFACEREPO_FACE_CONTEXT_H
