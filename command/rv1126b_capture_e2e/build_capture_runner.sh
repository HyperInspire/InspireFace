#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository=$(cd -- "$script_dir/../.." && pwd)
compiler=${GCC_COMPILER:-arm-linux-gnueabihf}
output_dir=${1:-"$repository/build/rv1126b-capture-e2e"}
[[ $(uname -s) == Linux ]] || { echo 'Linux cross-build host required' >&2; exit 1; }
[[ $("$compiler-g++" -dumpmachine) == arm*-linux-gnueabihf ]] || { echo 'ARM EABI hard-float compiler required' >&2; exit 1; }
: "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to the official Toolkit2 Linux/librknn_api directory}"
if [[ -n ${SDK_INSTALL_DIR:-} ]]; then sdk_dir=$SDK_INSTALL_DIR
else sdk_dir=${SDK_BUILD_DIR:-"$repository/build/inspireface-linux-armhf-rv1126b"}/install/InspireFace; fi
mkdir -p "$output_dir"
"$compiler-g++" -std=c++14 -O2 -Wall -Wextra -Werror \
    -I"$sdk_dir/include" \
    "$script_dir/capture_e2e_runner.cpp" -L"$sdk_dir/lib" -lInspireFace -Wl,--no-as-needed -lrknnrt -Wl,--as-needed -ldl \
    -Wl,-rpath,'$ORIGIN' -o "$output_dir/capture_e2e_runner"
cp "$sdk_dir/lib/libInspireFace.so" "$sdk_dir/lib/librknnrt.so" "$output_dir/"
"$compiler-readelf" -d "$output_dir/capture_e2e_runner" | grep -q 'Shared library: \[libInspireFace.so\]'
