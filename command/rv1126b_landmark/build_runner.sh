#!/usr/bin/env bash
set -euo pipefail
: "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to Toolkit2 Linux/librknn_api}"
COMPILER_PREFIX=${GCC_COMPILER:-arm-linux-gnueabihf}
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
OUTPUT=${1:-"${SCRIPT_DIR}/rknn_landmark_runner"}
"${COMPILER_PREFIX}-g++" -std=c++14 -O2 \
    -I"${RKNN_RUNTIME_DIR}/include" \
    "${SCRIPT_DIR}/rknn_landmark_runner.cpp" \
    -L"${RKNN_RUNTIME_DIR}/armhf" -lrknnrt -ldl \
    -Wl,-rpath,/oem/usr/lib -o "${OUTPUT}"
"${COMPILER_PREFIX}-strip" "${OUTPUT}"
