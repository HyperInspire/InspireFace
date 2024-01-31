//
// Created by Tunm-Air13 on 2023/9/15.
//
#pragma once
#ifndef HYPERFACEREPO_FACEPOSEQUALITY_H
#define HYPERFACEREPO_FACEPOSEQUALITY_H

#include "../../data_type.h"
#include "middleware/any_net.h"

namespace inspire {

/**
 * @struct FacePoseQualityResult
 * @brief Structure to store the results of face pose quality analysis.
 *
 * This includes pitch, yaw, roll angles, landmarks, and their corresponding quality scores.
 */
    struct FacePoseQualityResult {
        float pitch;                ///< Pitch angle of the face.
        float yaw;                  ///< Yaw angle of the face.
        float roll;                 ///< Roll angle of the face.
        std::vector<Point2f> lmk;   ///< Landmarks of the face.
        std::vector<float> lmk_quality; ///< Quality scores for each landmark.
    };

/**
 * @class FacePoseQuality
 * @brief Class for assessing the quality of face pose using neural networks.
 *
 * Inherits from AnyNet and provides functionalities to compute face pose quality metrics.
 */
class HYPER_API FacePoseQuality : public AnyNet {
public:
    /**
     * @brief Default constructor for FacePoseQuality.
     */
    FacePoseQuality();

    /**
     * @brief Computes face pose quality for a given affine-transformed face image.
     * @param bgr_affine Affine-transformed face image in BGR format.
     * @return FacePoseQualityResult The computed face pose quality metrics.
     */
    FacePoseQualityResult operator()(const Matrix& bgr_affine);

    /**
     * @brief Computes the affine transformation matrix for face cropping.
     * @param rect Rectangle representing the face in the image.
     * @return cv::Mat The computed affine transformation matrix.
     */
    static cv::Mat ComputeCropMatrix(const cv::Rect2f &rect);

public:
    const static int INPUT_WIDTH = 96;   ///< Width of the input image for the network.
    const static int INPUT_HEIGHT = 96;  ///< Height of the input image for the network.

};

}   // namespace hyper

#endif //HYPERFACEREPO_FACEPOSEQUALITY_H
