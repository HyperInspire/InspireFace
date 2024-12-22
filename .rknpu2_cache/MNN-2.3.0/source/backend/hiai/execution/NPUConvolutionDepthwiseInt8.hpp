//
//  NPUConvolutionDepthwiseInt8.hpp
//  MNN
//
//  Created by MNN on b'2020/10/15'.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef NPUDEMO_NPUConvolutionDepthwise_INT8_HPP
#define NPUDEMO_NPUConvolutionDepthwise_INT8_HPP

#include "NPUCommonExecution.hpp"
#include "NPUBackend.hpp"

namespace MNN {

class NPUConvolutionDepthwiseInt8 : public NPUCommonExecution {
public:
    NPUConvolutionDepthwiseInt8(Backend *b, const Op *op, const std::vector<Tensor *> &inputs,
                            const std::vector<Tensor *> &outputs);
    ErrorCode onResize(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs);
    virtual ~NPUConvolutionDepthwiseInt8() = default;
   
private:
    ge::op::Const mConst_w;
    ge::op::Const mConst_b;

    shared_ptr<ge::op::Activation> mRelu_conv;
};

} // namespace MNN

#endif // NPUDEMO_NPUConvolutionDepthwise_INT8_HPP
