//
//  TorchNot.cpp
//  MNNConverter
//
//  Created by MNN on 2021/10/18.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include "MNN_generated.h"
#include "TorchExtraManager.hpp"
#include "logkit.h"

namespace MNN {
namespace Express {

class TorchNotTransform : public TorchExtraManager::Transform {
public:
    virtual EXPRP onExecute(EXPRP expr) const override {
        auto input   = expr->inputs()[0];
        auto one    = _Scalar<int32_t>(1);
        auto newExpr = _Negative(input-one)->expr().first;
        newExpr->setName(expr->name());
        return newExpr;
    }
};

static auto gRegister = []() {
    TorchExtraManager::get()->insert("__not__", std::shared_ptr<TorchExtraManager::Transform>(new TorchNotTransform));
    return true;
}();
} // namespace Express
} // namespace MNN
