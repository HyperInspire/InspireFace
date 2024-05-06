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


SCRIPT_DIR=$(pwd)  # Project dir

mkdir -p build/linux_cuda
# shellcheck disable=SC2164
cd build/linux_cuda

cmake -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DISF_BUILD_WITH_SAMPLE=OFF \
  -DISF_BUILD_WITH_TEST=OFF \
  -DISF_ENABLE_BENCHMARK=OFF \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DISF_GLOBAL_INFERENCE_BACKEND_USE_MNN_CUDA=ON \
  -DISF_LINUX_MNN_CUDA=/home/tunm/software/MNN-2.7.0/build_cuda/install \
  -DISF_BUILD_SHARED_LIBS=ON ${SCRIPT_DIR}

make -j4

move_install_files "$(pwd)"
