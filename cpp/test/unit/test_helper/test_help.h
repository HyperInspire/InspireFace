//
// Created by Tunm-Air13 on 2023/9/12.
//

#ifndef HYPERFACEREPO_TEST_HELP_H
#define HYPERFACEREPO_TEST_HELP_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include "inspireface/c_api/inspireface.h"
#include "limonp/StringUtil.hpp"
#include "cnpy/npy.hpp"
#include "opencv2/opencv.hpp"

using namespace indicators;

typedef std::vector<std::pair<std::string, std::string>> FaceImageDataList;

inline FaceImageDataList LoadLFWFunneledValidData(const std::string &dir, const std::string &txtPath){
    FaceImageDataList list;
    std::ifstream file(txtPath);
    std::string line;

    while (std::getline(file, line)) {
        std::vector<std::string> parts;
        limonp::Split(line, parts, "/");
        if (parts.size() >= 2) {
            std::string name = parts[0];
            std::string fullPath = dir + "/" + line;

            list.push_back({name, fullPath});
        }
    }

    return list;
}

inline bool ImportLFWFunneledValidData(HContextHandle handle, FaceImageDataList& data, size_t importNum) {
    auto dataSize = data.size();
    std::string title = "Import " + std::to_string(importNum) + " face data...";
    // Hide cursor
    show_console_cursor(false);
    BlockProgressBar bar{
            option::BarWidth{60},
            option::Start{"["},
            option::End{"]"},
            option::PostfixText{title},
            option::ForegroundColor{Color::white}  ,
            option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
    };

    auto progress = 0.0f;
    for (size_t i = 0; i < importNum; ++i) {
        bar.set_progress(progress);
        size_t index = i % dataSize;
        // Data processing
        auto item = data[index];
        cv::Mat image = cv::imread(item.second);
        HF_ImageData imageData = {0};
        imageData.data = image.data;
        imageData.height = image.rows;
        imageData.width = image.cols;
        imageData.format = STREAM_BGR;
        imageData.rotation = CAMERA_ROTATION_0;
        HImageHandle imgHandle;
        auto ret = HF_CreateImageStream(&imageData, &imgHandle);
        if (ret != HSUCCEED || image.empty()) {
            std::cerr << "Error image: " << std::to_string(ret)  << " , " << item.second << std::endl;
            return false;
        }
        // Face tracked
        HF_MultipleFaceData multipleFaceData = {0};
        ret = HF_FaceContextRunFaceTrack(handle, imgHandle, &multipleFaceData);

        if (ret != HSUCCEED) {
            std::cerr << "Error Track: " << std::to_string(ret)  << " , " << item.second << std::endl;
            return false;
        }

        if (multipleFaceData.detectedNum == 0) {
            std::cerr << "Not Detected face: " << item.second << std::endl;
            return false;
        }

        // Extract face feature
        HF_FaceFeature feature = {0};
        ret = HF_FaceFeatureExtract(handle, imgHandle, multipleFaceData.tokens[0], &feature);
        if (ret != HSUCCEED) {
            std::cerr << "Error extract: " << std::to_string(ret)  << " , " << item.second << std::endl;
            return false;
        }
        char *newTagName = new char[item.first.size() + 1];
        std::strcpy(newTagName, item.first.c_str());
        HF_FaceFeatureIdentity identity = {0};
        identity.customId = index;
        identity.tag = newTagName;
        identity.feature = &feature;
        ret = HF_FeaturesGroupInsertFeature(handle, identity);
        if (ret != HSUCCEED) {
            std::cerr << "Error insert feature: " << std::to_string(ret)  << " , " << item.second << std::endl;
            return false;
        }

        delete[] newTagName;
        HF_ReleaseImageStream(imgHandle);
        // Update progress
        progress = 100.0f * (float)(i + 1) / importNum;
    }
    bar.set_progress(100.0f);
    // Show cursor
    show_console_cursor(true);
    std::cout << "\033[0m\n"; // ANSI resets the color code

    return true;
}

inline std::pair<std::vector<std::vector<float>>, std::vector<std::string>> LoadMatrixAndTags(
        const std::string& matrixFileName, const std::string& tagsFileName) {

    std::vector<std::vector<float>> featureMatrix;
    std::vector<std::string> tagNames;


    std::vector<unsigned long> shape {};
    bool fortran_order;
    std::vector<double> data;

    npy::LoadArrayFromNumpy(matrixFileName, shape, fortran_order, data);
    auto feature_num = shape[0];
    unsigned long vector_length = shape[1];
    assert(shape[1] == 512);

    // Iterate through the data, putting the data one vector ata time
    for (unsigned long i = 0; i < feature_num; ++i) {
        //Creates a new vector to store the data of the current vector
        std::vector<float> vector_data(vector_length);

        // Put the data in data into the vector in order
        for (unsigned long j = 0; j < vector_length; ++j) {
            vector_data[j] = (float)(data[i * vector_length + j]);
        }

        // Adds the current vector to featureMatrix
        featureMatrix.push_back(vector_data);

        // Here you can add the corresponding tag name, such as tagNames.push_back("Tag_Name");
    }

    // Read txt file
    std::ifstream txtFile(tagsFileName);
    if (txtFile.is_open()) {
        std::string line;
        while (std::getline(txtFile, line)) {
            tagNames.push_back(line);
        }
        txtFile.close();
    } else {
        std::cerr << "Unable to open the text file." << std::endl;
    }
//
    return std::make_pair(featureMatrix, tagNames);
}



#endif //HYPERFACEREPO_TEST_HELP_H
