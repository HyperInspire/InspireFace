//
// Created by Tunm-Air13 on 2023/11/3.
//

#ifndef SLEEPMONITORING_RKNN_ADAPTER_NANO_H
#define SLEEPMONITORING_RKNN_ADAPTER_NANO_H

#include "log.h"
#include "memory"
#include <cstdio>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "rknn_api.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <limits>
#include <vector>
#include "log.h"

inline std::vector<float> softmax(const std::vector<float> &input) {
    std::vector<float> output;
    output.reserve(input.size());

    if (input.empty()) {
        return output;
    }

    float max = *std::max_element(input.begin(), input.end());
    float sum = 0.0;

    for (float val : input) {
        sum += std::exp(val - max);
    }

    for (float val : input) {
        output.push_back(std::exp(val - max) / sum);
    }

    return output;
}

inline int NC1HWC2_int8_to_NCHW_float(const int8_t *src, float *dst, int *dims, int channel, int h, int w, int zp, float scale) {
    int batch = dims[0];
    int C1 = dims[1];
    int C2 = dims[4];
    int hw_src = dims[2] * dims[3];
    int hw_dst = h * w;
    for (int i = 0; i < batch; i++) {
        const int8_t *batch_src = src + static_cast<size_t>(i) * C1 * hw_src * C2;
        float *batch_dst = dst + static_cast<size_t>(i) * channel * hw_dst;
        for (int c = 0; c < channel; ++c) {
            int plane = c / C2;
            const int8_t *src_c = plane * hw_src * C2 + batch_src;
            int offset = c % C2;
            for (int cur_h = 0; cur_h < h; ++cur_h)
                for (int cur_w = 0; cur_w < w; ++cur_w) {
                    int cur_hw = cur_h * w + cur_w;
                    batch_dst[c * hw_dst + cur_h * w + cur_w] = (src_c[C2 * cur_hw + offset] - zp) * scale;  // int8-->float
                }
        }
    }

    return 0;
}

