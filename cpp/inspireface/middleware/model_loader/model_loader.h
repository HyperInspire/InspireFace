//
// Created by Tunm-Air13 on 2021/9/17.
//
#pragma once
#ifndef FACEIN_LIB_MODEL_LOADER_H
#define FACEIN_LIB_MODEL_LOADER_H

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace inspire {


/**
 * @enum LOADER_STATUS_CODE
 * @brief Enum for loader status codes.
 *
 * This enum defines status codes for the loader, such as PASS, LICENSE_PAST_DUE, etc.
 */
enum LOADER_STATUS_CODE {
    PASS = 0,                   ///< Success
    LICENSE_PAST_DUE = 1,       ///< License expired
    LICENSE_ERROR = 2,          ///< License authentication error
    LICENSE_FORMAT_ERROR = 3,   ///< License format error
    PACK_ERROR = 4,             ///< File error
    PACK_VERSION_ERROR = 5,     ///< Version number error
    PACK_MODELS_NOT_MATCH = 6   ///< Model count mismatch
};

/**
 * @struct ModelSize
 * @brief Struct to represent the size of a model.
 *
 * This struct contains the size of the prototxt and caffemodel files for a model.
 */
struct ModelSize {
    int prototxt_size;    ///< Size of the prototxt file.
    int caffemodel_size;  ///< Size of the caffemodel file.
};

/**
 * @struct Model
 * @brief Struct to represent a model with its data buffers and size.
 *
 * This struct contains pointers to the prototxt and caffemodel data buffers as well as their sizes.
 */
struct Model {
    char *prototxtBuffer;   ///< Pointer to the prototxt data buffer.
    char *caffemodelBuffer; ///< Pointer to the caffemodel data buffer.
    ModelSize modelsize;    ///< Size of the model data.
};

/**
 * @class ModelLoader
 * @brief Class for loading and managing models.
 *
 * This class provides methods for loading models, retrieving model data, and checking the loader status.
 */
class ModelLoader {
public:
    /**
     * @brief Initialize the model loader.
     */
    ModelLoader();

    /**
     * @brief Initialize the model loader with a specific model path.
     *
     * @param model_path The path to the model files.
     */
    explicit ModelLoader(std::string model_path);

    /**
     * @brief Reset the model loader with a new model path.
     *
     * @param model_path The path to the model files.
     */
    void Reset(const std::string &model_path);

    /**
     * @brief Destructor for the model loader.
     */
    ~ModelLoader();

    /**
     * @brief Read a model by index.
     *
     * @param idx The index of the model to read.
     * @return Model* Pointer to the model data.
     */
    Model *ReadModel(int idx);

    /**
     * @brief Read a model by name.
     *
     * @param name The name of the model to read.
     * @return Model* Pointer to the model data.
     */
    Model *ReadModel(const std::string &name);

    /**
     * @brief Get the number of loaded models.
     *
     * @return std::size_t The number of loaded models.
     */
    std::size_t Count() const;

    /**
     * @brief Get the status code of the loader.
     *
     * @return int The status code.
     */
    int GetStatusCode();

    /**
     * @brief Get the MagicNumber
     *
     * @return MagicNumber.
     */
    int GetMagicNumber();

private:
    ModelSize *modelSize;           ///< Array of model sizes.
    Model *model;                   ///< Array of model data.
    int number_of_models;           ///< Number of loaded models.
    int magic_number;               ///< Magic number.
    std::map<std::string, int> n2i_;///< Model name to index mapping.
    int status_ = -1;               ///< Loader status code.
};
} // namespace inspire

#endif //FACEIN_LIB_MODEL_LOADER_H
