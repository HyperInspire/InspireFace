//
//  ConvolutionInt8Executor.hpp
//  MNN
//
//  Created by MNN on 2018/07/16.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef ConvolutionInt8Executor_hpp
#define ConvolutionInt8Executor_hpp

#include <stdio.h>
#include "core/AutoStorage.h"
#include "backend/cpu/compute/ConvolutionFloatFactory.h"
#include "backend/cpu/compute/ConvolutionIntFactory.hpp"
#include "backend/cpu/CPUConvolution.hpp"

namespace MNN {
class ConvolutionInt8Executor : public CPUConvolution {
public:
    ConvolutionInt8Executor(const Convolution2DCommon *convOp, Backend *b,
                            const ConvolutionCommon::Int8Common *common, const float *bias, size_t biasSize);
    virtual ~ConvolutionInt8Executor();
    virtual ErrorCode onExecute(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) override;
    virtual ErrorCode onResize(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) override;

private:
    std::shared_ptr<Tensor> mWeight;
    AutoStorage<float> mAlpha;
    AutoStorage<float> mBias;
    const IDSTQuan *mQuan;
    Tensor mSrcCopyBuffer;

    Tensor mTempBuffer;
    Tensor mTempDstBuffer;
    ConvolutionCommon::Im2ColParameter mIm2ColParamter;
    int mSrcCount;
    float mAMin;
    float mAMax;
    float mQuanScale;
    std::vector<float> mPostParameters;
    // mFakeBias used by GemmKernel
    std::shared_ptr<Tensor> mFakeBias;
};
} // namespace MNN

#endif /* ConvolutionInt8Executor_hpp */
