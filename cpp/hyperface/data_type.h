//
// Created by tunm on 2023/5/5.
//
#pragma once
#ifndef HYPERFACE_DATATYPE_H
#define HYPERFACE_DATATYPE_H

#include <cstdint>
#if defined(_WIN32) && (defined(_DEBUG) || defined(DEBUG))
#define _CRTDBG_MAP_ALLOC
#include "crtdbg.h"
#endif

#ifndef HYPER_API
#define HYPER_API
#endif

#include <opencv2/opencv.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

namespace hyper {

/**
 * @ingroup DataType
 * @{
 */

#if !defined(int64)
/** 64位整型定义 */
typedef int64_t int64;
#endif

#if !defined(uint64)
/** 64位无符号整型定义 */
typedef uint64_t uint64;
#endif

#if !defined(int32)
/** 32位整型定义 */
typedef int32_t int32;
#endif

#if !defined(uint32)
/** 32位无符号整型定义 */
typedef uint32_t uint32;
#endif

#if !defined(int8)
/** 8位整型定义 */
typedef int8_t int8;
#endif

#if !defined(uint8)
/** 无符号8位整型定义 */
typedef uint8_t uint8;
#endif

typedef std::vector<char> ByteArray;
typedef cv::Point Point2i;                          ///< int类型的2d坐标点
typedef cv::Point2f Point2f;                        ///< float类型的2d坐标点
typedef std::vector<Point2i> PointsList2i;          ///< int类型的2d坐标点列表
typedef std::vector<Point2f> PointsList2f;          ///< float类型的2d坐标点列表

typedef std::vector<PointsList2i> Contours2i;        ///< int类型的点集轮廓
typedef std::vector<PointsList2f> Contours2f;        ///< float类型的点集轮廓

typedef Contours2i Textures2i;                      ///< int纹理线条类型(与轮廓结构一致)

typedef std::vector<float> AnyTensorFp32;            ///< float的AnyTensor表达式

typedef cv::Mat Matrix;                             ///< 通用矩阵表达
typedef cv::Rect_<int> Rectangle;                   ///< 通用矩形
typedef cv::Size_<int> Size;                        ///< 通用尺寸

typedef std::vector<float> Embedded;                ///< 特征嵌入稠密向量
typedef std::vector<Embedded> EmbeddedList;         ///< 特征嵌入稠密向量列表

typedef std::string String;                         ///< 字符串

typedef std::vector<int> IndexList;                 ///< 索引列表

typedef struct FaceLoc {
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    float lmk[10];
} FaceLoc;                                          ///< 人脸标准化检测地标

typedef std::vector<FaceLoc> FaceLocList;                ///< 人脸地标列表

typedef struct FaceBasicData {
    int32_t dataSize;
    void* data;
} FaceBasicData;

/** @} */

}  // namespace yh

#endif //HYPERFACE_DATATYPE_H
