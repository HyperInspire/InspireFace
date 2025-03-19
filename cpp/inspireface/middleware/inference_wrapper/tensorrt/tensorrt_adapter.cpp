/**
 * Created by Claude
 * @date 2025-03-18
 */
#include "tensorrt_adapter.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <memory>
#include <cstring>      // for std::memcpy
#include <cuda_fp16.h>  // for half type support
#include <NvInfer.h>
#include <cuda_runtime_api.h>

// define specific deleters for TensorRT objects
struct TRTRuntimeDeleter {
    void operator()(nvinfer1::IRuntime *runtime) const {
        if (runtime)
            delete runtime;
    }
};

struct TRTEngineDeleter {
    void operator()(nvinfer1::ICudaEngine *engine) const {
        if (engine)
            delete engine;
    }
};

struct TRTContextDeleter {
    void operator()(nvinfer1::IExecutionContext *context) const {
        if (context)
            delete context;
    }
};

// custom deleter for CUDA stream
struct CUDAStreamDeleter {
    void operator()(cudaStream_t *stream) const {
        if (stream) {
            cudaStreamDestroy(*stream);
            delete stream;
        }
    }
};

// custom Logger class, inherit from TensorRT's ILogger
class TRTLogger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char *msg) noexcept override {
        if (severity <= Severity::kWARNING) {
            std::cout << "[TensorRT] " << msg << std::endl;
        }
    }
};

// read model file to memory
static std::vector<char> readModelFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "failed to open model file: " << filename << std::endl;
        return {};
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "failed to read model file" << std::endl;
        return {};
    }

    return buffer;
}

// CUDA error check macro
#define CHECK_CUDA(call)                                                           \
    do {                                                                           \
        cudaError_t error = call;                                                  \
        if (error != cudaSuccess) {                                                \
            std::cerr << "CUDA error: " << cudaGetErrorString(error) << std::endl; \
            return TENSORRT_HFAIL;                                                 \
        }                                                                          \
    } while (0)

// TensorRT adapter implementation class
class TensorRTAdapter::Impl {
public:
    Impl() : m_ownStream(false), m_inferenceMode(TensorRTAdapter::InferenceMode::FP32) {
        // create Logger with smart pointer
        m_logger = std::make_unique<TRTLogger>();
    }

    ~Impl() {
        // release resources - device memory needs to be released manually
        for (auto &pair : m_deviceBuffers) {
            if (pair.second) {
                cudaFree(pair.second);
            }
        }
        m_deviceBuffers.clear();

        // smart pointers will handle the release of other resources
    }

    int32_t readFromFile(const std::string &enginePath) {
        // read serialized engine file
        std::vector<char> modelData = readModelFile(enginePath);
        if (modelData.empty()) {
            return TENSORRT_HFAIL;
        }

        return deserializeEngine(modelData);
    }

    int32_t readFromBin(const std::vector<char> &model_data) {
        if (model_data.empty()) {
            return TENSORRT_HFAIL;
        }

        return deserializeEngine(model_data);
    }

    int32_t readFromBin(void *model_data, unsigned int model_size) {
        if (!model_data || model_size == 0) {
            std::cerr << "invalid model data or size" << std::endl;
            return TENSORRT_HFAIL;
        }

        // convert memory data to vector to reuse the existing deserializeEngine method
        std::vector<char> modelBuffer(static_cast<char *>(model_data), static_cast<char *>(model_data) + model_size);

        return deserializeEngine(modelBuffer);
    }

