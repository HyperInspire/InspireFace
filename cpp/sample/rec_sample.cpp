//
// Created by tunm on 2023/9/10.
//

#include <iostream>
#include "FaceContext.h"
#include "utils/test_helper.h"
#include "hyperface/recognition_module/extract/Alignment.h"
#include "recognition_module/features_block/FeatureBlock.h"

using namespace hyper;

std::string GetFileNameWithoutExtension(const std::string& filePath) {
    // 查找最后一个斜杠或反斜杠的位置
    size_t slashPos = filePath.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        // 从斜杠后面的位置获取文件名
        std::string fileName = filePath.substr(slashPos + 1);

        // 查找最后一个点的位置，即文件后缀的起始位置
        size_t dotPos = fileName.find_last_of('.');
        if (dotPos != std::string::npos) {
            // 返回去掉后缀的文件名部分
            return fileName.substr(0, dotPos);
        } else {
            // 没有后缀的情况下返回整个文件名
            return fileName;
        }
    }

    // 如果没有斜杠，直接查找后缀
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        // 返回去掉后缀的文件名部分
        return filePath.substr(0, dotPos);
    }

    // 如果没有斜杠和后缀，返回原始文件路径
    return filePath;
}

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

    float rec;
    auto ret = FaceRecognition::CosineSimilarity(feature_1, feature_2, rec);
    LOGD("rec: %f", rec);

    return 0;
}


int search(FaceContext &ctx) {

//    std::shared_ptr<FeatureBlock> block;
//    block.reset(FeatureBlock::Create(hyper::MC_OPENCV));

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
        ctx.FaceRecognitionModule()->RegisterFaceFeature(feature, i, GetFileNameWithoutExtension(files_list[i]));
    }

//    ctx.FaceRecognitionModule()->PrintMatrix();

//    auto ret = block->DeleteFeature(3);
//    LOGD("DEL: %d", ret);
//    block->PrintMatrix();

    ctx.FaceRecognitionModule()->DeleteFaceFeature(2);

    LOGD("库内人脸数量: %d", ctx.FaceRecognitionModule()->GetFaceFeatureCount());

    // 修改或插入一个人脸
    {
        Embedded feature;
        auto image = cv::imread("/Users/tunm/Downloads/face_rec/刘奕君/刘奕君3.jpg");
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

//        block->UpdateFeature(4, feature);
//        block->AddFeature(feature);
    }

    // 准备一个图像进行搜索
    {
        Embedded feature;
        auto image = cv::imread("/Users/tunm/Downloads/face_rec/刘奕君/刘奕君2.jpg");
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

        SearchResult result;
        auto timeStart = (double) cv::getTickCount();
        ctx.FaceRecognitionModule()->SearchFaceFeature(feature, result);
        double cost = ((double) cv::getTickCount() - timeStart) / cv::getTickFrequency() * 1000;
        LOGD("搜索耗时: %f", cost);
        LOGD("Top1: %d, %f, %s", result.index, result.score, result.tag.c_str());
    }


    return 0;
}

int main(int argc, char** argv) {
    FaceContext ctx;
    CustomPipelineParameter param;
    param.enable_recognition = true;
    int32_t ret = ctx.Configuration("test_res/model_zip/T1", DetectMode::DETECT_MODE_IMAGE, 1, param);
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