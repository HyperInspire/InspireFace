//
// Created by tunm on 2023/8/29.
//
#include <iostream>
#include "hyperface/TrackContext.h"
#include "opencv2/opencv.hpp"


int video_test(TrackContext& ctx, int cam_id) {
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

int main() {
    const std::string folder = "resource/models/";
    cv::Mat image = cv::imread("resource/images/cxk.jpg");

    TrackContext ctx;
    ctx.LoadDataFromFolder(folder);


    video_test(ctx, 0);

    return 0;
}