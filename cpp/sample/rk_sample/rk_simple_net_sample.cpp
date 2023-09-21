//
// Created by Tunm-Air13 on 2023/9/21.
//

#include "opencv2/opencv.hpp"
#include "hyperface/middleware/model_loader/ModelLoader.h"
#include "hyperface/track_module/face_detect/all.h"
#include "hyperface/pipeline_module/attribute/MaskPredict.h"
#include "model_index.h"
#include "hyperface/middleware/Timer.h"
#include "hyperface/track_module/quality/FacePoseQuality.h"
#include "hyperface/track_module/landmark/FaceLandmark.h"
#include "hyperface/pipeline_module/liveness/RBGAntiSpoofing.h"

using namespace hyper;

ModelLoader loader;


void test_rnet() {
    shared_ptr<RNet> m_rnet_;
    Parameter param;
    param.set<int>("model_index", ModelIndex::_04_refine_net);
    param.set<string>("input_layer", "input_1");
    param.set<vector<string>>("outputs_layers", {"conv5-1/Softmax", "conv5-2/BiasAdd"});
    param.set<vector<int>>("input_size", {24, 24});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<bool>("swap_color", true);        // RGB mode
    param.set<int>("data_type", InputTensorInfo::kDataTypeImage);
    param.set<int>("input_tensor_type", InputTensorInfo::kTensorTypeUint8);
    param.set<int>("output_tensor_type", InputTensorInfo::kTensorTypeFp32);
    param.set<bool>("nchw", false);

    auto model = loader.ReadModel(ModelIndex::_04_refine_net);
    m_rnet_ = std::make_shared<RNet>();
    m_rnet_->LoadParam(param, model, InferenceHelper::kRknn);

    {
        // Load a image
        cv::Mat image = cv::imread("test_res/images/test_data/hasface.jpg");

        Timer timer;
        auto score = (*m_rnet_)(image);
        LOGD("耗时: %f", timer.GetCostTimeUpdate());
        LOGD("has face: %f", score);
    }

    {
        // Load a image
        cv::Mat image = cv::imread("test_res/images/test_data/noface.jpg");

        Timer timer;
        auto score = (*m_rnet_)(image);
        LOGD("耗时: %f", timer.GetCostTimeUpdate());
        LOGD("non face: %f", score);
    }

}

void test_mask() {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_05_mask);
    param.set<string>("input_layer", "input_1");
    param.set<vector<string>>("outputs_layers", {"activation_1/Softmax",});
    param.set<vector<int>>("input_size", {96, 96});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<bool>("swap_color", true);        // RGB mode
    param.set<int>("data_type", InputTensorInfo::kDataTypeImage);
    param.set<int>("input_tensor_type", InputTensorInfo::kTensorTypeUint8);
    param.set<int>("output_tensor_type", InputTensorInfo::kTensorTypeFp32);
    param.set<bool>("nchw", false);

    shared_ptr<MaskPredict> m_mask_predict_;
    m_mask_predict_ = make_shared<MaskPredict>();
    auto model = loader.ReadModel(ModelIndex::_05_mask);
    m_mask_predict_->LoadParam(param, model, InferenceHelper::kRknn);

    {
        // Load a image
        cv::Mat image = cv::imread("test_res/images/test_data/mask.jpg");

        Timer timer;
        auto score = (*m_mask_predict_)(image);
        LOGD("耗时: %f", timer.GetCostTimeUpdate());
        LOGD("mask: %f", score);
    }

    {
        // Load a image
        cv::Mat image = cv::imread("test_res/images/test_data/nomask.jpg");

        Timer timer;
        auto score = (*m_mask_predict_)(image);
        LOGD("耗时: %f", timer.GetCostTimeUpdate());
        LOGD("maskless: %f", score);
    }

}

void test_quality() {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_07_pose_q_fp16);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"fc1", });
    param.set<vector<int>>("input_size", {96, 96});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<bool>("swap_color", true);        // RGB mode
    param.set<int>("data_type", InputTensorInfo::kDataTypeImage);
    param.set<int>("input_tensor_type", InputTensorInfo::kTensorTypeUint8);
    param.set<int>("output_tensor_type", InputTensorInfo::kTensorTypeFp32);
    param.set<bool>("nchw", false);
    shared_ptr<FacePoseQuality> m_face_quality_;
    m_face_quality_ = make_shared<FacePoseQuality>();
    auto model = loader.ReadModel(ModelIndex::_07_pose_q_fp16);
    m_face_quality_->LoadParam(param, model, InferenceHelper::kRknn);

    {
        std::vector<std::string> names = {
                "test_res/images/test_data/p3.jpg",
//                "test_res/images/test_data/p1.jpg",
        };
        for (int i = 0; i < names.size(); ++i) {
            LOGD("Image: %s", names[i].c_str());
            cv::Mat image = cv::imread(names[i]);

            Timer timer;
            auto pose_res = (*m_face_quality_)(image);
            LOGD("耗时: %f", timer.GetCostTimeUpdate());

            for (auto &p: pose_res.lmk) {
                cv::circle(image, p, 0, cv::Scalar(0, 0, 255), 2);
            }
            cv::imwrite("pose.jpg", image);
            LOGD("pitch: %f", pose_res.pitch);
            LOGD("yam: %f", pose_res.yaw);
            LOGD("roll: %f", pose_res.roll);

            for (auto q: pose_res.lmk_quality) {
                std::cout << q << ", ";
            }
            std::cout << std::endl;
        }

    }

}


