//
// Created by Tunm-Air13 on 2022/10/10.
//

#ifndef MAGIC_GESTURES_RKNN_ADAPTER_H
#define MAGIC_GESTURES_RKNN_ADAPTER_H

#include <iostream>
#include "opencv2/opencv.hpp"
#include "rknn_api.h"
#include "DataType.h"

namespace solex {

/**
 * @brief 获取RKNN数据类型字符串
 * @ingroup NeuralNetwork
 * @param type 数据类型
 * @return 字符串编码类型
 */
inline const char *get_type_string_(rknn_tensor_type type) {
    switch (type) {
        case RKNN_TENSOR_FLOAT32:
            return "FP32";
        case RKNN_TENSOR_FLOAT16:
            return "FP16";
        case RKNN_TENSOR_INT8:
            return "INT8";
        case RKNN_TENSOR_UINT8:
            return "UINT8";
        case RKNN_TENSOR_INT16:
            return "INT16";
        default:
            return "UNKNOW";
    }
}

inline const char *get_qnt_type_string_(rknn_tensor_qnt_type type) {
    switch (type) {
        case RKNN_TENSOR_QNT_NONE:
            return "NONE";
        case RKNN_TENSOR_QNT_DFP:
            return "DFP";
        case RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC:
            return "AFFINE";
        default:
            return "UNKNOW";
    }
}

inline void print_tensor_attr_(const rknn_tensor_attr &attr) {
    printf("  n_dims:%d \n", attr.n_dims);
    printf("  [ ");
    for (int i = 0; i < attr.n_dims; i++) {
        printf(" %d ", attr.dims[i]);
    }
    printf("] \n");
    printf("  size:%d \n", attr.size);
    printf("  n_elems:%d \n", attr.n_elems);
    printf("  scale:%f \n", attr.scale);
    printf("  name:%s \n", attr.name);
}

inline unsigned char *load_data_(FILE *fp, size_t ofst, size_t sz) {
    unsigned char *data;
    int ret;

    data = NULL;

    if (NULL == fp) {
        return NULL;
    }

    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0) {
        printf("blob seek failure.\n");
        return NULL;
    }

    data = (unsigned char *) malloc(sz);
    if (data == NULL) {
        printf("buffer malloc failure.\n");
        return NULL;
    }
    ret = fread(data, 1, sz, fp);
    return data;
}

