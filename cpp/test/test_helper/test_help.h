//
// Created by Tunm-Air13 on 2023/9/12.
//

#ifndef HYPERFACEREPO_TEST_HELP_H
#define HYPERFACEREPO_TEST_HELP_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "cnpy/npy.hpp"

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

    // 遍历 data，每次填充一个向量的数据
    for (unsigned long i = 0; i < feature_num; ++i) {
        // 创建一个新的向量来存储当前向量的数据
        std::vector<float> vector_data(vector_length);

        // 将 data 中的数据按顺序填充到向量中
        for (unsigned long j = 0; j < vector_length; ++j) {
            vector_data[j] = (float)(data[i * vector_length + j]);
        }

        // 将当前向量添加到 featureMatrix
        featureMatrix.push_back(vector_data);

        // 在这里你可以添加对应的 tag 名称，例如 tagNames.push_back("Tag_Name");
    }

    // 读取文本文件
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
