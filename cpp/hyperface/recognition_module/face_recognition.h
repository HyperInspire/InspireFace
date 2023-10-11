//
// Created by tunm on 2023/9/8.
//
#pragma once
#ifndef HYPERFACEREPO_FACERECOGNITION_H
#define HYPERFACEREPO_FACERECOGNITION_H
#include <mutex>
#include "extract/extract.h"
#include "common/face_info/face_object.h"
#include "common/face_data/data_tools.h"
#include "middleware/camera_stream/camera_stream.h"
#include "features_block/feature_block.h"
#include "persistence/sqlite_faces_manage.h"

namespace hyper {

typedef struct DatabaseConfiguration {
    bool enable_use_db = false;                    ///< 是否打开数据持久化
    std::string  db_path;
} DatabaseConfiguration;

class HYPER_API FaceRecognition {
public:
    FaceRecognition(ModelLoader &loader, bool enable_recognition, MatrixCore core = MC_OPENCV, int feature_block_num = 8);

    int32_t ConfigurationDB(DatabaseConfiguration& configuration);

    // 计算余弦相似性
    static int32_t CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2, float &res);

    static int32_t CosineSimilarity(const float* v1, const float *v2, int32_t size, float &res);

    int32_t FaceExtract(CameraStream &image, const FaceObject& face, Embedded &embedded);

    int32_t FaceExtract(CameraStream &image, const HyperFaceData& face, Embedded &embedded);

    int32_t RegisterFaceFeature(const std::vector<float>& feature, int featureIndex, const std::string &tag, int32_t customId);

    int32_t UpdateFaceFeature(const std::vector<float>& feature, int featureIndex, const std::string &tag, int32_t customId);

    int32_t SearchFaceFeature(const std::vector<float>& queryFeature, SearchResult &searchResult, float threshold = 0.5f, bool mostSimilar=true);

    int32_t InsertFaceFeature(const std::vector<float>& feature, const std::string &tag, int32_t customId);

    int32_t DeleteFaceFeature(int featureIndex);

    int32_t GetFaceFeature(int featureIndex, Embedded &feature);

    int32_t GetFaceFeatureCount();

    int32_t FindFeatureIndexByCustomId(int32_t customId);

    void PrintFeatureMatrixInfo();

    const std::shared_ptr<Extract> &getMExtract() const;

    int32_t GetFeatureNum() const;

    int32_t ViewDBTable();

private:
    int32_t InitExtractInteraction(Model *model);

private:

    DatabaseConfiguration m_db_configuration_;

    std::shared_ptr<SQLiteFaceManage> m_db_;

    std::shared_ptr<Extract> m_extract_;

    std::vector<std::shared_ptr<FeatureBlock>> m_feature_matrix_list_;

    // 临时固定
    const int32_t NUM_OF_FEATURES_IN_BLOCK = 512;


};

}   // namespace hyper

#endif //HYPERFACEREPO_FACERECOGNITION_H
