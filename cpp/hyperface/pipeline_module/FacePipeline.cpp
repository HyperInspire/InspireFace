//
// Created by tunm on 2023/9/7.
//

#include "FacePipeline.h"
#include "model_index.h"

namespace hyper {

FacePipeline::FacePipeline(ModelLoader &loader, hyper::CustomPipelineParameter param) {
    m_param_ = param;

    if (m_param_.enable_recognition) {
        auto ret = InitRecognition(loader.ReadModel(ModelIndex::_3_extract));
        if (ret != 0) {
            LOGE("InitRecognition error.");
        }
    }
}

int32_t FacePipeline::InitRecognition(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_3_extract);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"fc1_scale", });
    param.set<vector<int>>("input_size", {112, 112});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125, 0.0078125, 0.0078125});

    m_recognition_ = make_shared<Recognition>();
    m_recognition_->LoadParam(param, model);

    return 0;
}


}