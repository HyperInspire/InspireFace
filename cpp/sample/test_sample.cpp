//
// Created by tunm on 2023/9/7.
//

#include <iostream>
#include "face_context.h"
#include "opencv2/opencv.hpp"
#include "utils/test_helper.h"

using namespace hyper;

int main(int argc, char** argv) {
    FaceContext ctx;
    CustomPipelineParameter param;
    param.enable_liveness = true;
    param.enable_face_quality = true;
    int32_t ret = ctx.Configuration("test_res/model_zip/T1", DetectMode::DETECT_MODE_IMAGE, 1, param);
    if (ret != 0) {
        LOGE("初始化错误");
        return -1;
    }
    auto image = cv::imread("test_res/images/kun.jpg");
    cv::Mat rot90;
    TestUtils::rotate(image, rot90, ROTATION_90);

    CameraStream stream;
    stream.SetDataFormat(BGR);
    stream.SetRotationMode(ROTATION_90);
    stream.SetDataBuffer(rot90.data, rot90.rows, rot90.cols);
    ctx.FaceDetectAndTrack(stream);

    std::vector<HyperFaceData> faces;
    for (int i = 0; i < ctx.GetNumberOfFacesCurrentlyDetected(); ++i) {
//        const ByteArray &byteArray = ctx.GetDetectCache()[i];
        HyperFaceData face = {0};
//        ret = DeserializeHyperFaceData(byteArray, face);

        const FaceBasicData &faceBasic = ctx.GetFaceBasicDataCache()[i];
        ret = DeserializeHyperFaceData((char* )faceBasic.data, faceBasic.dataSize, face);
        LOGD("OK!");

        if (ret != HSUCCEED) {
            return -1;
        }
        faces.push_back(face);

        cv::Rect rect(face.rect.x, face.rect.y, face.rect.width, face.rect.height);
        std::cout << rect << std::endl;
        cv::rectangle(rot90, rect, cv::Scalar(0, 0, 233), 2);

        for (auto &p: face.keyPoints) {
            cv::Point2f point(p.x, p.y);
            cv::circle(rot90, point, 0, cv::Scalar(0, 0, 255), 5);
        }
    }

    cv::imshow("wq", rot90);
    cv::waitKey(0);


    ret = ctx.FacesProcess(stream, faces, param);
    if (ret != HSUCCEED) {
        return -1;
    }

    // view
    int32_t index = 0;
    LOGD("liveness: %f", ctx.GetRgbLivenessResultsCache()[index]);

    return 0;
}