    // create and deserialize engine
    int32_t deserializeEngine(const std::vector<char> &modelData) {
        // create runtime
        m_runtime.reset(nvinfer1::createInferRuntime(*m_logger));
        if (!m_runtime) {
            std::cerr << "failed to create TensorRT runtime" << std::endl;
            return TENSORRT_HFAIL;
        }

        // deserialize engine
        m_engine.reset(m_runtime->deserializeCudaEngine(modelData.data(), modelData.size()));
        if (!m_engine) {
            std::cerr << "failed to deserialize engine" << std::endl;
            return TENSORRT_HFAIL;
        }

        // create execution context
        m_context.reset(m_engine->createExecutionContext());
        if (!m_context) {
            std::cerr << "failed to create execution context" << std::endl;
            return TENSORRT_HFAIL;
        }

        // get all input and output tensor names
        int numIoTensors = m_engine->getNbIOTensors();
        for (int i = 0; i < numIoTensors; ++i) {
            const char *name = m_engine->getIOTensorName(i);
            nvinfer1::TensorIOMode mode = m_engine->getTensorIOMode(name);
            if (mode == nvinfer1::TensorIOMode::kINPUT) {
                m_inputNames.push_back(name);
            } else {
                m_outputNames.push_back(name);
            }
        }

        // initialize CUDA stream
        if (!m_stream) {
            cudaStream_t *stream = new cudaStream_t;
            CHECK_CUDA(cudaStreamCreate(stream));
            m_stream.reset(stream);
            m_ownStream = true;
        }

        // pre-allocate device memory
        return allocateDeviceMemory();
    }

    // allocate device memory
    int32_t allocateDeviceMemory() {
        // allocate device memory for each input and output tensor
        for (const auto &name : m_inputNames) {
            nvinfer1::Dims dims = m_engine->getTensorShape(name.c_str());
            nvinfer1::DataType dtype = m_engine->getTensorDataType(name.c_str());
            size_t size = getMemorySize(dims, dtype);

            void *buffer = nullptr;
            CHECK_CUDA(cudaMalloc(&buffer, size));
            m_deviceBuffers[name] = buffer;

            // store shape information
            m_inputShapes[name] = dimsToVector(dims);
        }

        for (const auto &name : m_outputNames) {
            nvinfer1::Dims dims = m_engine->getTensorShape(name.c_str());
            nvinfer1::DataType dtype = m_engine->getTensorDataType(name.c_str());
            size_t size = getMemorySize(dims, dtype);

            void *buffer = nullptr;
            CHECK_CUDA(cudaMalloc(&buffer, size));
            m_deviceBuffers[name] = buffer;

            // Save shape information
            m_outputShapes[name] = dimsToVector(dims);
        }

        return TENSORRT_HSUCCEED;
    }

    // set input data
    void setInput(const char *inputName, const void *data) {
        auto it = m_deviceBuffers.find(inputName);
        if (it != m_deviceBuffers.end()) {
            nvinfer1::Dims dims = m_engine->getTensorShape(inputName);
            nvinfer1::DataType dtype = m_engine->getTensorDataType(inputName);
            size_t size = getMemorySize(dims, dtype);

            // copy data from host to device
            cudaMemcpyAsync(it->second, data, size, cudaMemcpyHostToDevice, *m_stream.get());
            cudaStreamSynchronize(*m_stream.get());  // add synchronization to ensure data is fully copied
        }
    }

    // set batch size (only for models with dynamic shapes)
    int32_t setBatchSize(int batchSize) {
        if (m_inputNames.empty())
            return TENSORRT_HFAIL;

        for (const auto &name : m_inputNames) {
            nvinfer1::Dims dims = m_engine->getTensorShape(name.c_str());
            if (dims.nbDims > 0) {
                nvinfer1::Dims newDims = dims;
                newDims.d[0] = batchSize;

                if (!m_context->setInputShape(name.c_str(), newDims)) {
                    std::cerr << "为输入 " << name << " 设置批处理大小失败" << std::endl;
                    return TENSORRT_HFAIL;
                }

                // update shape information
                m_inputShapes[name] = dimsToVector(newDims);
            }
        }

        return TENSORRT_HSUCCEED;
    }

