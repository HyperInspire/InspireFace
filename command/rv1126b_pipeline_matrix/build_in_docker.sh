#!/bin/bash
set -e
SDK_DIR=/workspace/build/inspireface-linux-armhf-rv1126b/install/InspireFace
RKNN_LIB_DIR=/rknn/armhf
OUTPUT_DIR=/tmp/build_output
mkdir -p $OUTPUT_DIR
arm-linux-gnueabihf-g++ -std=c++14 -O2 -Wall -Wextra -Werror \
    -I${SDK_DIR}/include \
    /tmp/pipeline_matrix_runner.cpp \
    -L${SDK_DIR}/lib -L${RKNN_LIB_DIR} \
    -lInspireFace -Wl,--no-as-needed -lrknnrt -Wl,--as-needed -ldl \
    -Wl,-rpath,'$ORIGIN' -o ${OUTPUT_DIR}/pipeline_matrix_runner
cp ${SDK_DIR}/lib/libInspireFace.so ${RKNN_LIB_DIR}/librknnrt.so ${OUTPUT_DIR}/
ls -la ${OUTPUT_DIR}/
echo "--- ELF check ---"
arm-linux-gnueabihf-readelf -h ${OUTPUT_DIR}/pipeline_matrix_runner | grep -E 'Class|Machine|Flags'
arm-linux-gnueabihf-readelf -A ${OUTPUT_DIR}/pipeline_matrix_runner | grep VFP
arm-linux-gnueabihf-readelf -d ${OUTPUT_DIR}/pipeline_matrix_runner | grep 'Shared library'
