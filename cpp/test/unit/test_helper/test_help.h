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
