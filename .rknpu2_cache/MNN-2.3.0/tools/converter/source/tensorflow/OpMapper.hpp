//
//  OpMapper.hpp
//  MNNConverter
//
//  Created by MNN on 2019/01/31.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef OPMAPPER_HPP
#define OPMAPPER_HPP

#include <unordered_map>

#include "MNN_generated.h"

const std::unordered_map<std::string, MNN::OpType> tfOp2MNNOp{
    {"Conv2D", MNN::OpType_Convolution},
    {"Relu", MNN::OpType_ReLU},
    {"Relu6", MNN::OpType_ReLU6},
    {"MaxPool", MNN::OpType_Pooling},
    {"AvgPool", MNN::OpType_Pooling},
    {"DepthwiseConv2dNative", MNN::OpType_ConvolutionDepthwise},
    {"FusedBatchNorm", MNN::OpType_BatchNorm},
    {"Placeholder", MNN::OpType_Input},
    {"Const", MNN::OpType_Const}, // Not All Const op to MNN!!!
    {"StridedSlice", MNN::OpType_StridedSlice},
    {"Pack", MNN::OpType_Pack},
    {"ConcatV2", MNN::OpType_Concat},
    {"Concat", MNN::OpType_Concat},
    {"Conv2DBackpropInput", MNN::OpType_Deconvolution},
    {"Add", MNN::OpType_BinaryOp},
    {"Sub", MNN::OpType_BinaryOp},
    {"Mul", MNN::OpType_BinaryOp},
    {"Div", MNN::OpType_BinaryOp},
    {"Maximum", MNN::OpType_BinaryOp},
    {"Min", MNN::OpType_Reduction},
    {"Max", MNN::OpType_Reduction},
    {"Sigmoid", MNN::OpType_Sigmoid},
    {"Softmax", MNN::OpType_Softmax},
    {"Pad", MNN::OpType_Padding},
    {"MatMul", MNN::OpType_MatMul},
    {"ResizeBilinear", MNN::OpType_Interp},
    {"ResizeNearestNeighbor", MNN::OpType_Interp},
    {"Mean", MNN::OpType_Reduction},
    {"Cast", MNN::OpType_Cast},
    {"Squeeze", MNN::OpType_Squeeze},
    {"Shape", MNN::OpType_Shape},
    {"Reshape", MNN::OpType_Reshape},
    {"Sum", MNN::OpType_Reduction},
    {"Prod", MNN::OpType_Reduction},
    {"Gather", MNN::OpType_Gather},
    {"ExpandDims", MNN::OpType_ExpandDims},
    {"AsString", MNN::OpType_AsString},
    {"ReduceJoin", MNN::OpType_ReduceJoin},
    {"Selu", MNN::OpType_Selu},
    {"Rsqrt", MNN::OpType_UnaryOp},
    {"Log", MNN::OpType_UnaryOp},
    {"Square", MNN::OpType_UnaryOp},
    {"Dequantize", MNN::OpType_Dequantize},
    {"QuantizeV2", MNN::OpType_QuantizeV2},
    {"QuantizedBiasAdd", MNN::OpType_QuantizedBiasAdd},
    {"QuantizedConv2D", MNN::OpType_TfQuantizedConv2D},
    {"RequantizationRange", MNN::OpType_RequantizationRange},
    {"Requantize", MNN::OpType_Requantize},
    {"QuantizedRelu", MNN::OpType_QuantizedRelu},
    {"QuantizedMaxPool", MNN::OpType_QuantizedMaxPool},
    {"QuantizedMatMul", MNN::OpType_QuantizedMatMul},
    {"QuantizedReshape", MNN::OpType_QuantizedReshape},
    {"Exp", MNN::OpType_UnaryOp},
    {"Neg", MNN::OpType_UnaryOp},
    {"Log1p", MNN::OpType_UnaryOp},
    {"Reciprocal", MNN::OpType_UnaryOp},
    {"RealDiv", MNN::OpType_BinaryOp},
    {"Tanh", MNN::OpType_TanH},
    {"TopKV2", MNN::OpType_TopKV2},
    {"Split", MNN::OpType_Slice},
    {"QuantizedAvgPool", MNN::OpType_QuantizedAvgPool},
    {"Abs", MNN::OpType_UnaryOp},
    {"QuantizedRelu6", MNN::OpType_QuantizedRelu6},
    {"Ceil", MNN::OpType_UnaryOp},
    {"Sqrt", MNN::OpType_UnaryOp},
    {"Slice", MNN::OpType_SliceTf},
    {"CropAndResize", MNN::OpType_CropAndResize},
    {"Fill", MNN::OpType_Fill},
    {"GatherV2", MNN::OpType_GatherV2},
    {"Minimum", MNN::OpType_BinaryOp},
    {"GreaterEqual", MNN::OpType_BinaryOp},
    {"LessEqual", MNN::OpType_BinaryOp},
    {"Less", MNN::OpType_BinaryOp},
    {"NonMaxSuppressionV2", MNN::OpType_NonMaxSuppressionV2},
    {"NonMaxSuppressionV3", MNN::OpType_NonMaxSuppressionV2},
    {"Range", MNN::OpType_Range},
    {"Rank", MNN::OpType_Rank},
    {"Size", MNN::OpType_Size},
    {"Transpose", MNN::OpType_Transpose},
    {"Unpack", MNN::OpType_Unpack},
    {"ZerosLike", MNN::OpType_ZerosLike},
    {"Greater", MNN::OpType_BinaryOp},
    {"Where", MNN::OpType_Where},
    {"Select", MNN::OpType_Select},
    {"Tile", MNN::OpType_Tile},
    {"BiasAdd", MNN::OpType_BinaryOp},
    {"LRN", MNN::OpType_LRN},
    {"SpaceToBatchND", MNN::OpType_SpaceToBatchND},
    {"BatchToSpaceND", MNN::OpType_BatchToSpaceND},
    {"ArgMax", MNN::OpType_ArgMax},
    {"SplitV", MNN::OpType_Slice},
    {"FloorDiv", MNN::OpType_BinaryOp},
    {"Moments", MNN::OpType_Moments},
    {"InstanceNorm", MNN::OpType_InstanceNorm},
    {"RNNSequenceGRU", MNN::OpType_RNNSequenceGRU},
    {"BatchMatMul", MNN::OpType_BatchMatMul},
    {"Pow", MNN::OpType_BinaryOp},
    {"SquaredDifference", MNN::OpType_BinaryOp},
    {"Equal", MNN::OpType_BinaryOp},
    {"ListDiff", MNN::OpType_SetDiff1D},
    {"SetDiff1d", MNN::OpType_SetDiff1D},
    {"DepthToSpace", MNN::OpType_DepthToSpace},
    {"SpaceToDepth", MNN::OpType_SpaceToDepth},
    {"LeakyRelu", MNN::OpType_ReLU},
    {"ReverseSequence", MNN::OpType_ReverseSequence},
    {"Elu", MNN::OpType_ELU},
    {"Conv3D", MNN::OpType_Convolution3D},
    {"MaxPool3D", MNN::OpType_Pooling3D},
    {"TensorArrayV3", MNN::OpType_TensorArray},
    {"TensorArraySizeV3", MNN::OpType_TensorArraySize},
    {"TensorArrayReadV3", MNN::OpType_TensorArrayRead},
    {"TensorArrayWriteV3", MNN::OpType_TensorArrayWrite},
    {"TensorArrayGatherV3", MNN::OpType_TensorArrayGather},
    {"TensorArrayScatterV3", MNN::OpType_TensorArrayScatter},
};

#endif // OPMAPPER_HPP
