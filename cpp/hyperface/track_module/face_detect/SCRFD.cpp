//
// Created by tunm on 2021/9/19.
//

#include "SCRFD.h"

using namespace std;

namespace hyper {

void SCRFD::Detect(const cv::Mat &bgr, std::vector<FaceLoc> &results) {
    int ori_w = bgr.cols;
    int ori_h = bgr.rows;

    int w, h;
    float scale;
    if (ori_w > ori_h) {
        scale = (float) input_size_ / ori_w;
        w = input_size_;
        h = ori_h * scale;
    } else {
        scale = (float) input_size_ / ori_h;
        h = input_size_;
        w = ori_w * scale;
    }
    int wpad = input_size_ - w;
    int hpad = input_size_ - h;
    cv::Mat resized_img;
    cv::resize(bgr, resized_img, cv::Size(w, h));
    cv::Mat pad;
    cv::copyMakeBorder(resized_img, pad, 0, hpad, 0, wpad, cv::BORDER_CONSTANT, 0.0f);

    interpreter_->resizeTensor(input_, {1, 3, input_size_, input_size_});
    interpreter_->resizeSession(session_);
    std::shared_ptr<MNN::CV::ImageProcess> process(
            MNN::CV::ImageProcess::create(MNN::CV::BGR, MNN::CV::RGB, mean_, 3, normal_, 3));
    process->convert(pad.data, input_size_, input_size_, pad.step[0], input_);

    interpreter_->runSession(session_);

    for (const auto &head_info: heads_info_) {

        MNN::Tensor *kps;

        MNN::Tensor *tensor_scores = interpreter_->getSessionOutput(session_, head_info.cls_layer.c_str());
        MNN::Tensor tensor_scores_host(tensor_scores, tensor_scores->getDimensionType());
        tensor_scores->copyToHostTensor(&tensor_scores_host);

        MNN::Tensor *tensor_boxes = interpreter_->getSessionOutput(session_, head_info.box_layer.c_str());
        MNN::Tensor tensor_boxes_host(tensor_boxes, tensor_boxes->getDimensionType());
        tensor_boxes->copyToHostTensor(&tensor_boxes_host);

        if (use_kps_) {
            MNN::Tensor *tensor_lmk = interpreter_->getSessionOutput(session_, head_info.lmk_layer.c_str());
            MNN::Tensor tensor_lmk_host(tensor_lmk, tensor_lmk->getDimensionType());
            tensor_lmk->copyToHostTensor(&tensor_lmk_host);
            kps = &tensor_lmk_host;
        }

        decode(&tensor_scores_host, &tensor_boxes_host, kps, head_info.stride, results);
    }

    nms(results, nms_threshold_);

    std::sort(results.begin(), results.end(),
              [](FaceLoc a, FaceLoc b) { return (a.y2 - a.y1) * (a.x2 - a.x1) > (b.y2 - b.y1) * (b.x2 - b.x1); });

    for (auto &face: results) {
        face.x1 = face.x1 / scale;
        face.y1 = face.y1 / scale;
        face.x2 = face.x2 / scale;
        face.y2 = face.y2 / scale;
        if (use_kps_) {
            for (int i = 0; i < 5; ++i) {
                face.lmk[i * 2 + 0] = face.lmk[i * 2 + 0] / scale;
                face.lmk[i * 2 + 1] = face.lmk[i * 2 + 1] / scale;
            }
        }
    }


}

SCRFD::SCRFD() {
}

void SCRFD::Reload(const string &path, bool use_kps, int input_seize, int num_anchors, int thread_num) {
    interpreter_ = std::shared_ptr<MNN::Interpreter>(MNN::Interpreter::createFromFile(path.c_str()));
    MNN::ScheduleConfig config;
    config.numThread = thread_num;
    MNN::BackendConfig backendConfig;
    backendConfig.precision = (MNN::BackendConfig::PrecisionMode) 2;
    config.backendConfig = &backendConfig;
    session_ = interpreter_->createSession(config);
    input_ = interpreter_->getSessionInput(session_, input_name_.c_str());
    use_kps_ = use_kps;
    input_size_ = input_seize;
    num_anchors_ = num_anchors;
}
//
//void SCRFD::Reload(ml::Model *model, bool use_kps, int input_seize, int num_anchors, int thread_num) {
//    interpreter_ = std::shared_ptr<MNN::Interpreter>(
//            MNN::Interpreter::createFromBuffer(model->caffemodelBuffer, model->modelsize.caffemodel_size));
//    MNN::ScheduleConfig config;
//    config.numThread = thread_num;
//    MNN::BackendConfig backendConfig;
//    backendConfig.precision = (MNN::BackendConfig::PrecisionMode) 2;
//    config.backendConfig = &backendConfig;
//    session_ = interpreter_->createSession(config);
//    input_ = interpreter_->getSessionInput(session_, input_name_.c_str());
//    use_kps_ = use_kps;
//    input_size_ = input_seize;
//    num_anchors_ = num_anchors;
//}



void SCRFD::generate_anchors(int stride, int input_size, int num_anchors, vector<float> &anchors) {
    int height = ceil(input_size / stride);
    int width = ceil(input_size / stride);
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            for (int k = 0; k < num_anchors; ++k) {
                anchors.push_back(i * stride);
                anchors.push_back(j * stride);
            }
        }
    }
}