static void dump_tensor_attr(rknn_tensor_attr *attr) {
    if (attr == nullptr) {
        return;
    }
    char dims[128] = {0};
    const uint32_t dimension_count = std::min<uint32_t>(attr->n_dims, RKNN_MAX_DIMS);
    size_t used = 0;
    for (uint32_t i = 0; i < dimension_count && used < sizeof(dims); ++i) {
        const int written = std::snprintf(dims + used, sizeof(dims) - used, "%u%s", attr->dims[i], (i + 1 == dimension_count) ? "" : ", ");
        if (written < 0 || static_cast<size_t>(written) >= sizeof(dims) - used) {
            dims[sizeof(dims) - 1] = '\0';
            break;
        }
        used += static_cast<size_t>(written);
    }
    INSPIRE_LOGD(
      "  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
      "zp=%d, scale=%f",
      attr->index, attr->name, attr->n_dims, dims, attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
      get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

class RKNNAdapterNano {
public:
    RKNNAdapterNano(const RKNNAdapterNano &) = delete;
    RKNNAdapterNano &operator=(const RKNNAdapterNano &) = delete;
    RKNNAdapterNano() = default;

    int32_t Initialize(void *model_data, unsigned int model_size) {
        Release();
        if (model_data == nullptr || model_size == 0) {
            INSPIRE_LOGE("RKNN model buffer is empty");
            return -1;
        }
        int ret = rknn_init(&m_rk_ctx_, model_data, model_size, 0, NULL);
        if (ret < 0) {
            INSPIRE_LOGE("rknn_init fail! ret = %d", ret);
            Release();
            return -1;
        }

        // Get sdk and driver version
        rknn_sdk_version sdk_ver{};
        ret = rknn_query(m_rk_ctx_, RKNN_QUERY_SDK_VERSION, &sdk_ver, sizeof(sdk_ver));
        if (ret != RKNN_SUCC) {
            INSPIRE_LOGE("rknn_query fail! ret = %d", ret);
            Release();
            return -1;
        }
        INSPIRE_LOGD("rknn_api/rknnrt version: %s, driver version: %s", sdk_ver.api_version, sdk_ver.drv_version);

        // Get Model Input Output Info
        ret = rknn_query(m_rk_ctx_, RKNN_QUERY_IN_OUT_NUM, &m_rk_io_num_, sizeof(m_rk_io_num_));
        if (ret != RKNN_SUCC || m_rk_io_num_.n_input == 0 || m_rk_io_num_.n_output == 0) {
            INSPIRE_LOGE("rknn_query fail! ret = %d", ret);
            Release();
            return -1;
        }
        INSPIRE_LOGD("model input num: %d, output num: %d", m_rk_io_num_.n_input, m_rk_io_num_.n_output);

        INSPIRE_LOGD("input tensors:");
        m_input_attrs_.resize(m_rk_io_num_.n_input);
        for (uint32_t i = 0; i < m_rk_io_num_.n_input; i++) {
            memset(&m_input_attrs_[i], 0, sizeof(m_input_attrs_[i]));
            m_input_attrs_[i].index = i;
            // query info
            ret = rknn_query(m_rk_ctx_, RKNN_QUERY_INPUT_ATTR, &(m_input_attrs_[i]), sizeof(rknn_tensor_attr));
            if (ret < 0 || !IsValidInputAttr(m_input_attrs_[i])) {
                INSPIRE_LOGE("rknn_init error! ret = %d", ret);
                Release();
                return -1;
            }
            dump_tensor_attr(&m_input_attrs_[i]);

            // Keep the queried attribute as the model's semantic contract.  RKNN
            // conversion needs a separate uint8/NHWC binding descriptor when the
            // queried model input is int8, so never mutate the queried copy.
            m_input_binding_attrs_.push_back(m_input_attrs_[i]);
#if defined(ISF_RKNPU_RV1126B)
            m_input_binding_attrs_.back().type = RKNN_TENSOR_UINT8;
            m_input_binding_attrs_.back().fmt = RKNN_TENSOR_NHWC;
            m_input_binding_attrs_.back().pass_through = 0;
#endif
        }

        INSPIRE_LOGD("output tensors:");
        m_output_attrs_.resize(m_rk_io_num_.n_output);
        for (uint32_t i = 0; i < m_rk_io_num_.n_output; i++) {
            memset(&m_output_attrs_[i], 0, sizeof(m_output_attrs_[i]));
            m_output_attrs_[i].index = i;
            // query info
            ret = rknn_query(m_rk_ctx_, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(m_output_attrs_[i]), sizeof(rknn_tensor_attr));
            if (ret != RKNN_SUCC || !IsValidOutputAttr(m_output_attrs_[i])) {
                INSPIRE_LOGE("rknn_query fail! ret = %d", ret);
                Release();
                return -1;
            }
            dump_tensor_attr(&m_output_attrs_[i]);
        }

        // Get custom string
        rknn_custom_string custom_string{};
        ret = rknn_query(m_rk_ctx_, RKNN_QUERY_CUSTOM_STRING, &custom_string, sizeof(custom_string));
        if (ret != RKNN_SUCC) {
            INSPIRE_LOGE("rknn_query fail! ret = %d", ret);
            Release();
            return -1;
        }
        INSPIRE_LOGD("custom string: %s", custom_string.string);

        // Create input tensor memory
        m_input_mems_.resize(m_rk_io_num_.n_input);
        for (uint32_t i = 0; i < m_rk_io_num_.n_input; ++i) {
            m_input_mems_[i] = rknn_create_mem(m_rk_ctx_, m_input_attrs_[i].size_with_stride);
            if (m_input_mems_[i] == nullptr || m_input_mems_[i]->virt_addr == nullptr ||
                m_input_mems_[i]->size < m_input_attrs_[i].size_with_stride) {
                INSPIRE_LOGE("Failed to allocate RKNN input memory %u", i);
                Release();
                return -1;
            }
        }

        // Create output tensor memory
        m_output_mems_.resize(m_rk_io_num_.n_output);
        for (uint32_t i = 0; i < m_rk_io_num_.n_output; ++i) {
            m_output_mems_[i] = rknn_create_mem(m_rk_ctx_, m_output_attrs_[i].size_with_stride);
            if (m_output_mems_[i] == nullptr || m_output_mems_[i]->virt_addr == nullptr ||
                m_output_mems_[i]->size < m_output_attrs_[i].size_with_stride) {
                INSPIRE_LOGE("Failed to allocate RKNN output memory %u", i);
                Release();
                return -1;
            }
        }

        INSPIRE_LOGD("output origin tensors:");
        m_orig_output_attrs_.resize(m_rk_io_num_.n_output);
        for (uint32_t i = 0; i < m_rk_io_num_.n_output; i++) {
            memset(&m_orig_output_attrs_[i], 0, sizeof(m_orig_output_attrs_[i]));
            m_orig_output_attrs_[i].index = i;
            // query info
            ret = rknn_query(m_rk_ctx_, RKNN_QUERY_OUTPUT_ATTR, &(m_orig_output_attrs_[i]), sizeof(rknn_tensor_attr));
            if (ret != RKNN_SUCC || !IsValidOutputAttr(m_orig_output_attrs_[i]) ||
                !CanConvertOutput(m_output_attrs_[i], m_orig_output_attrs_[i])) {
                INSPIRE_LOGE("rknn_query fail! ret = %d", ret);
                Release();
                return -1;
            }
            dump_tensor_attr(&m_orig_output_attrs_[i]);
        }

        run_ = true;
        return 0;
    }

    int32_t SetInputData(const int index, const void *data, int width, int height, int channel,
                         rknn_tensor_type type = RKNN_TENSOR_UINT8, rknn_tensor_format format = RKNN_TENSOR_NHWC) {
        if (!run_ || index < 0 || static_cast<size_t>(index) >= m_input_mems_.size() ||
            static_cast<size_t>(index) >= m_input_attrs_.size() || static_cast<size_t>(index) >= m_input_binding_attrs_.size() ||
            data == nullptr || width <= 0 || height <= 0 || channel <= 0
#if defined(ISF_RKNPU_RV1126B)
            || type != RKNN_TENSOR_UINT8 || format != RKNN_TENSOR_NHWC
#else
            || (format != RKNN_TENSOR_NHWC && format != RKNN_TENSOR_NCHW) || TensorTypeBytes(type) == 0
#endif
            ) {
            INSPIRE_LOGE("Invalid RKNN input metadata");
            return -1;
        }

        auto &binding_attr = m_input_binding_attrs_[index];
        const auto &normal_attr = m_input_attrs_[index];
        if (!IsValidInputAttr(normal_attr)
#if defined(ISF_RKNPU_RV1126B)
            || normal_attr.fmt != RKNN_TENSOR_NHWC || normal_attr.type != RKNN_TENSOR_INT8 || normal_attr.n_dims != 4 ||
            normal_attr.dims[0] != 1 || width != static_cast<int>(normal_attr.dims[2]) ||
            height != static_cast<int>(normal_attr.dims[1]) || channel != static_cast<int>(normal_attr.dims[3])
#endif
            ) {
            INSPIRE_LOGE("RKNN input contract does not match storage");
            return -1;
        }
#if !defined(ISF_RKNPU_RV1126B)
        binding_attr.type = type;
        binding_attr.fmt = format;
#endif
        const size_t element_bytes = TensorTypeBytes(type);
        size_t source_row_bytes = 0;
        size_t source_size = 0;
        const uint32_t configured_stride = binding_attr.w_stride;
        const size_t destination_width = configured_stride == 0 ? static_cast<size_t>(width) : configured_stride;
        size_t destination_row_bytes = 0;
        size_t destination_size = 0;
        if (element_bytes == 0 || !CheckedMultiply(static_cast<size_t>(width), static_cast<size_t>(channel), source_row_bytes) ||
            !CheckedMultiply(source_row_bytes, element_bytes, source_row_bytes) ||
            !CheckedMultiply(source_row_bytes, static_cast<size_t>(height), source_size) ||
            !CheckedMultiply(destination_width, static_cast<size_t>(channel), destination_row_bytes) ||
            !CheckedMultiply(destination_row_bytes, element_bytes, destination_row_bytes) ||
            !CheckedMultiply(destination_row_bytes, static_cast<size_t>(height), destination_size) ||
            destination_width < static_cast<size_t>(width) || m_input_mems_[index] == nullptr ||
            m_input_mems_[index]->virt_addr == nullptr || destination_size > m_input_mems_[index]->size ||
            destination_size > binding_attr.size_with_stride) {
            INSPIRE_LOGE("RKNN input dimensions or storage are invalid");
            return -1;
        }

        const auto *source = static_cast<const uint8_t *>(data);
        auto *destination = static_cast<uint8_t *>(m_input_mems_[index]->virt_addr);
        std::memset(destination, 0, m_input_mems_[index]->size);
        if (format == RKNN_TENSOR_NHWC) {
            for (int row = 0; row < height; ++row) {
                std::memcpy(destination + static_cast<size_t>(row) * destination_row_bytes,
                            source + static_cast<size_t>(row) * source_row_bytes, source_row_bytes);
            }
        } else {
            const size_t source_channel_bytes = source_row_bytes / static_cast<size_t>(channel);
            const size_t destination_channel_bytes = destination_row_bytes / static_cast<size_t>(channel);
            for (int plane = 0; plane < channel; ++plane) {
                for (int row = 0; row < height; ++row) {
                    const size_t source_offset = (static_cast<size_t>(plane) * height + row) * source_channel_bytes;
                    const size_t destination_offset = (static_cast<size_t>(plane) * height + row) * destination_channel_bytes;
                    std::memcpy(destination + destination_offset, source + source_offset, source_channel_bytes);
                }
            }
        }

        const auto ret = rknn_set_io_mem(m_rk_ctx_, m_input_mems_[index], &binding_attr);
        if (ret < 0) {
            INSPIRE_LOGE("rknn_set_io_mem fail! ret = %d", ret);
            return -1;
        }

        return 0;
    }

    int32_t RunSession(bool use_raw_output = false) {
        m_output_nchw_.clear();
        if (!run_ || m_output_mems_.size() != m_output_attrs_.size() ||
            m_output_attrs_.size() != m_orig_output_attrs_.size()) {
            return -1;
        }
        // Set output tensor memory
        for (uint32_t i = 0; i < m_rk_io_num_.n_output; ++i) {
            if (m_output_mems_[i] == nullptr || m_output_mems_[i]->virt_addr == nullptr ||
                !CanConvertOutput(m_output_attrs_[i], m_orig_output_attrs_[i])) {
                return -1;
            }
            size_t required_bytes = 0;
            if (!NativeStorageBytes(m_output_attrs_[i], required_bytes) || required_bytes > m_output_mems_[i]->size ||
                required_bytes > m_output_attrs_[i].size_with_stride) {
                return -1;
            }
            // set output memory and attribute
            auto ret = rknn_set_io_mem(m_rk_ctx_, m_output_mems_[i], &m_output_attrs_[i]);
            if (ret < 0) {
                INSPIRE_LOGE("rknn_set_io_mem fail! ret = %d", ret);
                return -1;
            }
        }

        auto ret = rknn_run(m_rk_ctx_, NULL);
        if (ret < 0) {
            printf("rknn run error %d\n", ret);
            return -1;
        }

        if (use_raw_output) {
            std::vector<std::vector<float>> converted_outputs;
            converted_outputs.resize(m_rk_io_num_.n_output);
            for (uint32_t i = 0; i < m_rk_io_num_.n_output; ++i) {
                const size_t num_elements = m_orig_output_attrs_[i].n_elems;
                if (num_elements == 0 || m_output_mems_[i] == nullptr || m_output_mems_[i]->virt_addr == nullptr) {
                    return -1;
                }
                if (!ConvertOutputToNormal(m_output_attrs_[i], m_orig_output_attrs_[i], *m_output_mems_[i], converted_outputs[i])) {
                    return -1;
                }
            }
            m_output_nchw_.swap(converted_outputs);
        }

        return 0;
    }

    std::vector<float> &GetOutputData(size_t index) {
        return m_output_nchw_[index];
    }

    void ClearOutputData() {
        m_output_nchw_.clear();
    }

    rknn_tensor_mem *GetOutputRawData(size_t index) {
        return m_output_mems_[index];
    }

    const rknn_tensor_mem *GetOutputRawData(size_t index) const {
        return index < m_output_mems_.size() ? m_output_mems_[index] : nullptr;
    }

    const std::vector<rknn_tensor_attr> &GetNormalInputAttrs() const {
        return m_input_attrs_;
    }

    const std::vector<rknn_tensor_attr> &GetInputBindingAttrs() const {
        return m_input_binding_attrs_;
    }

    const std::vector<rknn_tensor_attr> &GetNativeOutputAttrs() const {
        return m_output_attrs_;
    }

    const std::vector<rknn_tensor_attr> &GetNormalOutputAttrs() const {
        return m_orig_output_attrs_;
    }

    const float *GetOutputDataPtr(const int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_output_nchw_.size() || m_output_nchw_[index].empty()) {
            return nullptr;
        }
        return m_output_nchw_[index].data();
    }

    std::vector<unsigned long> GetOutputTensorSize(const int &index) {
        if (index < 0 || static_cast<size_t>(index) >= m_output_attrs_.size() ||
            static_cast<size_t>(index) >= m_orig_output_attrs_.size()) {
            return {};
        }
        const auto &attribute = m_orig_output_attrs_[index];
        std::vector<unsigned long> dims(attribute.dims, attribute.dims + attribute.n_dims);
        return dims;
    }

    ~RKNNAdapterNano() {
        Release();
    }

    void Release() {
        for (auto *memory : m_input_mems_) {
            if (memory != nullptr) {
                rknn_destroy_mem(m_rk_ctx_, memory);
            }
        }
        for (auto *memory : m_output_mems_) {
            if (memory != nullptr) {
                rknn_destroy_mem(m_rk_ctx_, memory);
            }
        }
        if (m_rk_ctx_ != 0) {
            rknn_destroy(m_rk_ctx_);
        }
        m_rk_ctx_ = 0;
        m_rk_io_num_ = {};
        m_input_attrs_.clear();
        m_input_binding_attrs_.clear();
        m_output_attrs_.clear();
        m_orig_output_attrs_.clear();
        m_input_mems_.clear();
        m_output_mems_.clear();
        m_output_nchw_.clear();
        run_ = false;
    }

private:
    static bool CheckedMultiply(size_t left, size_t right, size_t &result) {
        if (left != 0 && right > std::numeric_limits<size_t>::max() / left) {
            return false;
        }
        result = left * right;
        return true;
    }

    static size_t TensorTypeBytes(rknn_tensor_type type) {
        switch (type) {
            case RKNN_TENSOR_FLOAT32:
            case RKNN_TENSOR_INT32:
            case RKNN_TENSOR_UINT32:
                return 4;
            case RKNN_TENSOR_FLOAT16:
            case RKNN_TENSOR_INT16:
            case RKNN_TENSOR_UINT16:
                return 2;
            case RKNN_TENSOR_INT8:
            case RKNN_TENSOR_UINT8:
            case RKNN_TENSOR_BOOL:
                return 1;
            case RKNN_TENSOR_INT64:
                return 8;
            default:
                return 0;
        }
    }

    static bool CheckedProduct(const uint32_t *dimensions, uint32_t dimension_count, size_t &result) {
        result = 1;
        if (dimensions == nullptr || dimension_count == 0 || dimension_count > RKNN_MAX_DIMS) {
            return false;
        }
        for (uint32_t index = 0; index < dimension_count; ++index) {
            if (dimensions[index] == 0 || !CheckedMultiply(result, dimensions[index], result)) {
                return false;
            }
        }
        return true;
    }

    static bool IsSupportedOutputType(const rknn_tensor_attr &attribute) {
        if (attribute.type == RKNN_TENSOR_FLOAT32) {
            return attribute.qnt_type == RKNN_TENSOR_QNT_NONE;
        }
        if (attribute.type == RKNN_TENSOR_INT8 || attribute.type == RKNN_TENSOR_UINT8) {
            if (attribute.qnt_type != RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC || !std::isfinite(attribute.scale)) {
                return false;
            }
            return attribute.type == RKNN_TENSOR_INT8 ? attribute.zp >= -128 && attribute.zp <= 127
                                                      : attribute.zp >= 0 && attribute.zp <= 255;
        }
        return false;
    }

    static bool IsValidTensorAttr(const rknn_tensor_attr &attribute) {
        const size_t element_bytes = TensorTypeBytes(attribute.type);
        size_t logical_elements = 0;
        size_t logical_bytes = 0;
        return element_bytes != 0 && attribute.n_elems != 0 && attribute.size != 0 && attribute.size_with_stride != 0 &&
               CheckedProduct(attribute.dims, attribute.n_dims, logical_elements) && logical_elements == attribute.n_elems &&
               CheckedMultiply(logical_elements, element_bytes, logical_bytes) && logical_bytes <= attribute.size &&
               attribute.size <= attribute.size_with_stride;
    }

    static bool IsValidInputAttr(const rknn_tensor_attr &attribute) {
        if (!IsValidTensorAttr(attribute) || attribute.n_dims != 4 ||
            (attribute.fmt != RKNN_TENSOR_NHWC && attribute.fmt != RKNN_TENSOR_NCHW)) {
            return false;
        }
        const uint32_t width_index = attribute.fmt == RKNN_TENSOR_NHWC ? 2 : 3;
        if (attribute.w_stride != 0 && attribute.w_stride < attribute.dims[width_index]) {
            return false;
        }
        const size_t stride = attribute.w_stride == 0 ? attribute.dims[width_index] : attribute.w_stride;
        size_t rows = 0;
        size_t row_elements = 0;
        size_t storage_elements = 0;
        size_t storage_bytes = 0;
        const size_t row_count_per_batch = attribute.fmt == RKNN_TENSOR_NHWC
                                             ? attribute.dims[1]
                                             : static_cast<size_t>(attribute.dims[1]) * attribute.dims[2];
        const size_t row_channels = attribute.fmt == RKNN_TENSOR_NHWC ? attribute.dims[3] : 1;
        return CheckedMultiply(attribute.dims[0], row_count_per_batch, rows) &&
               CheckedMultiply(stride, row_channels, row_elements) &&
               CheckedMultiply(rows, row_elements, storage_elements) &&
               CheckedMultiply(storage_elements, TensorTypeBytes(attribute.type), storage_bytes) &&
               storage_bytes <= attribute.size_with_stride;
    }

    static bool IsValidOutputAttr(const rknn_tensor_attr &attribute) {
        if (!IsValidTensorAttr(attribute) || !IsSupportedOutputType(attribute)) {
            return false;
        }
        if (attribute.fmt == RKNN_TENSOR_NC1HWC2) {
            return attribute.n_dims == 5 && (attribute.w_stride == 0 || attribute.w_stride >= attribute.dims[3]);
        }
        if (attribute.fmt == RKNN_TENSOR_NHWC || attribute.fmt == RKNN_TENSOR_NCHW) {
            return attribute.n_dims == 4 && (attribute.w_stride == 0 ||
                                              attribute.w_stride >= attribute.dims[attribute.fmt == RKNN_TENSOR_NHWC ? 2 : 3]);
        }
        return attribute.w_stride == 0;
    }

    static bool LogicalNchwShape(const rknn_tensor_attr &attribute, size_t &batch, size_t &channel, size_t &height, size_t &width) {
        if (attribute.n_dims != 4 || (attribute.fmt != RKNN_TENSOR_NHWC && attribute.fmt != RKNN_TENSOR_NCHW)) {
            return false;
        }
        batch = attribute.dims[0];
        if (attribute.fmt == RKNN_TENSOR_NHWC) {
            height = attribute.dims[1];
            width = attribute.dims[2];
            channel = attribute.dims[3];
        } else {
            channel = attribute.dims[1];
            height = attribute.dims[2];
            width = attribute.dims[3];
        }
        return true;
    }

    static bool CanConvertOutput(const rknn_tensor_attr &native, const rknn_tensor_attr &normal) {
        if (!IsValidOutputAttr(native) || !IsValidOutputAttr(normal)) {
            return false;
        }
        size_t normal_batch = 0, normal_channel = 0, normal_height = 0, normal_width = 0;
        if (native.fmt == RKNN_TENSOR_NC1HWC2) {
            return LogicalNchwShape(normal, normal_batch, normal_channel, normal_height, normal_width) &&
                   native.dims[0] == normal_batch && native.dims[2] == normal_height && native.dims[3] == normal_width &&
                   normal_channel <= static_cast<size_t>(native.dims[1]) * native.dims[4];
        }
        size_t native_batch = 0, native_channel = 0, native_height = 0, native_width = 0;
        if (LogicalNchwShape(native, native_batch, native_channel, native_height, native_width) &&
            LogicalNchwShape(normal, normal_batch, normal_channel, normal_height, normal_width)) {
            return native_batch == normal_batch && native_channel == normal_channel && native_height == normal_height &&
                   native_width == normal_width;
        }
        if (native.fmt != normal.fmt || native.n_dims != normal.n_dims || native.n_elems != normal.n_elems) {
            return false;
        }
        return std::equal(native.dims, native.dims + native.n_dims, normal.dims);
    }

    static bool NativeStorageBytes(const rknn_tensor_attr &attribute, size_t &bytes) {
        size_t storage_elements = attribute.n_elems;
        size_t stride = 0;
        if (attribute.fmt == RKNN_TENSOR_NHWC && attribute.n_dims == 4) {
            stride = attribute.w_stride == 0 ? attribute.dims[2] : attribute.w_stride;
            if (!CheckedMultiply(attribute.dims[0], attribute.dims[1], storage_elements) ||
                !CheckedMultiply(storage_elements, stride, storage_elements) ||
                !CheckedMultiply(storage_elements, attribute.dims[3], storage_elements)) {
                return false;
            }
        } else if (attribute.fmt == RKNN_TENSOR_NCHW && attribute.n_dims == 4) {
            stride = attribute.w_stride == 0 ? attribute.dims[3] : attribute.w_stride;
            if (!CheckedMultiply(attribute.dims[0], attribute.dims[1], storage_elements) ||
                !CheckedMultiply(storage_elements, attribute.dims[2], storage_elements) ||
                !CheckedMultiply(storage_elements, stride, storage_elements)) {
                return false;
            }
        } else if (attribute.fmt == RKNN_TENSOR_NC1HWC2 && attribute.n_dims == 5) {
            stride = attribute.w_stride == 0 ? attribute.dims[3] : attribute.w_stride;
            if (!CheckedMultiply(attribute.dims[0], attribute.dims[1], storage_elements) ||
                !CheckedMultiply(storage_elements, attribute.dims[2], storage_elements) ||
                !CheckedMultiply(storage_elements, stride, storage_elements) ||
                !CheckedMultiply(storage_elements, attribute.dims[4], storage_elements)) {
                return false;
            }
        }
        return CheckedMultiply(storage_elements, TensorTypeBytes(attribute.type), bytes);
    }

    static bool ReadNativeFloat(const rknn_tensor_attr &attribute, const uint8_t *source, size_t index, float &value) {
        if (source == nullptr) {
            return false;
        }
        if (attribute.type == RKNN_TENSOR_FLOAT32) {
            std::memcpy(&value, source + index * sizeof(float), sizeof(float));
        } else if (attribute.type == RKNN_TENSOR_INT8) {
            value = (static_cast<int32_t>(reinterpret_cast<const int8_t *>(source)[index]) - attribute.zp) * attribute.scale;
        } else if (attribute.type == RKNN_TENSOR_UINT8) {
            value = (static_cast<int32_t>(source[index]) - attribute.zp) * attribute.scale;
        } else {
            return false;
        }
        return std::isfinite(value);
    }

    static bool ConvertOutputToNormal(const rknn_tensor_attr &native, const rknn_tensor_attr &normal,
                                      const rknn_tensor_mem &memory, std::vector<float> &destination) {
        size_t storage_bytes = 0;
        if (!CanConvertOutput(native, normal) || memory.virt_addr == nullptr || !NativeStorageBytes(native, storage_bytes) ||
            storage_bytes > memory.size || storage_bytes > native.size_with_stride) {
            return false;
        }
        destination.assign(normal.n_elems, 0.0f);
        const auto *source = static_cast<const uint8_t *>(memory.virt_addr);
        size_t batch = 0, channel = 0, height = 0, width = 0;
        if (!LogicalNchwShape(normal, batch, channel, height, width)) {
            if (native.fmt != normal.fmt || native.n_elems != normal.n_elems) {
                return false;
            }
            for (size_t index = 0; index < destination.size(); ++index) {
                if (!ReadNativeFloat(native, source, index, destination[index])) {
                    return false;
                }
            }
            return true;
        }
        const size_t native_width = native.fmt == RKNN_TENSOR_NHWC ? native.dims[2] : native.dims[3];
        const size_t native_stride = native.w_stride == 0 ? native_width : native.w_stride;
        for (size_t n = 0; n < batch; ++n) {
            for (size_t c = 0; c < channel; ++c) {
                for (size_t h = 0; h < height; ++h) {
                    for (size_t w = 0; w < width; ++w) {
                        size_t source_index = 0;
                        if (native.fmt == RKNN_TENSOR_NHWC) {
                            source_index = ((n * height + h) * native_stride + w) * channel + c;
                        } else if (native.fmt == RKNN_TENSOR_NCHW) {
                            source_index = ((n * channel + c) * height + h) * native_stride + w;
                        } else if (native.fmt == RKNN_TENSOR_NC1HWC2) {
                            source_index = (((n * native.dims[1] + c / native.dims[4]) * height + h) * native_stride + w) *
                                             native.dims[4] + c % native.dims[4];
                        } else {
                            return false;
                        }
                        const size_t destination_index = normal.fmt == RKNN_TENSOR_NHWC
                                                           ? ((n * height + h) * width + w) * channel + c
                                                           : ((n * channel + c) * height + h) * width + w;
                        if (!ReadNativeFloat(native, source, source_index, destination[destination_index])) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    rknn_context m_rk_ctx_{};

    rknn_input_output_num m_rk_io_num_{};
    // Queried normal attributes are immutable semantic contracts.  Binding and
    // native attributes exist solely for RKNN memory registration/storage.
    std::vector<rknn_tensor_attr> m_input_attrs_;
    std::vector<rknn_tensor_attr> m_input_binding_attrs_;
    std::vector<rknn_tensor_attr> m_output_attrs_;
    std::vector<rknn_tensor_attr> m_orig_output_attrs_;

    std::vector<rknn_tensor_mem *> m_input_mems_;
    std::vector<rknn_tensor_mem *> m_output_mems_;

    std::vector<std::vector<float>> m_output_nchw_;
    bool run_{false};
};

#endif  // SLEEPMONITORING_RKNN_ADAPTER_NANO_H
