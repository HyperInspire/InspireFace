//
// Created by Tunm-Air13 on 2021/9/22.
//

#ifndef FACEIN_LIB_REFINE_NET_H
#define FACEIN_LIB_REFINE_NET_H

#include "opencv2/opencv.hpp"
//#include "model_loader.h"
#include "middleware/mnn_infer.h"

class RefineNet {
public:
    RefineNet();

//    RefineNet(ml::Model *refine_net);

//    void Reset(ml::Model *refine_net);

    void Reset(const std::string &path);

    float Infer(const cv::Mat &raw_face_crop_bgr);

private:
    std::shared_ptr<ModelInfer> m_infer;
    const int input_width_ = 24;
    const int input_height_ = 24;
};


#endif //FACEIN_LIB_REFINE_NET_H