inline unsigned char *load_model_(const char *filename, int *model_size) {
    FILE *fp;
    unsigned char *data;

    fp = fopen(filename, "rb");
    if (NULL == fp) {
        printf("Open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    data = load_data_(fp, 0, size);

    fclose(fp);

    *model_size = size;
    return data;
}

/**
 * RKNN执行推理的状态
 * @ingroup NeuralNetwork
 */
enum Status {
    SUCCESS = 0,                ///< 执行成功
    ERROR_SHAPE_MATCH = 1,      ///< 执行错误，tensor的形状不匹配
    ERROR_DATA_ORDER = 2        ///< 执行错误，tensor数据排序错误
};

/**
 * @brief RKNN神经网络推理适配器
 * @details 自定义化通用型的RKNN推理适配包装类, 可作与其他需要使用RKNN进行神经网络推理的任务进行组合使用
 * @ingroup NeuralNetwork
 */
class RKNNAdapter {
public:
    // 禁止拷贝
    RKNNAdapter(const RKNNAdapter &) = delete;
    RKNNAdapter &operator=(const RKNNAdapter &) = delete;
    RKNNAdapter() = default;

    /**
     * @brief 手动初始化
     * @details 初始化rknn模型，会申请一块内存进行创建推理器会话
     * @param model_path rknn模型的路径
     * @return 返回初始化结果
     */
    int Initialize(const char *model_path) {
        /* Create the neural network */
        int model_data_size = 0;
        model_data = load_model_(model_path, &model_data_size);
        int ret = rknn_init(&rk_ctx_, model_data, model_data_size, 0);
//        LOG_INFO("RKNN Init ok.");
        if (ret < 0) {
            LOG_ERROR("rknn_init fail! ret={}", ret);
            return -1;
        }

        return init_();
    }

//    /**
//     * @brief 手动初始化
//     * @details 初始化rknn模型，会申请一块内存进行创建推理器会话
//     * @param model 传入SolexCV视觉库中的模型类型指针
//     * @return 返回初始化结果
//     */
//    int Initialize(solexcv::Model *model) {
//        /* Create the neural network */
//        int model_data_size = 0;
//        LOG_INFO("Read models size: {:.1f} MB", round(float(model->modelsize.caffemodel_size) / (1024 * 1024)));
//        LOG_INFO("The neural network is being initialized...");
//        int ret = rknn_init(&rk_ctx_, model->caffemodelBuffer, model->modelsize.caffemodel_size, 0);
////        LOG_INFO("RKNN Init ok.");
//        if (ret < 0) {
//            LOG_ERROR("rknn_init fail! ret={}", ret);
//            return -1;
//        }
//
//        return init_();
//    }

    int Initialize(const unsigned char* model_data, const unsigned int model_size) {
        /* Create the neural network */
        LOG_INFO("The neural network is being initialized...");
        int ret = rknn_init(&rk_ctx_, (void *) model_data, model_size, 0);

        if (ret < 0) {
            LOG_ERROR("rknn_init fail! ret={}", ret);
            return -1;
        }

        return init_();
    }

    /**
     * @brief 获取输入图像Tensor的尺寸
     * @details 用于获取rknn模型的输入尺寸，提取预知输入大小方便进行输入的前处理操作
     * @param index 输入层的索引号
     * @return 返回由各个尺寸组成的维度信息
     */
    std::vector<int> GetInputTensorSize(const int &index) {
        std::vector<int> dims(input_attrs_[index].dims,
                              input_attrs_[index].dims +
                              input_attrs_[index].n_dims);
        return dims;
    }

    /**
     * @brief 获取s输出图像Tensor的尺寸
     * @details 用于获取rknn模型的输出尺寸
     * @param index 输出层的索引号
     * @return 返回由各个尺寸组成的维度信息
     */
    std::vector<unsigned long> GetOutputTensorSize(const int &index) {
//        std::cout << "output_attrs_[index].n_dims:" << output_attrs_[index].n_dims << std::endl;
        std::vector<unsigned long> dims(output_attrs_[index].dims,
                                        output_attrs_[index].dims +
                                        output_attrs_[index].n_dims);
        return dims;
    }

    /**
     * @brief 获取输出Tensor的长度
     * @details 用于获取rknn模型的输出的长度信息
     * @param index 输出层的索引号
     * @return 长度信息
     */
    int GetOutputTensorLen(const int &index) {
        std::vector<unsigned long> tensor_size_out = GetOutputTensorSize(index);
        int size = 1;
        for (auto &one: tensor_size_out) size *= one;
        return size;
    }

    /**
     * @brief 设置输入层的数据流
     * @details 将图像输入喂入输入层中，是执行推理的前一步骤
     * @param index 输入层索引号
     * @param data 图像数据, 采用opencv的mat类型
     * @return 返回输入状态
     */
    Status SetInputData(const int index, const cv::Mat &data) {
        if (data.type() != CV_8UC3) {
            LOG_ERROR("error: input data required CV_8UC3");
        }
        if (index < input_tensors_.size()) {
            input_tensors_[index].index = 0;
            input_tensors_[index].type = RKNN_TENSOR_UINT8;
            input_tensors_[index].size = data.cols * data.rows * data.channels();
            input_tensors_[index].fmt = RKNN_TENSOR_NHWC;
            input_tensors_[index].buf = data.data;
            input_tensors_[index].pass_through = 0;
        } else {
            LOG_ERROR("error: assert index < len");
        }
        return SUCCESS;
    }

    /**
     * @brief 执行神经网络推理
     * @details 需要完成输入数据到输入层后才能执行该步骤, 该步骤为耗时操作
     * @return 返回推理状态结果
     */
    int RunModel() {
        LOG_TRACE("set input");
        int ret = rknn_inputs_set(rk_ctx_, rk_io_num_.n_input, input_tensors_.data());
        if (ret < 0)
            LOG_ERROR("rknn_run fail! ret={}", ret);

        for (int i = 0; i < rk_io_num_.n_output; i++) {
            output_tensors_[i].want_float = outputs_want_float_;
        }

        LOG_TRACE("rknn_run");
        ret = rknn_run(rk_ctx_, nullptr);
        if (ret < 0) {
            LOG_ERROR("rknn_run fail! ret={}", ret);
            return -1;
        }

        ret = rknn_outputs_get(rk_ctx_, rk_io_num_.n_output, output_tensors_.data(), NULL);
        if (ret < 0) {
            LOG_ERROR("rknn_run fail! ret={}", ret);
            exit(0);
        }
        return ret;
    }

    /**
     * @brief 获取输出层的数据
     * @details 返回推理结束后输出层数据，需要先执行推理才能获取
     * @param index 输出层索引
     * @return 返回输出数据的指针
     */
    const float *GetOutputData(const int index) {
        return (float *) (output_tensors_[index].buf);
    }

    /**
     * @brief 获取输出层的数据（UINT8）
     * @details 返回推理结束后输出层UInt8格式的数据，需要先执行推理才能获取
     * @param index 输出层索引
     * @return 返回输出数据的指针
     */
    u_int8_t *GetOutputDataU8(const int index) {
        return (uint8_t *) (output_tensors_[index].buf);
    }

    /**
     * @brief 可变长数据输入操作
     * @details 暂时还未支持该功能
     * @param index_name 输入层索引
     * @param shape 需要改变的维度
     */
    void ResizeInputTensor(const std::string &index_name,
                           const std::vector<int> &shape) {
        // No implementation
    }

    /**
     * @brief 检测尺寸
     * @details 暂时未实现该功能
     * */
    void CheckSize() {
        // No implementation
    }

    /**
     * @brief 获取输出层的数量
     * @details 获取输出层的数量, 通常在多任务多输出神经网络使用
     * @return 返回数量
     */
    size_t GetOutputsNum() const {
        return rk_io_num_.n_output;
    }

    /**
     * @brief 返回输出层的所有Tensor
     * @details 将输出层所有的Tensor进行获取
     * @return 返回所有Tensor
     */
    std::vector<rknn_output> &GetOutputTensors() {
        return output_tensors_;
    }

    /**
     * @brief 返回输出层的所有Tensor节点信息
     * @details 节点信息包含输出尺寸、类型等其他信息
     * @return 返回信息
     */
    std::vector<rknn_tensor_attr> &GetOutputTensorAttr() {
        return output_attrs_;
    }

    /**
     * @brief 析构函数
     */
    ~RKNNAdapter() {
        Release();
    }

    /**
     * @brief 释放资源
     * @details 释放掉所有内存中的资源，通常在析构函数下进行
     */
    void Release() {
        rknn_destroy(rk_ctx_);
        if (model_data) {
            free(model_data);
        }
    }

    /**
     * @brief 设置输出模式是否需要支持浮点格式
     * @details 根据编码风格选定，有些后处理会使用UInt8类型的格式进编解码
     * @param outputsWantFloat 0或1
     */
    void setOutputsWantFloat(int outputsWantFloat) {
        outputs_want_float_ = outputsWantFloat;
    }

private:
    /**
     * 初始化
     * @return
     */
    int init_() {
        rknn_sdk_version version;
        int ret = rknn_query(rk_ctx_, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
        if (ret < 0) {
            LOG_ERROR("rknn_init error ret={}", ret);
            return -1;
        }
        LOG_INFO("sdk version: {} driver version: {}", version.api_version, version.drv_version);

        ret = rknn_query(rk_ctx_, RKNN_QUERY_IN_OUT_NUM, &rk_io_num_,
                         sizeof(rk_io_num_));

        if (ret != RKNN_SUCC) {
            LOG_ERROR("rknn_query ctx fail! ret={}", ret);
            return -1;
        }

        LOG_TRACE("models input num: {}, output num: {}", rk_io_num_.n_input, rk_io_num_.n_output);


//        spdlog::trace("input tensors: ");
        input_attrs_.resize(rk_io_num_.n_input);
        output_attrs_.resize(rk_io_num_.n_output);
        input_tensors_.resize(rk_io_num_.n_input);
        output_tensors_.resize(rk_io_num_.n_output);

        for (int i = 0; i < rk_io_num_.n_input; ++i) {
            memset(&input_attrs_[i], 0, sizeof(input_attrs_[i]));
            memset(&input_tensors_[i], 0, sizeof(input_tensors_[i]));
            input_attrs_[i].index = i;
            ret = rknn_query(rk_ctx_, RKNN_QUERY_INPUT_ATTR, &(input_attrs_[i]),
                             sizeof(rknn_tensor_attr));

            LOG_TRACE("input node index {}", i);
            int channel = 3;
            int width = 0;
            int height = 0;
            if (input_attrs_[i].fmt == RKNN_TENSOR_NCHW) {
                LOG_TRACE("models is NCHW input fmt");
                width = input_attrs_[i].dims[0];
                height = input_attrs_[i].dims[1];
            } else {
                LOG_TRACE("models is NHWC input fmt");
                width = input_attrs_[i].dims[1];
                height = input_attrs_[i].dims[2];
            }
            LOG_TRACE("models input height={}, width={}, channel={}", height, width, channel);
//            print_tensor_attr_(input_attrs_);
            if (ret != RKNN_SUCC) {
                LOG_ERROR("rknn_query fail! ret={}", ret);
                return -1;
            }
        }

//        printf("[debug]models input num: %d, output num: %d\n", rk_io_num_.n_input, rk_io_num_.n_output);
        for (int i = 0; i < rk_io_num_.n_output; ++i) {
            memset(&output_attrs_[i], 0, sizeof(output_attrs_[i]));
            memset(&output_tensors_[i], 0, sizeof(output_tensors_[i]));
            output_attrs_[i].index = i;
            ret = rknn_query(rk_ctx_, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs_[i]),
                             sizeof(rknn_tensor_attr));

            if (output_attrs_[i].qnt_type != RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC ||
                output_attrs_[i].type != RKNN_TENSOR_UINT8) {
                LOG_WARN("The Demo required for a Affine asymmetric u8 quantized rknn models, but output quant type is {}, output "
                        "data type is {}",
                        get_qnt_type_string_(output_attrs_[i].qnt_type), get_type_string_(output_attrs_[i].type));
//                return -1;
            }
//            print_tensor_attr_(output_attrs_[i]);


//            rknn_tensor_attr rknn_attr;
//            memset(&rknn_attr, 0, sizeof(rknn_tensor_attr));
//
//            ret = rknn_query(rk_ctx_, RKNN_QUERY_OUTPUT_ATTR, &rknn_attr,
//                             sizeof(rknn_tensor_attr));
//            printf("output node index %d \n", i);
//            print_tensor_attr_(rknn_attr);


            if (ret != RKNN_SUCC) {
                LOG_ERROR("rknn_query fail! ret={}", ret);
                return -1;
            }
        }

        return ret;
    }

private:
    rknn_context rk_ctx_;                               // rknn的上下文管理器
    rknn_input_output_num rk_io_num_;                   // rkn的输入输出流数量

    std::vector<rknn_tensor_attr> input_attrs_;         // 输入属性
    std::vector<rknn_tensor_attr> output_attrs_;        // 输出属性
    std::vector<rknn_input> input_tensors_;             // 输入数据
    std::vector<rknn_output> output_tensors_;           // 输出数据

    int outputs_want_float_ = 0;                        // 支持浮点输出

    std::vector<int> tensor_shape_;                     // 输入形状
    int width_;                                         // 输入宽
    int height_;                                        // 输入高
    bool run_status_;                                   // 执行状态


    unsigned char *model_data;                          // 模型数据流
};

} // stsrc

#endif //MAGIC_GESTURES_RKNN_ADAPTER_H
