//
// Created by tunm on 2023/10/3.
//

#ifndef HYPERFACEREPO_HTYPEDEF_H
#define HYPERFACEREPO_HTYPEDEF_H


typedef void*               HPVoid;
typedef void*               HImageHandle;
typedef void*               HContextHandle;
typedef long                HLong;
typedef float               HFloat;
typedef double              HDouble;
typedef	unsigned char		HUInt8;
typedef signed int			HInt32;
typedef long                HResult;
typedef char*               HString;

typedef struct HFaceRect {
    HInt32 x;
    HInt32 y;
    HInt32 width;
    HInt32 height;
} HFaceRect;

typedef struct HFaceEulerAngle {
    HFloat* roll;
    HFloat* yaw;
    HFloat* pitch;
} HFaceEulerAngle;

#endif //HYPERFACEREPO_HTYPEDEF_H
