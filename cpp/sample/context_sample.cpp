//
// Created by tunm on 2023/9/15.
//


#include <iostream>
#include "FaceContext.h"
#include "opencv2/opencv.hpp"
#include "utils/test_helper.h"

using namespace hyper;

int main() {
    FaceContext ctx;
    CustomPipelineParameter param;
    param.enable_liveness = true;
    param.enable_face_quality = true;
    int32_t ret = ctx.Configuration("resource/model_zip/T1", DetectMode::DETECT_MODE_VIDEO, 1, param);
    if (ret != 0) {
        LOGE("初始化错误");
        return -1;
    }
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头." << std::endl;
        return -1;
    }

    cv::namedWindow("Webcam", cv::WINDOW_NORMAL);

    while (true) {
        cv::Mat frame;

        cap >> frame;

        if (frame.empty()) {
            std::cerr << "无法从摄像头获取图像." << std::endl;
            break;
        }

        CameraStream stream;
        stream.SetDataBuffer(frame.data, frame.rows, frame.cols);
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);

        ctx.FaceDetectAndTrack(stream);

//        LOGD("Track Cost: %f", ctx.GetTrackTotalUseTime());

        auto &faces = ctx.GetTrackingFaceList();
        for (auto &face: faces) {
            auto rect = face.GetRect();
            int track_id = face.GetTrackingId();
            int track_count = face.GetTrackingCount();

            cv::rectangle(frame, rect, cv::Scalar(0, 0, 255), 2, 1);

            // 创建要显示的文本
            std::string text = "ID: " + std::to_string(track_id) + " Count: " + std::to_string(track_count);

            // 设置文本显示位置，通常在框的上方
            cv::Point text_position(rect.x, rect.y - 10);

            const auto& pose_and_quality = face.high_result;
            float mean_quality = 0.0f;
            for (int i = 0; i < pose_and_quality.lmk_quality.size(); ++i) {
                mean_quality += pose_and_quality.lmk_quality[i];
            }
            mean_quality /= pose_and_quality.lmk_quality.size();
            mean_quality = 1 - mean_quality;
            std::string pose_text = "pitch: " + std::to_string(pose_and_quality.pitch) + ",Yaw: " + std::to_string(pose_and_quality.yaw) + ",roll:" +std::to_string(pose_and_quality.roll) + ", q: " +
                    to_string(mean_quality);

            // 设置文本显示位置，通常在框的下方
            cv::Point pose_position(rect.x, rect.y + rect.height + 20);


            // 设置文本字体和大小
            int font_face = cv::FONT_HERSHEY_SIMPLEX;
            double font_scale = 0.5;
            int font_thickness = 1;
            cv::Scalar font_color(255, 255, 255); // 文本颜色，白色

            // 在图像上绘制文本
            cv::putText(frame, text, text_position, font_face, font_scale, font_color, font_thickness);
            cv::putText(frame, pose_text, pose_position, font_face, font_scale, font_color, font_thickness);
        }


        cv::imshow("Webcam", frame);

        if (cv::waitKey(1) == 27) {
            break;
        }
    }
    cap.release();

    cv::destroyAllWindows();

    return 0;
}