    // forward inference
    int32_t forward() {
        if (!m_context || !m_engine) {
            return TENSORRT_HFAIL;
        }

        // check if all tensors are bound to addresses
        for (const auto &name : m_inputNames) {
            if (!m_context->setTensorAddress(name.c_str(), m_deviceBuffers[name])) {
                std::cerr << "failed to set input tensor " << name << " address" << std::endl;
                return TENSORRT_FORWARD_FAILED;
            }
        }

        for (const auto &name : m_outputNames) {
            if (!m_context->setTensorAddress(name.c_str(), m_deviceBuffers[name])) {
                std::cerr << "failed to set output tensor " << name << " address" << std::endl;
                return TENSORRT_FORWARD_FAILED;
            }
        }

        // record start time - use high precision timing
        auto start = std::chrono::high_resolution_clock::now();

        // forward inference
        bool status = m_context->enqueueV3(*m_stream.get());

        // synchronize CUDA stream
        cudaStreamSynchronize(*m_stream.get());

        // record end time
        auto end = std::chrono::high_resolution_clock::now();

        // calculate duration (microseconds) then convert to milliseconds, keep high precision
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        m_inferenceTime = duration_us.count() / 1000.0;

        return status ? TENSORRT_HSUCCEED : TENSORRT_FORWARD_FAILED;
    }

    // get output data
    const void *getOutput(const char *nodeName) {
        auto it = m_deviceBuffers.find(nodeName);
        if (it != m_deviceBuffers.end()) {
            nvinfer1::Dims dims = m_context->getTensorShape(nodeName);
            nvinfer1::DataType dtype = m_engine->getTensorDataType(nodeName);
            size_t size = getMemorySize(dims, dtype);

            // copy output data from device to host
            if (m_hostOutputBuffers.find(nodeName) == m_hostOutputBuffers.end()) {
                m_hostOutputBuffers[nodeName].resize(size);
            }

            cudaMemcpyAsync(m_hostOutputBuffers[nodeName].data(), it->second, size, cudaMemcpyDeviceToHost, *m_stream.get());
            cudaStreamSynchronize(*m_stream.get());

            return m_hostOutputBuffers[nodeName].data();
        }
        return nullptr;
    }

    // get output data and convert to float type vector
    std::vector<float> getOutputAsFloat(const char *nodeName) {
        std::vector<float> result;
        auto it = m_deviceBuffers.find(nodeName);
        if (it != m_deviceBuffers.end()) {
            nvinfer1::Dims dims = m_context->getTensorShape(nodeName);
            nvinfer1::DataType dtype = m_engine->getTensorDataType(nodeName);

            // calculate total number of elements
            size_t numElements = 1;
            for (int i = 0; i < dims.nbDims; ++i) {
                numElements *= dims.d[i];
            }

            // allocate buffer of appropriate size based on data type
            size_t elementSize = 0;
            switch (dtype) {
                case nvinfer1::DataType::kFLOAT:
                    elementSize = sizeof(float);
                    break;
                case nvinfer1::DataType::kHALF:
                    elementSize = sizeof(half);
                    break;
                case nvinfer1::DataType::kINT8:
                    elementSize = sizeof(int8_t);
                    break;
                case nvinfer1::DataType::kINT32:
                    elementSize = sizeof(int32_t);
                    break;
                default:
                    return result;
            }

            // allocate temporary buffer
            std::vector<unsigned char> buffer(numElements * elementSize);

            // copy data from device memory to host memory
            cudaMemcpyAsync(buffer.data(), it->second, buffer.size(), cudaMemcpyDeviceToHost, *m_stream.get());
            cudaStreamSynchronize(*m_stream.get());

            // convert to float based on data type
            result.resize(numElements);
            switch (dtype) {
                case nvinfer1::DataType::kFLOAT:
                    std::memcpy(result.data(), buffer.data(), buffer.size());
                    break;
                case nvinfer1::DataType::kHALF: {
                    const half *halfData = reinterpret_cast<const half *>(buffer.data());
                    for (size_t i = 0; i < numElements; ++i) {
                        result[i] = __half2float(halfData[i]);
                    }
                    break;
                }
                case nvinfer1::DataType::kINT8: {
                    const int8_t *int8Data = reinterpret_cast<const int8_t *>(buffer.data());
                    for (size_t i = 0; i < numElements; ++i) {
                        result[i] = static_cast<float>(int8Data[i]);
                    }
                    break;
                }
                case nvinfer1::DataType::kINT32: {
                    const int32_t *int32Data = reinterpret_cast<const int32_t *>(buffer.data());
                    for (size_t i = 0; i < numElements; ++i) {
                        result[i] = static_cast<float>(int32Data[i]);
                    }
                    break;
                }
            }
        }
        return result;
    }