void test_landmark_mnn() {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_01_lmk);
    param.set<string>("input_layer", "input_1");
    param.set<vector<string>>("outputs_layers", {"prelu1/add", });
    param.set<vector<int>>("input_size", {112, 112});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125f, 0.0078125f, 0.0078125f});

    shared_ptr<FaceLandmark> m_landmark_predictor_;
    m_landmark_predictor_ = std::make_shared<FaceLandmark>(112);
    auto model = loader.ReadModel(ModelIndex::_01_lmk);
    m_landmark_predictor_->LoadParam(param, model);

    cv::Mat image = cv::imread("test_res/images/test_data/crop.png");
    cv::resize(image, image, cv::Size(112, 112));

    vector<float> lmk;
    Timer timer;
    for (int i = 0; i < 50; ++i) {
        lmk = (*m_landmark_predictor_)(image);
        LOGD("耗时: %f", timer.GetCostTimeUpdate());
    }

    for (int i = 0; i < FaceLandmark::NUM_OF_LANDMARK; ++i) {
        float x = lmk[i * 2 + 0] * 112;
        float y = lmk[i * 2 + 1] * 112;
        cv::circle(image, cv::Point2f(x, y), 0, cv::Scalar(0, 0, 255), 1);
    }

    cv::imwrite("lmk.jpg", image);


}



void test_landmark() {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_01_lmk);
    param.set<string>("input_layer", "input_1");
    param.set<vector<string>>("outputs_layers", {"prelu1/add", });
    param.set<vector<int>>("input_size", {112, 112});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<int>("data_type", InputTensorInfo::kDataTypeImage);
    param.set<int>("input_tensor_type", InputTensorInfo::kTensorTypeUint8);
    param.set<int>("output_tensor_type", InputTensorInfo::kTensorTypeFp32);
    param.set<bool>("nchw", false);

    shared_ptr<FaceLandmark> m_landmark_predictor_;
    m_landmark_predictor_ = std::make_shared<FaceLandmark>(112);
    auto model = loader.ReadModel(ModelIndex::_01_lmk);
    m_landmark_predictor_->LoadParam(param, model, InferenceHelper::kRknn);

    cv::Mat image = cv::imread("test_res/images/test_data/0.jpg");
    cv::resize(image, image, cv::Size(112, 112));

    vector<float> lmk;
    Timer timer;
    for (int i = 0; i < 50; ++i) {
        lmk = (*m_landmark_predictor_)(image);
        LOGD("耗时: %f", timer.GetCostTimeUpdate());
    }

    for (int i = 0; i < FaceLandmark::NUM_OF_LANDMARK; ++i) {
        float x = lmk[i * 2 + 0] * 112;
        float y = lmk[i * 2 + 1] * 112;
        cv::circle(image, cv::Point2f(x, y), 0, cv::Scalar(0, 0, 255), 1);
    }

    cv::imwrite("lmk.jpg", image);


}


void test_liveness() {

    Parameter param;
    param.set<int>("model_index", ModelIndex::_06_msafa27);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"556",});
    param.set<vector<int>>("input_size", {80, 80});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<bool>("swap_color", true);        // RGB mode
    param.set<int>("data_type", InputTensorInfo::kDataTypeImage);
    param.set<int>("input_tensor_type", InputTensorInfo::kTensorTypeUint8);
    param.set<int>("output_tensor_type", InputTensorInfo::kTensorTypeFp32);
    param.set<bool>("nchw", false);

    shared_ptr<RBGAntiSpoofing> m_rgb_anti_spoofing_;
    auto model = loader.ReadModel(ModelIndex::_06_msafa27);
    m_rgb_anti_spoofing_ = make_shared<RBGAntiSpoofing>(80, true);
    m_rgb_anti_spoofing_->LoadParam(param, model, InferenceHelper::kRknn);

    std::vector<std::string> names = {
            "test_res/images/test_data/real.jpg",
            "test_res/images/test_data/fake.jpg",
    };

    for (int i = 0; i < names.size(); ++i) {
        auto image = cv::imread(names[i]);
        Timer timer;
        auto score = (*m_rgb_anti_spoofing_)(image);
        LOGD("耗时: %f", timer.GetCostTimeUpdate());
        LOGD("%s : %f", names[i].c_str(), score);
    }

}


int main() {
    loader.Reset("test_res/model_zip/T1_rv1109rv1126");

//    test_rnet();

//    test_mask();

//    test_quality();

//    test_landmark_mnn();

//    test_landmark();

    test_liveness();

    return 0;
}