//
//  MetalReLU6.hpp
//  MNN
//
//  Created by MNN on 2019/01/30.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef MetalReLU6_hpp
#define MetalReLU6_hpp

#import "core/Execution.hpp"
#import "MetalDefine.h"

#if MNN_METAL_ENABLED
namespace MNN {

class MetalReLU6 : public Execution {
public:
    MetalReLU6(Backend *backend, float minValue, float maxValue);
    virtual ~MetalReLU6() = default;
    virtual ErrorCode onExecute(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) override;
private:
    id<MTLBuffer> mConst;
};

} // namespace MNN
#endif /* MNN_METAL_ENABLED */
#endif /* MetalReLU6_hpp */