    // set inference mode
    void setInferenceMode(TensorRTAdapter::InferenceMode mode) {
        m_inferenceMode = mode;
        // apply this setting during actual inference
    }

    // set CUDA stream
    void setCudaStream(void *streamPtr) {
        if (m_ownStream) {
            m_stream.reset();
            m_ownStream = false;
        }

        // create a new smart pointer instead of using reset + lambda
        cudaStream_t *streamPointer = static_cast<cudaStream_t *>(streamPtr);
        // use empty deleter, because this stream is managed by external code
        m_stream =
          std::unique_ptr<cudaStream_t, CUDAStreamDeleter>(streamPointer,
                                                           CUDAStreamDeleter()  // use default deleter, but not actually delete the external stream
          );
    }

    // print model info
    void printModelInfo() const {
        if (!m_engine) {
            std::cerr << "engine not initialized" << std::endl;
            return;
        }

        std::cout << "\nengine info:" << std::endl;
        std::cout << "engine layers: " << m_engine->getNbLayers() << std::endl;
        std::cout << "input/output tensors: " << m_engine->getNbIOTensors() << std::endl;

        std::cout << "\ninput tensors:" << std::endl;
        for (const auto &name : m_inputNames) {
            nvinfer1::Dims dims = m_engine->getTensorShape(name.c_str());
            nvinfer1::DataType dtype = m_engine->getTensorDataType(name.c_str());

            std::cout << "name: " << name << ", shape: (";
            for (int d = 0; d < dims.nbDims; ++d) {
                std::cout << dims.d[d];
                if (d < dims.nbDims - 1)
                    std::cout << ", ";
            }
            std::cout << "), type: " << getDataTypeString(dtype) << std::endl;
        }

        std::cout << "\noutput tensors:" << std::endl;
        for (const auto &name : m_outputNames) {
            nvinfer1::Dims dims = m_engine->getTensorShape(name.c_str());
            nvinfer1::DataType dtype = m_engine->getTensorDataType(name.c_str());

            std::cout << "name: " << name << ", shape: (";
            for (int d = 0; d < dims.nbDims; ++d) {
                std::cout << dims.d[d];
                if (d < dims.nbDims - 1)
                    std::cout << ", ";
            }
            std::cout << "), type: " << getDataTypeString(dtype) << std::endl;
        }
    }

    // get input tensor names list
    const std::vector<std::string> &getInputNames() const {
        return m_inputNames;
    }

    // get output tensor names list
    const std::vector<std::string> &getOutputNames() const {
        return m_outputNames;
    }

    // get input tensor shape by name
    const std::vector<int> &getInputShapeByName(const std::string &name) const {
        static std::vector<int> emptyShape;
        auto it = m_inputShapes.find(name);
        return (it != m_inputShapes.end()) ? it->second : emptyShape;
    }

    // get output tensor shape by name
    const std::vector<int> &getOutputShapeByName(const std::string &name) const {
        static std::vector<int> emptyShape;
        auto it = m_outputShapes.find(name);
        return (it != m_outputShapes.end()) ? it->second : emptyShape;
    }

    // get inference time
    double getInferenceTime() const {
        return m_inferenceTime;
    }

private:
    // helper function: convert TensorRT's Dims to standard vector
    std::vector<int> dimsToVector(const nvinfer1::Dims &dims) const {
        std::vector<int> shape;
        for (int i = 0; i < dims.nbDims; ++i) {
            shape.push_back(dims.d[i]);
        }
        return shape;
    }

    // helper function: calculate memory size
    size_t getMemorySize(const nvinfer1::Dims &dims, nvinfer1::DataType dtype) const {
        size_t size = 1;
        for (int i = 0; i < dims.nbDims; ++i) {
            size *= dims.d[i];
        }

        switch (dtype) {
            case nvinfer1::DataType::kFLOAT:
                return size * 4;
            case nvinfer1::DataType::kHALF:
                return size * 2;
            case nvinfer1::DataType::kINT8:
                return size;
            case nvinfer1::DataType::kINT32:
                return size * 4;
            case nvinfer1::DataType::kBOOL:
                return size;
            default:
                return size;
        }
    }

