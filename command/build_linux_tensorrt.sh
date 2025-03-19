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

SCRIPT_DIR=$(pwd)  
BUILD_FOLDER_NAME="inspireface-linux-tensorrt${TAG}"

# 创建两层嵌套的目录结构
mkdir -p build/${BUILD_FOLDER_NAME}
cd build/${BUILD_FOLDER_NAME}

echo "TENSORRT_ROOT: ${TENSORRT_ROOT}"

cmake  \
  -DCMAKE_BUILD_TYPE=Release \
  -DISF_BUILD_WITH_SAMPLE=ON \
  -DISF_BUILD_WITH_TEST=ON \
  -DISF_ENABLE_BENCHMARK=ON \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DTENSORRT_ROOT=${TENSORRT_ROOT} \
  -DISF_ENABLE_TENSORRT=ON ../..

make -j4

cd ../..

if [ $? -eq 0 ] && [ -d "$(pwd)/install" ]; then
  move_install_files "$(pwd)"
else
  echo "Build failed or the installation directory does not exist"
  exit 1
fi