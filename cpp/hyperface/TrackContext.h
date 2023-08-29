//
// Created by tunm on 2023/8/29.
//
#pragma once
#ifndef HYPERFACEREPO_TRACKCONTEXT_H
#define HYPERFACEREPO_TRACKCONTEXT_H
#include <iostream>
#include "face_detect/all.h"
#include "landmark/all.h"
#include "face_info/all.h"
#include "camera_stream/camera_stream.h"

using namespace std;

class TrackContext {
public:

    TrackContext();

    int LoadDataFromFolder(const std::string &folder_path);

    void UpdateStream(CameraStream &image, bool is_detect);

private:

    void SparseLandmarkPredict(const cv::Mat &raw_face_crop,
                               std::vector<cv::Point2f> &landmarks_output,
                               float &score, float size);

    bool trackFace(CameraStream &image, FaceObject &face);

    static void BlackingTrackingRegion(cv::Mat &image, cv::Rect &rect_mask);

    void nms(float th = 0.5);

    void detectFace(const cv::Mat &input, float scale);

public:

    std::vector<FaceObject> trackingFace;

private:
    std::vector<FaceObject> candidate_faces_;
    int detection_index_;
    int detection_interval_;
    int tracking_idx_;
    double det_use_time_;
    double track_total_use_time_;

private:

    std::shared_ptr<SCRFD> m_face_detector_;

    std::shared_ptr<Landmark> m_landmark_predictor_;

    std::shared_ptr<RefineNet> m_refine_net_;

};


#endif //HYPERFACEREPO_TRACKCONTEXT_H
