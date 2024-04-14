//
// Created by tunm on 2023/9/10.
//

#include <iostream>
#include "face_context.h"
#include "utils/test_helper.h"
#include "inspireface/recognition_module/extract/alignment.h"
#include "recognition_module/face_feature_extraction.h"
#include "feature_hub/feature_hub.h"

using namespace inspire;

std::string GetFileNameWithoutExtension(const std::string& filePath) {
    size_t slashPos = filePath.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        std::string fileName = filePath.substr(slashPos + 1);

        size_t dotPos = fileName.find_last_of('.');
        if (dotPos != std::string::npos) {
            return fileName.substr(0, dotPos);
        } else {
            return fileName;
        }
    }

    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        return filePath.substr(0, dotPos);
    }

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
    auto ret = FEATURE_HUB->CosineSimilarity(feature_1, feature_2, rec);
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
        FEATURE_HUB->RegisterFaceFeature(feature, i, GetFileNameWithoutExtension(files_list[i]), 1000 + i);
    }

//    ctx.FaceRecognitionModule()->PrintMatrix();

//    auto ret = block->DeleteFeature(3);
//    LOGD("DEL: %d", ret);
//    block->PrintMatrix();

    FEATURE_HUB->DeleteFaceFeature(2);

    LOGD("Number of faces in the library: %d", FEATURE_HUB->GetFaceFeatureCount());

    // Update or insert a face
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

    // Prepare an image to search
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
        FEATURE_HUB->SearchFaceFeature(feature, result);
        double cost = ((double) cv::getTickCount() - timeStart) / cv::getTickFrequency() * 1000;
        LOGD("Search time: %f", cost);
        LOGD("Top1: %d, %f, %s %d", result.index, result.score, result.tag.c_str(), result.customId);
    }


    return 0;
}

int main(int argc, char** argv) {
    FaceContext ctx;
    CustomPipelineParameter param;
    param.enable_recognition = true;
    int32_t ret = ctx.Configuration("test_res/model_zip/Pikachu", DetectMode::DETECT_MODE_IMAGE, 1, param);
    if (ret != 0) {
        LOGE("Initialization error");
        return -1;
    }
    // 1v1对比
    comparison1v1(ctx);

    // 搜索
//    search(ctx);

    return 0;

}