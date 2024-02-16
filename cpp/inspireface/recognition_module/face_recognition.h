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

namespace inspire {

/**
 * @struct DatabaseConfiguration
 * @brief Structure to configure database settings for FaceRecognition.
 */
    typedef struct DatabaseConfiguration {
        bool enable_use_db = false; ///< Whether to enable data persistence.
        std::string  db_path;      ///< Path to the database file.
    } DatabaseConfiguration;

/**
 * @class FaceRecognition
 * @brief Class for performing face recognition tasks.
 *
 * This class provides methods for face feature extraction, registration, update, search, and more.
 */
class INSPIRE_API FaceRecognition {
public:
    /**
     * @brief Constructor for FaceRecognition class.
     *
     * @param loader ModelLoader instance for model loading.
     * @param enable_recognition Whether face recognition is enabled.
     * @param core Type of matrix core to use for feature extraction.
     * @param feature_block_num Number of feature blocks to use.
     */
    FaceRecognition(ModelLoader &loader, bool enable_recognition, MatrixCore core = MC_OPENCV, int feature_block_num = 20);

    /**
     * @brief Computes the cosine similarity between two feature vectors.
     *
     * @param v1 First feature vector.
     * @param v2 Second feature vector.
     * @param res Output parameter to store the cosine similarity result.
     * @return int32_t Status code indicating success (0) or failure.
     */
    static int32_t CosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2, float &res);

    /**
     * @brief Computes the cosine similarity between two feature vectors.
     *
     * @param v1 Pointer to the first feature vector.
     * @param v2 Pointer to the second feature vector.
     * @param size Size of the feature vectors.
     * @param res Output parameter to store the cosine similarity result.
     * @return int32_t Status code indicating success (0) or failure.
     */
    static int32_t CosineSimilarity(const float* v1, const float *v2, int32_t size, float &res);

    /**
     * @brief Extracts a facial feature from an image and stores it in the provided 'embedded'.
     *
     * @param image CameraStream instance containing the image.
     * @param face FaceObject representing the detected face.
     * @param embedded Output parameter to store the extracted facial feature.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t FaceExtract(CameraStream &image, const FaceObject& face, Embedded &embedded);

    /**
     * @brief Extracts a facial feature from an image and stores it in the provided 'embedded'.
     *
     * @param image CameraStream instance containing the image.
     * @param face HyperFaceData representing the detected face.
     * @param embedded Output parameter to store the extracted facial feature.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t FaceExtract(CameraStream &image, const HyperFaceData& face, Embedded &embedded);

    /**
     * @brief Registers a facial feature in the feature block.
     *
     * @param feature Vector of floats representing the feature.
     * @param featureIndex Index of the feature in the block.
     * @param tag String tag associated with the feature.
     * @param customId Custom identifier for the feature.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t RegisterFaceFeature(const std::vector<float>& feature, int featureIndex, const std::string &tag, int32_t customId);

    /**
     * @brief Updates a facial feature in the feature block.
     *
     * @param feature Vector of floats representing the updated feature.
     * @param featureIndex Index of the feature in the block.
     * @param tag New string tag for the feature.
     * @param customId Custom identifier for the feature.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t UpdateFaceFeature(const std::vector<float>& feature, int featureIndex, const std::string &tag, int32_t customId);

    /**
     * @brief Searches for the nearest facial feature in the feature block to a given query feature.
     *
     * @param queryFeature Query feature vector.
     * @param searchResult SearchResult structure to store the search results.
     * @param threshold Threshold for considering a match.
     * @param mostSimilar Whether to find the most similar feature.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t SearchFaceFeature(const std::vector<float>& queryFeature, SearchResult &searchResult, float threshold = 0.5f, bool mostSimilar=true);

    /**
     * @brief Inserts a facial feature into the feature block.
     *
     * @param feature Vector of floats representing the feature.
     * @param tag String tag associated with the feature.
     * @param customId Custom identifier for the feature.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t InsertFaceFeature(const std::vector<float>& feature, const std::string &tag, int32_t customId);

    /**
     * @brief Deletes a facial feature from the feature block.
     *
     * @param featureIndex Index of the feature to delete.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t DeleteFaceFeature(int featureIndex);

    /**
     * @brief Retrieves a facial feature from the feature block.
     *
     * @param featureIndex Index of the feature to retrieve.
     * @param feature Output parameter to store the retrieved feature.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t GetFaceFeature(int featureIndex, Embedded &feature);

    /**
     * @brief Retrieves a facial entity from the feature block.
     *
     * @param featureIndex Index of the feature to retrieve.
     * @param result Output parameter to store the retrieved entity.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t GetFaceEntity(int featureIndex, Embedded &feature, std::string& tag, FEATURE_STATE& status);

    /**
     * @brief Retrieves the total number of facial features stored in the feature block.
     *
     * @return int32_t Total number of facial features.
     */
    int32_t GetFaceFeatureCount();

    /**
     * @brief Finds the index of a feature by its custom ID.
     *
     * @param customId Custom identifier to search for.
     * @return int32_t Index of the feature with the given custom ID, or -1 if not found.
     */
    int32_t FindFeatureIndexByCustomId(int32_t customId);

    /**
     * @brief Prints information about the feature matrix.
     */
    void PrintFeatureMatrixInfo();

    /**
     * @brief Gets the Extract instance associated with this FaceRecognition.
     *
     * @return const std::shared_ptr<Extract>& Pointer to the Extract instance.
     */
    const std::shared_ptr<Extract> &getMExtract() const;

    /**
     * @brief Gets the number of features in the feature block.
     *
     * @return int32_t Number of features.
     */
    int32_t GetFeatureNum() const;

private:
    /**
     * @brief Initializes the interaction with the Extract model.
     *
     * @param model Pointer to the loaded model.
     * @return int32_t Status code indicating success (0) or failure.
     */
    int32_t InitExtractInteraction(Model *model);

private:
    std::shared_ptr<Extract> m_extract_; ///< Pointer to the Extract instance.
    std::vector<std::shared_ptr<FeatureBlock>> m_feature_matrix_list_; ///< List of feature blocks.
    const int32_t NUM_OF_FEATURES_IN_BLOCK = 512; ///< Number of features in each feature block.
    int m_mb_;
};

}   // namespace inspire

#endif //HYPERFACEREPO_FACERECOGNITION_H
