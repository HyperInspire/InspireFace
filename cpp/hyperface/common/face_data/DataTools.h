//
// Created by tunm on 2023/9/17.
//

#ifndef HYPERFACEREPO_DATATOOLS_H
#define HYPERFACEREPO_DATATOOLS_H
#include "opencv2/opencv.hpp"
#include "FaceDataType.h"
#include "../face_info/FaceObject.h"

namespace hyper {

inline void PrintTransMatrix(const TransMatrix& matrix) {
        std::cout << "Transformation Matrix:" << std::endl;
        std::cout << "m00: " << matrix.m00 << "\t";
        std::cout << "m01: " << matrix.m01 << "\t";
        std::cout << "tx: " << matrix.tx << std::endl;

        std::cout << "m10: " << matrix.m10 << "\t";
        std::cout << "m11: " << matrix.m11 << "\t";
        std::cout << "ty: " << matrix.ty << std::endl;
    }

inline void HYPER_API PrintHyperFaceData(const HyperFaceData& data) {
    std::cout << "Track State: " << data.trackState << std::endl;
    std::cout << "In Group Index: " << data.inGroupIndex << std::endl;
    std::cout << "Track ID: " << data.trackId << std::endl;
    std::cout << "Track Count: " << data.trackCount << std::endl;

    std::cout << "Face Rectangle:" << std::endl;
    std::cout << "x: " << data.rect.x << "\t";
    std::cout << "y: " << data.rect.y << "\t";
    std::cout << "width: " << data.rect.width << "\t";
    std::cout << "height: " << data.rect.height << std::endl;

    PrintTransMatrix(data.trans);

}

inline HyperFaceData HYPER_API FaceObjectToHyperFaceData(const FaceObject& obj, int group_index = -1) {
    HyperFaceData data;
    // Face rect
    data.rect.x = obj.bbox_.x;
    data.rect.y = obj.bbox_.y;
    data.rect.width = obj.bbox_.width;
    data.rect.height = obj.bbox_.height;
    // Trans matrix
    data.trans.m00 = obj.getTransMatrix().at<double>(0, 0);
    data.trans.m01 = obj.getTransMatrix().at<double>(0, 1);
    data.trans.m10 = obj.getTransMatrix().at<double>(1, 0);
    data.trans.m11 = obj.getTransMatrix().at<double>(1, 1);
    data.trans.tx = obj.getTransMatrix().at<double>(0, 2);
    data.trans.ty = obj.getTransMatrix().at<double>(1, 2);
    // KetPoints five
    for (int i = 0; i < obj.keyPointFive.size(); ++i) {
        data.keyPoints[i].x = obj.keyPointFive[i].x;
        data.keyPoints[i].y = obj.keyPointFive[i].y;
    }
    // Basic data
    data.inGroupIndex = group_index;
    data.trackCount = obj.tracking_count_;
    data.trackId = obj.GetTrackingId();
    data.trackState = obj.TrackingState();

    return data;
}

inline cv::Mat HYPER_API TransMatrixToMat(const TransMatrix& trans) {
    cv::Mat mat(2, 3, CV_64F);
    mat.at<double>(0, 0) = trans.m00;
    mat.at<double>(0, 1) = trans.m01;
    mat.at<double>(1, 0) = trans.m10;
    mat.at<double>(1, 1) = trans.m11;
    mat.at<double>(0, 2) = trans.tx;
    mat.at<double>(1, 2) = trans.ty;
    return mat;
}

inline cv::Rect HYPER_API FaceRectToRect(const FaceRect& faceRect) {
    return {faceRect.x, faceRect.y, faceRect.width, faceRect.height};
}

inline cv::Point2f HYPER_API HPointToPoint2f(const HPoint& point) {
    return {point.x, point.y};
}

// 序列化 HyperFaceData 到字节流
std::vector<char> HYPER_API SerializeHyperFaceData(const hyper::HyperFaceData& data) {
    std::vector<char> byteArray;
    byteArray.reserve(sizeof(data));

    // 首先将 HyperFaceData 结构体本身序列化
    const char* dataBytes = reinterpret_cast<const char*>(&data);
    byteArray.insert(byteArray.end(), dataBytes, dataBytes + sizeof(data));

    return byteArray;
}

// 反序列化字节流为 HyperFaceData
hyper::HyperFaceData HYPER_API DeserializeHyperFaceData(const std::vector<char>& byteArray) {
    hyper::HyperFaceData data;

    // 检查字节流大小是否足够
    if (byteArray.size() >= sizeof(data)) {
        // 从字节流中复制数据到 HyperFaceData 结构体
        std::memcpy(&data, byteArray.data(), sizeof(data));
    } else {
        std::cerr << "字节流大小不足以还原 HyperFaceData" << std::endl;
    }

    return data;
}

}   // namespace hyper
#endif //HYPERFACEREPO_DATATOOLS_H
