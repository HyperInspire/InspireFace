#ifdef ISF_ENABLE_TENSORRT
#ifndef INSPIRE_CUDA_TOOLKIT_H
#define INSPIRE_CUDA_TOOLKIT_H
#include <cuda_runtime_api.h>
#include <NvInfer.h>
#include <log.h>
#include "herror.h"

namespace inspire {

inline static int32_t GetCudaDeviceCount(int32_t *device_count) {
    cudaError_t error = cudaGetDeviceCount(device_count);
    if (error != cudaSuccess) {
        INSPIRE_LOGE("CUDA error: %s", cudaGetErrorString(error));
        return HERR_DEVICE_CUDA_UNKNOWN_ERROR;
    }
    return HSUCCEED;
}

inline static int32_t CheckCudaUsability(int32_t *is_support) {
    int device_count = GetCudaDeviceCount();
    if (device_count == 0) {
        *is_support = 1;
        INSPIRE_LOGE("No CUDA devices found");
        return HERR_DEVICE_CUDA_NOT_SUPPORT;
    }
    *is_support = device_count > 0;
    return HSUCCEED;
}

inline static int32_t _PrintCudaDeviceInfo() {
    try {
        INSPIRE_LOGI("TensorRT version: %d.%d.%d", NV_TENSORRT_MAJOR, NV_TENSORRT_MINOR, NV_TENSORRT_PATCH);

        // check if CUDA is available
        int device_count;
        cudaError_t error = cudaGetDeviceCount(&device_count);
        if (error != cudaSuccess) {
            INSPIRE_LOGE("CUDA error: %s", cudaGetErrorString(error));
            return HERR_DEVICE_CUDA_UNKNOWN_ERROR;
        }
        INSPIRE_LOGI("available CUDA devices: %d", device_count);

        // get current CUDA device
        int currentDevice;
        error = cudaGetDevice(&currentDevice);
        if (error != cudaSuccess) {
            INSPIRE_LOGE("[CUDA error] failed to get current CUDA device: %s", cudaGetErrorString(error));
            return HERR_DEVICE_CUDA_UNKNOWN_ERROR;
        }
        INSPIRE_LOGI("current CUDA device ID: %d", currentDevice);

        // get GPU device properties
        cudaDeviceProp prop;
        error = cudaGetDeviceProperties(&prop, currentDevice);
        if (error != cudaSuccess) {
            INSPIRE_LOGE("[CUDA error] failed to get CUDA device properties: %s", cudaGetErrorString(error));
            return HERR_DEVICE_CUDA_UNKNOWN_ERROR;
        }

        // print device detailed information
        INSPIRE_LOGI("\nCUDA device details:");
        INSPIRE_LOGI("device name: %s", prop.name);
        INSPIRE_LOGI("compute capability: %d.%d", prop.major, prop.minor);
        INSPIRE_LOGI("global memory: %d MB", prop.totalGlobalMem / (1024 * 1024));
        INSPIRE_LOGI("max shared memory/block: %d KB", prop.sharedMemPerBlock / 1024);
        INSPIRE_LOGI("max threads/block: %d", prop.maxThreadsPerBlock);
        INSPIRE_LOGI("max block dimensions: (%d, %d, %d)", prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
        INSPIRE_LOGI("max grid size: (%d, %d, %d)", prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
        INSPIRE_LOGI("total constant memory: %d KB", prop.totalConstMem / 1024);
        INSPIRE_LOGI("multi-processor count: %d", prop.multiProcessorCount);
        INSPIRE_LOGI("max blocks per multi-processor: %d", prop.maxBlocksPerMultiProcessor);
        INSPIRE_LOGI("clock frequency: %d MHz", prop.clockRate / 1000);
        INSPIRE_LOGI("memory frequency: %d MHz", prop.memoryClockRate / 1000);
        INSPIRE_LOGI("memory bus width: %d bits", prop.memoryBusWidth);
        INSPIRE_LOGI("L2 cache size: %d KB", prop.l2CacheSize / 1024);
        INSPIRE_LOGI("theoretical memory bandwidth: %f GB/s", 2.0 * prop.memoryClockRate * (prop.memoryBusWidth / 8) / 1.0e6);

        // check if FP16 is supported
        bool supportsFP16 = prop.major >= 6 || (prop.major == 5 && prop.minor >= 3);
        INSPIRE_LOGI("FP16 support: %s", supportsFP16 ? "yes" : "no");

        // check if unified memory is supported
        INSPIRE_LOGI("unified memory support: %s", prop.unifiedAddressing ? "yes" : "no");

        // check if concurrent kernel execution is supported
        INSPIRE_LOGI("concurrent kernel execution: %s", prop.concurrentKernels ? "yes" : "no");

        // check if asynchronous engine is supported
        INSPIRE_LOGI("asynchronous engine count: %d", prop.asyncEngineCount);

        return HSUCCEED;
    } catch (const std::exception &e) {
        INSPIRE_LOGE("error when printing CUDA device info: %s", e.what());
        return HERR_DEVICE_CUDA_UNKNOWN_ERROR;
    }
}

inline static int32_t PrintCudaDeviceInfo() {
    INSPIRE_LOGI("================================================");
    auto ret = _PrintCudaDeviceInfo();
    INSPIRE_LOGI("================================================");
    return ret;
}

}  // namespace inspire

#endif  // INSPIRE_CUDA_TOOLKIT_H
#endif  // ISF_ENABLE_TENSORRT
