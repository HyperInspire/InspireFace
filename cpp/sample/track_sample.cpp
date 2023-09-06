//
// Created by Tunm-Air13 on 2023/9/6.
//

#include <iostream>
#include "middleware/model_loader/ModelLoader.h"
#include "track_module/face_detect/FaceDetect.h"
#include "model_index.h"
#include "track_module/face_detect/SCRFD.h"
#include "track_module/face_detect/RNet.h"
#include "track_module/face_detect/RefineNet.h"

using namespace hyper;

int scrfd() {
    std::shared_ptr<ModelLoader> loader = std::make_shared<ModelLoader>();
    loader->Reset("resource/model_zip/T1");

    Parameter param;
    param.set<int>("model_index", ModelIndex::_0_fdet_160);
    param.set<string>("input_layer", "input.1");
    param.set<vector<string>>("outputs_layers", {"443", "468", "493", "446", "471", "496", "449", "474", "499"});
    param.set<vector<int>>("input_size", {160, 160});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125f, 0.0078125f, 0.0078125f});

    FaceDetect face_detect;
    face_detect.LoadParam(param, loader->ReadModel(ModelIndex::_0_fdet_160));

    cv::Mat image = cv::imread("resource/images/cxk.jpg");

    double time = (double) cv::getTickCount();
    auto out = face_detect(image);
    double _diff =
            (((double) cv::getTickCount() - time) / cv::getTickFrequency()) * 1000;
    LOGD("help cost %f", _diff);

    LOGD("%d", out.size());

//    auto &box = out[0];
//    cv::Rect rect(cv::Point(box.x1, box.y1), cv::Point(box.x2, box.y2));
//    auto crop = image(rect);
//    cv::imwrite("crop.png", crop);


    SCRFD scrfd;
    scrfd.Reload("resource/models/0_fdet_160.mnn", true);
    std::vector<FaceLoc>  res;

//    for (int i = 0; i < out.size(); ++i) {
//        auto &box = out[i];
//        cv::rectangle(image, cv::Point(box.x1, box.y1), cv::Point(box.x2, box.y2), cv::Scalar(255, 0, 200), 2);
//    }
//    cv::imshow("w", image);
//    cv::waitKey(0);

    time = (double) cv::getTickCount();
    scrfd.Detect(image, res);
    _diff =
            (((double) cv::getTickCount() - time) / cv::getTickFrequency()) * 1000;
    LOGD("raw cost %f", _diff);

    return 0;
}


int rnet() {
    std::shared_ptr<ModelLoader> loader = std::make_shared<ModelLoader>();
    loader->Reset("resource/model_zip/T1");

    Parameter param;
    param.set<int>("model_index", ModelIndex::_4_refine_net);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"prob1", "conv5-2"});
    param.set<vector<int>>("input_size", {24, 24});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125f, 0.0078125f, 0.0078125f});
    param.set<bool>("swap_color", true);        // RGB mode

    RNet rnet_;
    rnet_.LoadParam(param, loader->ReadModel(ModelIndex::_4_refine_net));

    cv::Mat image = cv::imread("resource/images/crop.png");

    {
        double time = (double) cv::getTickCount();
        auto p1 = rnet_(image);
        double _diff =
                (((double) cv::getTickCount() - time) / cv::getTickFrequency()) * 1000;
        LOGD("help cost %f", _diff);
        LOGD("p1: %f", p1);

    }
    RefineNet refine_net;
    refine_net.Reset("resource/models/4_refine_net.mnn");

    {
        double time = (double) cv::getTickCount();
        auto p2 = refine_net.Infer(image);
        double _diff =
                (((double) cv::getTickCount() - time) / cv::getTickFrequency()) * 1000;
        LOGD("raw cost %f", _diff);
        LOGD("p2: %f", p2);
    }

    return 0;
}

int main(int argc, char** argv) {
//    scrfd();  // 人脸检测
    rnet();


    return 0;
}