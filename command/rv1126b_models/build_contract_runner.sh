#!/usr/bin/env bash
set -euo pipefail
[[ $(uname -s) == Linux ]] || { echo 'Linux build host required' >&2; exit 1; }
: "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to official Toolkit2 Linux/librknn_api}"
compiler=${GCC_COMPILER:-arm-linux-gnueabihf}
[[ $($compiler-g++ -dumpmachine) == arm*-linux-gnueabihf ]] || { echo 'ARM hard-float compiler required' >&2; exit 1; }
[[ -s "$RKNN_RUNTIME_DIR/include/rknn_api.h" && -s "$RKNN_RUNTIME_DIR/armhf/librknnrt.so" ]] || { echo 'Missing official ARMHF header/runtime' >&2; exit 1; }
"$compiler-readelf" -h "$RKNN_RUNTIME_DIR/armhf/librknnrt.so" | grep -q 'hard-float ABI' || { echo 'Runtime is not ARM hard-float' >&2; exit 1; }
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
output=${1:?Specify output executable path}
"$compiler-g++" -std=c++14 -O2 -Wall -Wextra -Werror -I"$RKNN_RUNTIME_DIR/include" \
    "$script_dir/rknn_contract_runner.cpp" -L"$RKNN_RUNTIME_DIR/armhf" -lrknnrt -ldl \
    -Wl,-rpath,/oem/usr/lib -o "$output"
file "$output"
"$compiler-readelf" -h -A -d "$output"