    // helper function: get data type string representation
    std::string getDataTypeString(nvinfer1::DataType dtype) const {
        switch (dtype) {
            case nvinfer1::DataType::kFLOAT:
                return "FLOAT";
            case nvinfer1::DataType::kHALF:
                return "HALF";
            case nvinfer1::DataType::kINT8:
                return "INT8";
            case nvinfer1::DataType::kINT32:
                return "INT32";
            case nvinfer1::DataType::kBOOL:
                return "BOOL";
            default:
                return "UNKNOWN";
        }
    }

    // member variables - using smart pointers
    std::unique_ptr<TRTLogger> m_logger;
    std::unique_ptr<nvinfer1::IRuntime, TRTRuntimeDeleter> m_runtime;
    std::unique_ptr<nvinfer1::ICudaEngine, TRTEngineDeleter> m_engine;
    std::unique_ptr<nvinfer1::IExecutionContext, TRTContextDeleter> m_context;

    bool m_ownStream;
    std::unique_ptr<cudaStream_t, CUDAStreamDeleter> m_stream;

    std::vector<std::string> m_inputNames;
    std::vector<std::string> m_outputNames;
    std::map<std::string, void *> m_deviceBuffers;
    std::map<std::string, std::vector<unsigned char>> m_hostOutputBuffers;

    std::map<std::string, std::vector<int>> m_inputShapes;
    std::map<std::string, std::vector<int>> m_outputShapes;

    TensorRTAdapter::InferenceMode m_inferenceMode;
    double m_inferenceTime;
};

// implement TensorRTAdapter methods
TensorRTAdapter::TensorRTAdapter() : pImpl(new Impl()) {}

TensorRTAdapter::~TensorRTAdapter() {
    if (pImpl) {
        delete pImpl;
        pImpl = nullptr;
    }
}

int32_t TensorRTAdapter::readFromFile(const std::string &enginePath) {
    return pImpl->readFromFile(enginePath);
}

int32_t TensorRTAdapter::readFromBin(void *model_data, unsigned int model_size) {
    return pImpl->readFromBin(model_data, model_size);
}

TensorRTAdapter TensorRTAdapter::readNetFrom(const std::string &enginePath) {
    TensorRTAdapter adapter;
    adapter.readFromFile(enginePath);
    return adapter;
}

TensorRTAdapter TensorRTAdapter::readNetFromBin(const std::vector<char> &model_data) {
    TensorRTAdapter adapter;
    adapter.pImpl->readFromBin(model_data);
    return adapter;
}

std::vector<std::string> TensorRTAdapter::getInputNames() const {
    return pImpl->getInputNames();
}

std::vector<std::string> TensorRTAdapter::getOutputNames() const {
    return pImpl->getOutputNames();
}

std::vector<int> TensorRTAdapter::getInputShapeByName(const std::string &name) {
    return pImpl->getInputShapeByName(name);
}

std::vector<int> TensorRTAdapter::getOutputShapeByName(const std::string &name) {
    return pImpl->getOutputShapeByName(name);
}

void TensorRTAdapter::setInput(const char *inputName, const void *data) {
    pImpl->setInput(inputName, data);
}

int32_t TensorRTAdapter::setBatchSize(int batchSize) {
    return pImpl->setBatchSize(batchSize);
}

int32_t TensorRTAdapter::forward() {
    return pImpl->forward();
}

const void *TensorRTAdapter::getOutput(const char *nodeName) {
    return pImpl->getOutput(nodeName);
}

std::vector<float> TensorRTAdapter::getOutputAsFloat(const char *nodeName) {
    return pImpl->getOutputAsFloat(nodeName);
}

double TensorRTAdapter::getInferenceTime() const {
    return pImpl->getInferenceTime();
}

void TensorRTAdapter::setInferenceMode(InferenceMode mode) {
    pImpl->setInferenceMode(mode);
}

void TensorRTAdapter::setCudaStream(void *streamPtr) {
    pImpl->setCudaStream(streamPtr);
}