void SCRFD::decode(MNN::Tensor *cls_pred, MNN::Tensor *box_pred, MNN::Tensor *lmk_pred, int stride,
                   std::vector<FaceLoc> &results) {
    vector<float> anchors_center;
    generate_anchors(stride, input_size_, num_anchors_, anchors_center);
    const float *scores = cls_pred->host<float>();
    const float *boxes = box_pred->host<float>();
    float *lmk;
    if (use_kps_)
        lmk = lmk_pred->host<float>();

    for (int i = 0; i < anchors_center.size() / 2; ++i) {

        if (scores[i] > prob_threshold_) {
            FaceLoc faceInfo;
            float cx = anchors_center[i * 2 + 0];
            float cy = anchors_center[i * 2 + 1];
            float x1 = cx - boxes[i * 4 + 0] * stride;
            float y1 = cy - boxes[i * 4 + 1] * stride;
            float x2 = cx + boxes[i * 4 + 2] * stride;
            float y2 = cy + boxes[i * 4 + 3] * stride;
            faceInfo.x1 = x1;
            faceInfo.y1 = y1;
            faceInfo.x2 = x2;
            faceInfo.y2 = y2;
            faceInfo.score = scores[i];
            if (use_kps_) {
                for (int j = 0; j < 5; ++j) {
                    float px = cx + lmk[i * 10 + j * 2 + 0] * stride;
                    float py = cy + lmk[i * 10 + j * 2 + 1] * stride;
                    faceInfo.lmk[j * 2 + 0] = px;
                    faceInfo.lmk[j * 2 + 1] = py;
                }
            }
            results.push_back(faceInfo);
        }

    }

}

void SCRFD::nms(vector<FaceLoc> &input_faces, float nms_threshold) {
    std::sort(input_faces.begin(), input_faces.end(), [](FaceLoc a, FaceLoc b) { return a.score > b.score; });
    std::vector<float> area(input_faces.size());
    for (int i = 0; i < int(input_faces.size()); ++i) {
        area[i] =
                (input_faces.at(i).x2 - input_faces.at(i).x1 + 1) *
                (input_faces.at(i).y2 - input_faces.at(i).y1 + 1);
    }
    for (int i = 0; i < int(input_faces.size()); ++i) {
        for (int j = i + 1; j < int(input_faces.size());) {
            float xx1 = (std::max)(input_faces[i].x1, input_faces[j].x1);
            float yy1 = (std::max)(input_faces[i].y1, input_faces[j].y1);
            float xx2 = (std::min)(input_faces[i].x2, input_faces[j].x2);
            float yy2 = (std::min)(input_faces[i].y2, input_faces[j].y2);
            float w = (std::max)(float(0), xx2 - xx1 + 1);
            float h = (std::max)(float(0), yy2 - yy1 + 1);
            float inter = w * h;
            float ovr = inter / (area[i] + area[j] - inter);
            if (ovr >= nms_threshold) {
                input_faces.erase(input_faces.begin() + j);
                area.erase(area.begin() + j);
            } else {
                j++;
            }
        }
    }
}

SCRFD::~SCRFD() {
    interpreter_->releaseModel();
    interpreter_->releaseSession(session_);
}

void SCRFD::load_heads(const std::vector<DetHeadInfo> &heads_info) {
    heads_info_ = heads_info;
}

void SCRFD::Draw(cv::Mat &img, bool use_kps, const vector<FaceLoc> &results) {
    for (auto &item: results) {
        char text[256];
        sprintf(text, "%.1f%%", item.score * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
        int x = item.x1;
        int y = item.y1 - label_size.height - baseLine;
        if (y < 0)
            y = 0;
        if (x + label_size.width > img.cols)
            x = img.cols - label_size.width;
        cv::rectangle(img, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                      cv::Scalar(255, 255, 255), -1);
        cv::rectangle(img, cv::Point(item.x1, item.y1), cv::Point(item.x2, item.y2), cv::Scalar(255, 255, 255), 2);
        cv::putText(img, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 0, 0), 1);
        if (use_kps) {
            for (int i = 0; i < 5; ++i) {
                float x = item.lmk[i * 2 + 0];
                float y = item.lmk[i * 2 + 1];
                cv::line(img, cv::Point(x, y), cv::Point(x, y), cv::Scalar(255, 255, 255), 2);
            }
        }
    }
}

}
