#!/bin/bash

# Define download URLs
MNN_IOS_URL="https://github.com/alibaba/MNN/releases/download/2.8.1/mnn_2.8.1_ios_armv82_cpu_metal_coreml.zip"
OPENCV_IOS_URL="https://github.com/opencv/opencv/releases/download/4.5.1/opencv-4.5.1-ios-framework.zip"

# Set the cache directory
macos_cache=".macos_cache/"

# Create the directory if it does not exist
mkdir -p "${macos_cache}"

# Function to download and unzip a file if the required framework does not exist
download_and_unzip() {
    local url=$1
    local dir=$2
    local framework_name=$3  # Name of the framework directory to check

    # Check if the framework already exists
    if [ ! -d "${dir}${framework_name}" ]; then
        local file_name=$(basename "$url")
        local full_path="${dir}${file_name}"

        # Check if the zip file already exists
        if [ ! -f "$full_path" ]; then
            echo "Downloading ${file_name}..."
            # Download the file
            curl -sL "$url" -o "$full_path"
        else
            echo "${file_name} already downloaded. Proceeding to unzip."
        fi

        # Unzip the file to a temporary directory
        echo "Unzipping ${file_name}..."
        unzip -q "$full_path" -d "${dir}"
        rm "$full_path"

        # Move the framework if it's in a subdirectory specific to the iOS build
        if [ "${framework_name}" == "MNN.framework" ]; then
            mv "${dir}ios_build/Release-iphoneos/${framework_name}" "${dir}"
            rm -rf "${dir}ios_build"  # Clean up the subdirectory
        fi

        echo "${framework_name} has been set up."
    else
        echo "${framework_name} already exists in ${dir}. Skipping download and unzip."
    fi
}

# Download and unzip MNN iOS package
download_and_unzip "$MNN_IOS_URL" "$macos_cache" "MNN.framework"

# Download and unzip OpenCV iOS package
download_and_unzip "$OPENCV_IOS_URL" "$macos_cache" "opencv2.framework"


TOOLCHAIN=toolchain/ios.toolchain.cmake

BUILD_DIR="build/inspireface-ios"

mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

cmake \
    -DIOS_3RDPARTY=".macos_cache" \
    -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN} \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DIOS_DEPLOYMENT_TARGET=9.0 \
    -DENABLE_BITCODE=0 \
    ../..

#make -j2