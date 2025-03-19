#!/bin/bash

# Reusable function to handle 'install' directory operations
move_install_files() {
    local root_dir="$1"
    local install_dir="$root_dir/install"

    # Step 1: Check if the 'install' directory exists
    if [ ! -d "$install_dir" ]; then
        echo "Error: 'install' directory does not exist in $root_dir"
        exit 1
    fi

    # Step 2: Delete all other files/folders except 'install'
    find "$root_dir" -mindepth 1 -maxdepth 1 -not -name "install" -exec rm -rf {} +

    # Step 3: Move all files from 'install' to the root directory
    mv "$install_dir"/* "$root_dir" 2>/dev/null

    # Step 4: Remove the empty 'install' directory
    rmdir "$install_dir"

    echo "Files from 'install' moved to $root_dir, and 'install' directory deleted."
}

if [ -n "$VERSION" ]; then
    TAG="-$VERSION"
else
    TAG=""
fi

# 获取项目根目录的绝对路径
SCRIPT_DIR=$(pwd)  # Project dir
BUILD_FOLDER_PATH="${SCRIPT_DIR}/build/inspireface-linux-tensorrt${TAG}"

mkdir -p ${BUILD_FOLDER_PATH}
cd ${BUILD_FOLDER_PATH}

echo "TENSORRT_ROOT: ${TENSORRT_ROOT}"
# 明确指定CUDA路径
CUDA_PATH="/usr/local/cuda"  # 请替换为您系统上的实际CUDA路径

cmake -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DISF_BUILD_WITH_SAMPLE=ON \
  -DISF_BUILD_WITH_TEST=ON \
  -DISF_ENABLE_BENCHMARK=ON \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DTENSORRT_ROOT=/home/tunm/software/TensorRT-10.8.0.43 \
  -DCUDA_TOOLKIT_ROOT_DIR=${CUDA_PATH} \
  -DISF_ENABLE_TENSORRT=ON ${SCRIPT_DIR}

make -j4

move_install_files "$(pwd)"