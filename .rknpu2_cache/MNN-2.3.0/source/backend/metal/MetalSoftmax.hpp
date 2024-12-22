//
//  MetalSoftmax.hpp
//  MNN
//
//  Created by MNN on 2019/01/30.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef MetalSoftmax_hpp
#define MetalSoftmax_hpp

#import "core/Execution.hpp"

#if MNN_METAL_ENABLED
namespace MNN {

class MetalSoftmax : public Execution {
public:
    MetalSoftmax(Backend *backend, int32_t axis);
    virtual ~MetalSoftmax() = default;
    virtual ErrorCode onResize(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) override;
    virtual ErrorCode onExecute(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) override;

private:
    int32_t mAxis;
    int32_t mAxisLen;
    id<MTLBuffer> mShapeBuffer;
    id<MTLComputePipelineState> mPipeline;
    std::pair<MTLSize, MTLSize> mThreads;
};

} // namespace MNN
#endif /* MNN_METAL_ENABLED */
#endif /* MetalSoftmax_hpp */
