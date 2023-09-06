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

        ctx.UpdateStream(stream, true);

        auto const &faces = ctx.trackingFace;
        for (auto const &face: faces) {
            auto rect = face.GetRect();
            cv::rectangle(frame, rect, cv::Scalar(0, 0, 255), 2, 1);
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

        ctx.UpdateStream(stream, true);

        auto const &faces = ctx.trackingFace;
        for (auto const &face : faces) {
            auto rect = face.GetRect();
            cv::rectangle(frame, rect, cv::Scalar(0, 0, 255), 2, 1);
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

    const std::string folder = "resource/model_zip/T1";
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