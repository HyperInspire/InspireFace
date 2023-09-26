//
// Created by tunm on 2023/8/29.
//
#include <iostream>
#include "hyperface/track_module/FaceTrack.h"
#include "opencv2/opencv.hpp"

using namespace hyper;

int video_test(FaceTrack &ctx, int cam_id) {
    cv::VideoCapture cap(cam_id);

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

        ctx.UpdateStream(stream, false);

        LOGD("Track Cost: %f", ctx.GetTrackTotalUseTime());

        auto const &faces = ctx.trackingFace;
        for (auto const &face: faces) {
            auto rect = face.GetRect();
            int track_id = face.GetTrackingId();
            int track_count = face.GetTrackingCount();

            cv::rectangle(frame, rect, cv::Scalar(0, 0, 255), 2, 1);

            // 创建要显示的文本
            std::string text = "ID: " + std::to_string(track_id) + " Count: " + std::to_string(track_count);

            // 设置文本显示位置，通常在框的上方
            cv::Point text_position(rect.x, rect.y - 10);

            const auto& euler = face.getPoseEulerAngle();
            std::string pose_text = "P: " + std::to_string(euler[0]) + ",Yaw: " + std::to_string(euler[1]) + ",roll:" +std::to_string(euler[2]);

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

void video_file_test(FaceTrack& ctx, const std::string& video_filename) {
    cv::VideoCapture cap(video_filename);

    if (!cap.isOpened()) {
        std::cerr << "无法打开视频文件: " << video_filename << std::endl;
        return;
    }

    cv::namedWindow("Video", cv::WINDOW_NORMAL);

    while (true) {
        cv::Mat frame;
        cap >> frame;

        if (frame.empty()) {
            std::cerr << "无法从视频文件获取帧." << std::endl;
            break;
        }

        CameraStream stream;
        stream.SetDataBuffer(frame.data, frame.rows, frame.cols);
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);

        ctx.UpdateStream(stream, false);
        LOGD("Track Cost: %f", ctx.GetTrackTotalUseTime());

        auto const &faces = ctx.trackingFace;
        for (auto const &face: faces) {
            auto rect = face.GetRect();
            int track_id = face.GetTrackingId();
            int track_count = face.GetTrackingCount();

            cv::rectangle(frame, rect, cv::Scalar(0, 0, 255), 2, 1);

            auto lmk = face.GetLanmdark();
            for (auto & p : lmk) {
                cv::circle(frame, p, 0, cv::Scalar(0, 0, 242), 2);
            }

            // 创建要显示的文本
            std::string text = "ID: " + std::to_string(track_id) + " Count: " + std::to_string(track_count);

            // 设置文本显示位置，通常在框的上方
            cv::Point text_position(rect.x, rect.y - 10);

            const auto& euler = face.high_result;
            std::string pose_text = "pitch: " + std::to_string(euler.pitch) + ",Yaw: " + std::to_string(euler.yaw) + ",roll:" +std::to_string(euler.roll);

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

        cv::imshow("Video", frame);

        if (cv::waitKey(1) == 27) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <source> <input>" << std::endl;
        return 1;
    }

    const std::string source = argv[1];
    const std::string input = argv[2];

    const std::string folder = "test_res/model_zip/T1";
    LOGD("%s", folder.c_str());
    ModelLoader loader;
    loader.Reset(folder);

    FaceTrack ctx;
    ctx.Configuration(loader);

    if (source == "webcam") {
        int cam_id = std::stoi(input);
        video_test(ctx, cam_id);
    } else if (source == "image") {
        cv::Mat image = cv::imread(input);
        if (!image.empty()) {
//            image_test(ctx, image);
        } else {
            std::cerr << "无法打开图像文件." << std::endl;
        }
    } else if (source == "video") {
        video_file_test(ctx, input);
    } else {
        std::cerr << "无效的输入源: " << source << std::endl;
        return 1;
    }

    return 0;
}