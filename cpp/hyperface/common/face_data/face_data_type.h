//
// Created by tunm on 2023/9/17.
//
#pragma once
#ifndef HYPERFACEREPO_FACEDATATYPE_H
#define HYPERFACEREPO_FACEDATATYPE_H

#include "../../data_type.h"
#include "../face_info/face_object.h"

namespace hyper {


typedef struct Face3DAngle {
    float roll;
    float yaw;
    float pitch;
} Face3DAngle;

typedef struct FaceRect {
    int x;
    int y;
    int width;
    int height;
} FaceRect;

typedef struct Point2F {
    float x;
    float y;
} HPoint;

typedef struct TransMatrix {
    double m00;
    double m01;
    double m10;
    double m11;
    double tx;
    double ty;
} TransMatrix;

typedef struct HyperFaceData {
    int trackState;
    int inGroupIndex;
    int trackId;
    int trackCount;
    FaceRect rect;
    TransMatrix trans;
    Point2F keyPoints[5];
    Face3DAngle face3DAngle;
    float quality[5];
} HyperFaceData;


}

#endif //HYPERFACEREPO_FACEDATATYPE_H
