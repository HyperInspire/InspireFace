//
// Created by Tunm-Air13 on 2023/9/11.
//

#ifndef HYPERFACEREPO_FEATUREBLOCKOPENCV_H
#define HYPERFACEREPO_FEATUREBLOCKOPENCV_H
#include "recognition_module/features_block/FeatureBlock.h"

namespace hyper {

class HYPER_API FeatureBlockOpenCV : public FeatureBlock{
public:
    explicit FeatureBlockOpenCV(int32_t features_max = 512, int32_t feature_length = 512);

    int32_t SearchNearest(const std::vector<float>& queryFeature, SearchResult &searchResult) override;

    int32_t GetFeature(int row, vector<float> &feature) override;

protected:
    int32_t UnsafeAddFeature(const vector<float> &feature, const std::string &tag) override;

    int32_t UnsafeDeleteFeature(int rowToDelete) override;

    int32_t UnsafeUpdateFeature(int rowToUpdate, const vector<float> &newFeature, const std::string &tag) override;

    int32_t UnsafeRegisterFeature(int rowToUpdate, const vector<float> &feature, const std::string &tag) override;


public:
    void PrintMatrixSize() override;

    void PrintMatrix() override;

private:

    cv::Mat m_feature_matrix_;

};

}   // namespace hyper


#endif //HYPERFACEREPO_FEATUREBLOCKOPENCV_H
