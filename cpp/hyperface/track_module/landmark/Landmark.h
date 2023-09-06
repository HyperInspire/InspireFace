//
// Created by Tunm-Air13 on 2021/9/11.
//

#ifndef FACE_DEV_SCRFD_LANDMARK_H
#define FACE_DEV_SCRFD_LANDMARK_H

#include "middleware/mnn_infer.h"
#include "opencv2/opencv.hpp"
//#include "model_loader.h"


const static int left_eye_center_ = 55;    //左眼中心 55
const static int right_eye_center_ = 105;    //右眼中心 105
const static int nose_corner_ = 69;         //鼻尖 69
const static int mouth_left_corner_ = 45;    //左嘴角 45
const static int mouth_right_corner_ = 50;  //右嘴角 50

class Landmark {
public:
    explicit Landmark();

//    Landmark(ml::Model *model);

    void Reset(const std::string& model_path);

//    void Reset(ml::Model *model);


    std::vector<cv::Point2f> predict(const cv::Mat &crop_bgr);

    std::vector<float> infer(const cv::Mat &crop_bgr);

    int LandmarkNum() const {
        return num_lmk_;
    }

private:
    std::shared_ptr<ModelInfer> m_infer;
    int num_lmk_ = 106;
};


#endif //FACE_DEV_SCRFD_LANDMARK_H
