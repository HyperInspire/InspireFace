//
//  MetalConvolutionGEMM.hpp
//  MNN
//
//  Created by MNN on 2019/01/31.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef MetalConvolutionGEMM_hpp
#define MetalConvolutionGEMM_hpp

#import "MetalConvolutionCommon.hpp"

#if MNN_METAL_ENABLED
namespace MNN {

class MetalConvolutionGEMM : public MetalConvolutionCommon {
public:
    static bool isValid(const Convolution2D *conv, const Tensor *input);
    MetalConvolutionGEMM(Backend *backend, const Tensor *input, const MNN::Op *op);
    virtual ~MetalConvolutionGEMM() = default;
    virtual ErrorCode onResize(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) override;
    virtual ErrorCode onExecute(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) override;

protected:
    virtual ErrorCode onFloat(const Tensor *input, const Tensor *output) override;
    virtual id<MTLBuffer> weightForFloat(int group, int oc, int ic, int kh, int kw, const float *src) override;

private:
    id<MTLBuffer> mShapeBuffer = nil;
    std::shared_ptr<Tensor> mTempInput;
    std::shared_ptr<Tensor> mTempOutput;
    id<MTLComputePipelineState> mPipelineGEMM;
    std::pair<MTLSize, MTLSize> mGemm;
    id<MTLComputePipelineState> mPipelineIm2Col;
    std::pair<MTLSize, MTLSize> mIm2Col;
    id<MTLComputePipelineState> mPipelineCol2Im;
    std::pair<MTLSize, MTLSize> mCol2Im;
};

} // namespace MNN
#endif /* MNN_METAL_ENABLED */
#endif /* MetalConvolutionGEMM_hpp */
