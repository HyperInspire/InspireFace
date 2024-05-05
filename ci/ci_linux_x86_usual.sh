#!/bin/bash

set -e  # If the command fails to execute, exit immediately

TARGET_DIR="test_res"
DOWNLOAD_URL="https://github.com/tunmx/inspireface-store/raw/main/resource/test_res-lite.zip"
ZIP_FILE="test_res-lite.zip"
BUILD_DIRNAME="ci_ubuntu18"
TEST_DIR="./build/${BUILD_DIRNAME}/test"
TEST_EXECUTABLE="$(realpath ./build/${BUILD_DIRNAME}/test/Test)"
FULL_TEST_DIR="$(realpath ${TARGET_DIR})"

# Check whether the directory exists
if [ ! -d "$TARGET_DIR" ]; then
    echo "Directory '$TARGET_DIR' does not exist. Downloading..."

    wget -q "$DOWNLOAD_URL" -O "$ZIP_FILE"

    echo "Extracting '$ZIP_FILE' to '$TARGET_DIR'..."
    unzip "$ZIP_FILE"

    rm "$ZIP_FILE"
    rm "__MACOSX"

    echo "Download and extraction complete."
else
    echo "Directory '$TARGET_DIR' already exists. Skipping download."
fi

mkdir -p build/${BUILD_DIRNAME}/
# shellcheck disable=SC2164
cd build/${BUILD_DIRNAME}/

cmake -DCMAKE_BUILD_TYPE=Release \
  -DISF_BUILD_WITH_SAMPLE=OFF \
  -DISF_BUILD_WITH_TEST=ON \
  -DISF_ENABLE_BENCHMARK=ON \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DOpenCV_DIR=3rdparty/inspireface-precompile/opencv/4.5.1/opencv-ubuntu18-x86/lib/cmake/opencv4 \
  -DISF_BUILD_SHARED_LIBS=OFF ../../

make -j4

ln -s ${FULL_TEST_DIR} .

if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Error: Test executable '$TEST_EXECUTABLE' not found. Please ensure it is built correctly."
    exit 1  # Exit with a non-zero status code to indicate an error
else
    echo "Test executable found. Running tests..."
    "$TEST_EXECUTABLE"
fi

