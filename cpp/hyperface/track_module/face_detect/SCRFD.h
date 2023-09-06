//
// Created by tunm on 2021/9/19.
//

#ifndef MNN_SCRFD_SCRFD_H
#define MNN_SCRFD_SCRFD_H

#include "opencv2/opencv.hpp"
#include "MNN/Interpreter.hpp"
#include "MNN/Tensor.hpp"
#include "MNN/ImageProcess.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
//#include "model_loader.h"
#include "DataType.h"


namespace hyper {

typedef struct DetHeadInfo {
    std::string cls_layer;
    std::string box_layer;
    std::string lmk_layer;
    int stride;
} DetHeadInfo;

class SCRFD {
public:
    SCRFD();

    void
    Reload(const std::string &path, bool use_kps, int input_seize = 160, int num_anchors = 2, int thread_num = 1);

//    void Reload(ml::Model *model, bool use_kps, int input_seize = 160, int num_anchors = 2, int thread_num = 1);

    void Detect(const cv::Mat &bgr, std::vector<FaceLoc> &results);

    ~SCRFD();


    static void Draw(cv::Mat &img, bool use_kps, const std::vector<FaceLoc> &results);

private:

    static void generate_anchors(int stride, int input_size, int num_anchors, std::vector<float> &anchors);

    void decode(MNN::Tensor *cls_pred, MNN::Tensor *box_pred, MNN::Tensor *lmk_pred, int stride,
                std::vector<FaceLoc> &results);

    void nms(std::vector<FaceLoc> &input_faces, float nms_threshold);

    void load_heads(const std::vector<DetHeadInfo> &heads_info);


private:
    std::shared_ptr<MNN::Interpreter> interpreter_;
    MNN::Tensor *input_ = nullptr;
    MNN::Session *session_ = nullptr;
    int input_size_;
    int num_anchors_;
    float prob_threshold_ = 0.5f;
    float nms_threshold_ = 0.4f;
    bool use_kps_ = false;

    float mean_[3] = {127.5f, 127.5f, 127.5f};
    float normal_[3] = {0.0078125, 0.0078125, 0.0078125};

    std::string input_name_ = "input.1";
    std::vector<DetHeadInfo> heads_info_ = {
            {"443", "446", "449", 8},
            {"468", "471", "474", 16},
            {"493", "496", "499", 32},
    };
};

}

#endif //MNN_SCRFD_SCRFD_H
