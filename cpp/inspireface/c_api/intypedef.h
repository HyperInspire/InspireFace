//
// Created by tunm on 2023/10/3.
//

#ifndef INSPIREFACE_INTYPEDEF_H
#define INSPIREFACE_INTYPEDEF_H

// clang-format off
typedef void*               HPVoid;                           ///< Pointer to Void.
typedef void*               HFImageStream;                   ///< Handle for image.
typedef void*               HFSession;                       ///< Handle for context.
typedef void*               HFImageBitmap;                   ///< Handle for image bitmap.
typedef long                HLong;                            ///< Long integer.
typedef float               HFloat;                          ///< Single-precision floating point.
typedef float*              HPFloat;                         ///< Pointer to Single-precision floating point.
typedef double              HDouble;                          ///< Double-precision floating point.
typedef	unsigned char		HUInt8;                           ///< Unsigned 8-bit integer.
typedef signed int			HInt32;                           ///< Signed 32-bit integer.
typedef signed int			HOption;                          ///< Signed 32-bit integer option.
typedef signed int*			HPInt32;                          ///< Pointer to signed 32-bit integer.
typedef long                HResult;                          ///< Result code.
typedef char*               HString;                          ///< String.
typedef const char*         HPath;                            ///< Const String.
typedef char                HBuffer;                            ///< Character.
typedef char                HChar;                            ///< Character.
typedef char*               HPBuffer;                           ///< Pointer Character.
typedef long                HSize;                            ///< Size
typedef long*               HPSize;                            ///< Pointer Size
// clang-format on  

typedef struct HFaceRect {
    HInt32 x;             ///< X-coordinate of the top-left corner of the rectangle.
    HInt32 y;             ///< Y-coordinate of the top-left corner of the rectangle.
    HInt32 width;         ///< Width of the rectangle.
    HInt32 height;        ///< Height of the rectangle.
} HFaceRect;         ///< Rectangle representing a face region.

typedef struct HPoint2f{
    HFloat x;          ///< X-coordinate
    HFloat y;          ///< Y-coordinate
} HPoint2f;

typedef struct HPoint2i{
    HInt32 x;          ///< X-coordinate
    HInt32 y;          ///< Y-coordinate
} HPoint2i;

typedef struct HColor {
    HFloat r;          ///< Red component
    HFloat g;          ///< Green component
    HFloat b;          ///< Blue component
} HFColor;

#endif //INSPIREFACE_INTYPEDEF_H
