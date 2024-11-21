/**
 * @author Jingyu Yan
 * @date 2024-10-01
 */

#include "mask_predict_adapt.h"

namespace inspire {

MaskPredictAdapt::MaskPredictAdapt() : AnyNetAdapter("MaskPredictAdapt") {}

float MaskPredictAdapt::operator()(const inspirecv::Image &bgr_affine) {
    AnyTensorOutputs outputs;
    if (bgr_affine.Height() == m_input_size_ && bgr_affine.Width() == m_input_size_) {
        Forward(bgr_affine, outputs);
    } else {
        auto resized = bgr_affine.Resize(m_input_size_, m_input_size_);
        Forward(resized, outputs);
    }

    return outputs[0].second[0];
}

}  // namespace inspire