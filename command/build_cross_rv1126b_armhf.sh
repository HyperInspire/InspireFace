#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
: "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to RKNN 2.3.2+ rknpu2/runtime/Linux/librknn_api}"
COMPILER_PREFIX=${GCC_COMPILER:-${ARM_CROSS_COMPILE_TOOLCHAIN:+${ARM_CROSS_COMPILE_TOOLCHAIN}/bin/}arm-linux-gnueabihf}
BUILD_DIR=${BUILD_DIR:-"${SCRIPT_DIR}/build/inspireface-linux-armhf-rv1126b"}

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=armv7 \
    -DCMAKE_C_COMPILER="${COMPILER_PREFIX}-gcc" \
    -DCMAKE_CXX_COMPILER="${COMPILER_PREFIX}-g++" \
    -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS:-} -mfpu=neon" \
    -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} -mfpu=neon -flax-vector-conversions" \
    -DCMAKE_BUILD_TYPE=Release \
    -DISF_BUILD_LINUX_AARCH64=OFF \
    -DISF_BUILD_LINUX_ARM7=ON \
    -DISF_ENABLE_RKNN=ON \
    -DISF_RK_DEVICE_TYPE=RV1126B \
    -DISF_RK_COMPILER_TYPE=armhf \
    -DISF_RKNN_RUNTIME_DIR="${RKNN_RUNTIME_DIR}" \
    -DISF_ENABLE_RGA="${ISF_ENABLE_RGA:-ON}" \
    -DISF_BUILD_WITH_SAMPLE=OFF \
    -DISF_BUILD_WITH_TEST=OFF \
    -DISF_BUILD_SHARED_LIBS=ON
cmake --build "${BUILD_DIR}" --parallel "${JOBS:-4}"
cmake --install "${BUILD_DIR}"
