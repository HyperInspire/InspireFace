//
//  NPUUnary.hpp
//  MNN
//
//  Created by MNN on b'2020/10/15'.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef NPUDEMO_NPUUnary_HPP
#define NPUDEMO_NPUUnary_HPP

#include "NPUCommonExecution.hpp"
#include "NPUBackend.hpp"

namespace MNN {

class NPUUnary : public NPUCommonExecution {
public:
    NPUUnary(Backend *b, const Op *op, const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs);
    ErrorCode onResize(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs);
    virtual ~NPUUnary() = default;
   
};
} // namespace MNN

#endif // NPUDEMO_NPUUnary_HPP