void TensorRTAdapter::printModelInfo() const {
    pImpl->printModelInfo();
}

// add static method to TensorRTAdapter class
int32_t TensorRTAdapter::printCudaDeviceInfo() {
    try {
        // print TensorRT version
        std::cout << "TensorRT version: " << NV_TENSORRT_MAJOR << "." << NV_TENSORRT_MINOR << "." << NV_TENSORRT_PATCH << std::endl;

        // check if CUDA is available
        int device_count;
        cudaError_t error = cudaGetDeviceCount(&device_count);
        if (error != cudaSuccess) {
            std::cerr << "CUDA error: " << cudaGetErrorString(error) << std::endl;
            return TENSORRT_HFAIL;
        }
        std::cout << "available CUDA devices: " << device_count << std::endl;

        // get current CUDA device
        int currentDevice;
        error = cudaGetDevice(&currentDevice);
        if (error != cudaSuccess) {
            std::cerr << "failed to get current CUDA device: " << cudaGetErrorString(error) << std::endl;
            return TENSORRT_HFAIL;
        }
        std::cout << "current CUDA device ID: " << currentDevice << std::endl;

        // get GPU device properties
        cudaDeviceProp prop;
        error = cudaGetDeviceProperties(&prop, currentDevice);
        if (error != cudaSuccess) {
            std::cerr << "failed to get CUDA device properties: " << cudaGetErrorString(error) << std::endl;
            return TENSORRT_HFAIL;
        }

        // print device detailed information
        std::cout << "\nCUDA device details:" << std::endl;
        std::cout << "device name: " << prop.name << std::endl;
        std::cout << "compute capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "global memory: " << prop.totalGlobalMem / (1024 * 1024) << " MB" << std::endl;
        std::cout << "max shared memory/block: " << prop.sharedMemPerBlock / 1024 << " KB" << std::endl;
        std::cout << "max threads/block: " << prop.maxThreadsPerBlock << std::endl;
        std::cout << "max block dimensions: (" << prop.maxThreadsDim[0] << ", " << prop.maxThreadsDim[1] << ", " << prop.maxThreadsDim[2] << ")"
                  << std::endl;
        std::cout << "max grid size: (" << prop.maxGridSize[0] << ", " << prop.maxGridSize[1] << ", " << prop.maxGridSize[2] << ")" << std::endl;
        std::cout << "total constant memory: " << prop.totalConstMem / 1024 << " KB" << std::endl;
        std::cout << "multi-processor count: " << prop.multiProcessorCount << std::endl;
        std::cout << "max blocks per multi-processor: " << prop.maxBlocksPerMultiProcessor << std::endl;
        std::cout << "clock frequency: " << prop.clockRate / 1000 << " MHz" << std::endl;
        std::cout << "memory frequency: " << prop.memoryClockRate / 1000 << " MHz" << std::endl;
        std::cout << "memory bus width: " << prop.memoryBusWidth << " bits" << std::endl;
        std::cout << "L2 cache size: " << prop.l2CacheSize / 1024 << " KB" << std::endl;
        std::cout << "theoretical memory bandwidth: " << 2.0 * prop.memoryClockRate * (prop.memoryBusWidth / 8) / 1.0e6 << " GB/s" << std::endl;

        // check if FP16 is supported
        bool supportsFP16 = prop.major >= 6 || (prop.major == 5 && prop.minor >= 3);
        std::cout << "FP16 support: " << (supportsFP16 ? "yes" : "no") << std::endl;

        // check if unified memory is supported
        std::cout << "unified memory support: " << (prop.unifiedAddressing ? "yes" : "no") << std::endl;

        // check if concurrent kernel execution is supported
        std::cout << "concurrent kernel execution: " << (prop.concurrentKernels ? "yes" : "no") << std::endl;

        // check if asynchronous engine is supported
        std::cout << "asynchronous engine count: " << prop.asyncEngineCount << std::endl;

        return TENSORRT_HSUCCEED;
    } catch (const std::exception &e) {
        std::cerr << "error when printing CUDA device info: " << e.what() << std::endl;
        return TENSORRT_HFAIL;
    }
}