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

BUILD_FOLDER_PATH="build/inspireface-aarch64-manylinux_2_36${TAG}/"
SCRIPT_DIR=$(pwd)  # Project dir

mkdir -p ${BUILD_FOLDER_PATH}
# shellcheck disable=SC2164
cd ${BUILD_FOLDER_PATH}

cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DISF_BUILD_WITH_SAMPLE=OFF \
  -DISF_BUILD_WITH_TEST=OFF \
  -DISF_ENABLE_BENCHMARK=OFF \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DISF_BUILD_SHARED_LIBS=ON \
  -Wno-dev \
  ${SCRIPT_DIR}

make -j4
make install

move_install_files "$(pwd)"
BUILD_DYLIB_PATH="$(pwd)/InspireFace/lib/libInspireFace.so"
# Copy the library to the python directory

DYLIB_DEST_PATH="${SCRIPT_DIR}/python/inspireface/modules/core/libs/linux/arm64/"
mkdir -p ${DYLIB_DEST_PATH}
cp -r ${BUILD_DYLIB_PATH} ${DYLIB_DEST_PATH}


PYTHON_PRJ_PATH=${SCRIPT_DIR}/python
cd ${PYTHON_PRJ_PATH}/
# Build wheels for Python 3.7-3.12
for PYTHON_VERSION in python3.7 python3.8 python3.9 python3.10 python3.11 python3.12; do
    if [[ "${PYTHON_VERSION}" == "python3.12" ]]; then
        ${PYTHON_VERSION} -m pip install setuptools wheel twine
    fi
    ${PYTHON_VERSION} setup.py bdist_wheel
done

echo "Build wheel for Linux arm64, Well Done!"
