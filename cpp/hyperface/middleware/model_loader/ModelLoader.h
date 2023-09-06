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

namespace hyper {

enum LOADER_STATUS_CODE {
    PASS = 0,                   ///< 通过
    LICENSE_PAST_DUE = 1,       ///< 过期
    LICENSE_ERROR = 2,          ///< 认证错误
    LICENSE_FORMAT_ERROR = 3,   ///< 格式错误
    PACK_ERROR = 4,             ///< 文件错误
    PACK_VERSION_ERROR = 5      ///< 版本号错误
};

struct ModelSize {
    int prototxt_size;
    int caffemodel_size;
};

struct Model {
    char *prototxtBuffer;
    char *caffemodelBuffer;
    ModelSize modelsize;
};

class ModelLoader {
public:
    /**
     * @brief 初始化算法资源加载器
     * */
    ModelLoader();

    /**
     * @brief 初始化算法资源加载器 以原始文件路径的形式加载
     *
     * @param model_path 模型路径
     * */
    explicit ModelLoader(std::string model_path);

    /**
     * @brief 加载模型
     *
     * @param model_path 模型路径
     * */
    void Reset(const std::string &model_path);


    ~ModelLoader();

    /**
     * @brief 获取模型
     *
     * @param idx 模型index
     * */
    Model *ReadModel(int idx);

    /**
     * @brief 获取模型
     *
     * @param name 模型名称
     * */
    Model *ReadModel(const std::string &name);

    /**
     * @brief 模型数量
     * */
    std::size_t Count() const;


    /**
     * @brief 获取license认证状态
     * */
    int GetStatusCode();

private:
    // 模型尺寸
    ModelSize *modelSize;
    // 模型指针
    Model *model;
    // 模型总数
    int number_of_models;
    // 魔法值
    int magic_number;
    // 数量字典
    std::map<std::string, int> n2i_;

    int status_ = -1;
};
} // namespace pr

#endif //FACEIN_LIB_MODEL_LOADER_H
