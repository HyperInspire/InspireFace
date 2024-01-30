//
// Created by tunm on 2023/8/29.
//
#pragma once
#ifndef HYPERFACEREPO_FACETRACK_H
#define HYPERFACEREPO_FACETRACK_H
#include <iostream>
#include "face_detect/all.h"
#include "landmark/face_landmark.h"
#include "common/face_info/all.h"
#include "middleware/camera_stream/camera_stream.h"
#include "quality/face_pose_quality.h"

namespace inspire {

class HYPER_API FaceTrack {
public:

    explicit FaceTrack(int max_detected_faces = 1, int detection_interval = 20);

    int Configuration(ModelLoader &loader);

    void UpdateStream(CameraStream &image, bool is_detect);

private:


    void SparseLandmarkPredict(const cv::Mat &raw_face_crop,
                               std::vector<cv::Point2f> &landmarks_output,
                               float &score, float size = 112.0);

    bool TrackFace(CameraStream &image, FaceObject &face);

    static void BlackingTrackingRegion(cv::Mat &image, cv::Rect &rect_mask);

    void nms(float th = 0.5);

    void DetectFace(const cv::Mat &input, float scale);

    int InitLandmarkModel(Model* model);
    int InitDetectModel(Model* model);
    int InitRNetModel(Model* model);
    int InitFacePoseModel(Model* model);

public:
    double GetTrackTotalUseTime() const;

public:

    std::vector<FaceObject> trackingFace;

private:
    std::vector<FaceObject> candidate_faces_;
    int detection_index_;
    int detection_interval_;
    int tracking_idx_ = 0;
    double det_use_time_;
    double track_total_use_time_;
    const int max_detected_faces_;

private:

    std::shared_ptr<FaceDetect> m_face_detector_;

    std::shared_ptr<FaceLandmark> m_landmark_predictor_;

    std::shared_ptr<RNet> m_refine_net_;

    std::shared_ptr<FacePoseQuality> m_face_quality_;

};

}   // namespace hyper

#endif //HYPERFACEREPO_FACETRACK_H
