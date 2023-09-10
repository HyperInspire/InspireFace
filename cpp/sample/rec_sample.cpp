//
// Created by tunm on 2023/9/10.
//

#include <iostream>
#include "FaceContext.h"
#include "utils/test_helper.h"
#include "recognition_module/Alignment.h"

using namespace hyper;

int comparison1v1(FaceContext &ctx) {
    Embedded feature_1;
    Embedded feature_2;

    {
        auto image = cv::imread("/Users/tunm/Downloads/face_rec/刘亦菲/刘亦菲2.jpg");
        cv::Mat rot90;
        TestUtils::rotate(image, rot90, ROTATION_90);

        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_90);
        stream.SetDataBuffer(rot90.data, rot90.rows, rot90.cols);
        ctx.FaceDetectAndTrack(stream);
        const auto &faces = ctx.GetTrackingFaceList();
        if (faces.empty()) {
            LOGD("image1 not face");
            return -1;
        }
        ctx.FaceRecognitionModule()->FaceExtract(stream, faces[0], feature_1);

    }

    {
        auto image = cv::imread("/Users/tunm/Downloads/face_rec/伍佰/伍佰1.jpg");
        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);
        stream.SetDataBuffer(image.data, image.rows, image.cols);
        ctx.FaceDetectAndTrack(stream);
        const auto &faces = ctx.GetTrackingFaceList();
        if (faces.empty()) {
            LOGD("image1 not face");
            return -1;
        }
        ctx.FaceRecognitionModule()->FaceExtract(stream, faces[0], feature_2);

    }

    auto rec = ctx.FaceRecognitionModule()->CosineSimilarity(feature_1, feature_2);
    LOGD("rec: %f", rec);

    return 0;
}


int search(FaceContext &ctx) {
    std::vector<String> files_list = {
            "/Users/tunm/Downloads/face_rec/胡歌/胡歌1.jpg",
            "/Users/tunm/Downloads/face_rec/刘浩存/刘浩存1.jpg",
            "/Users/tunm/Downloads/face_rec/刘亦菲/刘亦菲1.jpg",
            "/Users/tunm/Downloads/face_rec/刘奕君/刘奕君1.jpg",
            "/Users/tunm/Downloads/face_rec/伍佰/伍佰1.jpg",
    };
    for (int i = 0; i < files_list.size(); ++i) {
        auto image = cv::imread(files_list[i]);
        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);
        stream.SetDataBuffer(image.data, image.rows, image.cols);
        ctx.FaceDetectAndTrack(stream);
        const auto &faces = ctx.GetTrackingFaceList();
        if (faces.empty()) {
            LOGD("image1 not face");
            return -1;
        }
        Embedded feature;
        ctx.FaceRecognitionModule()->FaceExtract(stream, faces[0], feature);

        ctx.FaceRecognitionModule()->AddFeature(feature);
    }

    ctx.FaceRecognitionModule()->PrintMatrix();


    // 准备一个图像进行搜索
    {
        Embedded feature;
        auto image = cv::imread("/Users/tunm/Downloads/face_rec/伍佰/伍佰2.jpg");
        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);
        stream.SetDataBuffer(image.data, image.rows, image.cols);
        ctx.FaceDetectAndTrack(stream);
        const auto &faces = ctx.GetTrackingFaceList();
        if (faces.empty()) {
            LOGD("image1 not face");
            return -1;
        }
        ctx.FaceRecognitionModule()->FaceExtract(stream, faces[0], feature);

        float topOneScore;
        int topOneIndex;
        ctx.FaceRecognitionModule()->SearchNearest(feature, topOneIndex, topOneScore);
        LOGD("Top1: %d, %f", topOneIndex, topOneScore);
    }


    return 0;
}

int main(int argc, char** argv) {
    FaceContext ctx;
    CustomPipelineParameter param;
    param.enable_recognition = true;
    int32_t ret = ctx.Configuration("resource/model_zip/T1", DetectMode::DETECT_MODE_IMAGE, 1, param);
    if (ret != 0) {
        LOGE("初始化错误");
        return -1;
    }

    // 1v1对比
//    comparison1v1(ctx);

    // 搜索
    search(ctx);

    return 0;

}