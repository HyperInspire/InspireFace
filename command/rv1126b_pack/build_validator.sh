#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository=$(cd -- "$script_dir/../.." && pwd)
compiler=${GCC_COMPILER:-arm-linux-gnueabihf}
sdk_build=${SDK_BUILD_DIR:-"$repository/build/inspireface-linux-armhf-rv1126b"}
output_dir=${1:-"$repository/build/rv1126b-pack-validator"}

[[ $(uname -s) == Linux ]] || { echo 'Linux cross-build host required' >&2; exit 1; }
[[ $($compiler-g++ -dumpmachine) == arm*-linux-gnueabihf ]] || { echo 'ARM EABI hard-float compiler required' >&2; exit 1; }
if [[ -n ${SDK_INSTALL_DIR:-} ]]; then
    sdk_dir=$SDK_INSTALL_DIR
else
    : "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to the official Toolkit2 2.3.2 Linux/librknn_api directory}"
    [[ -s "$RKNN_RUNTIME_DIR/include/rknn_api.h" && -s "$RKNN_RUNTIME_DIR/armhf/librknnrt.so" ]] || {
        echo 'Missing official RKNN 2.3.2 ARMHF runtime' >&2; exit 1;
    }
    "$compiler-readelf" -h "$RKNN_RUNTIME_DIR/armhf/librknnrt.so" | grep -q 'hard-float ABI' || {
        echo 'RKNN runtime is not ARM hard-float' >&2; exit 1;
    }
    BUILD_DIR="$sdk_build" "$repository/command/build_cross_rv1126b_armhf.sh"
    sdk_dir="$sdk_build/install/InspireFace"
fi
[[ -s "$sdk_dir/include/inspireface.h" && -s "$sdk_dir/lib/libInspireFace.so" && -s "$sdk_dir/lib/librknnrt.so" ]] || {
    echo 'Existing RV1126B SDK build is missing its public header or runtime libraries' >&2; exit 1;
}
"$compiler-readelf" -h "$sdk_dir/lib/librknnrt.so" | grep -q 'hard-float ABI' || {
    echo 'SDK RKNN runtime is not ARM hard-float' >&2; exit 1;
}
strings "$sdk_dir/lib/librknnrt.so" | grep -E 'librknnrt version: (2\.([3-9]|[1-9][0-9]+)\.|[3-9][0-9]*\.)' > /dev/null || {
    echo 'SDK RKNN runtime is older than 2.3.2' >&2; exit 1;
}

mkdir -p "$output_dir"
"$compiler-g++" -std=c++14 -O2 -Wall -Wextra -Werror \
    -I"$sdk_dir/include" "$script_dir/validate_pack_main.cpp" \
    -L"$sdk_dir/lib" -lInspireFace -Wl,-rpath,'$ORIGIN' -o "$output_dir/validate_pack_main"
cp "$sdk_dir/lib/libInspireFace.so" "$sdk_dir/lib/librknnrt.so" "$output_dir/"

for candidate in "$output_dir/validate_pack_main" "$output_dir/libInspireFace.so" "$output_dir/librknnrt.so"; do
    file "$candidate"
    "$compiler-readelf" -h "$candidate" | grep -q 'ELF32'
    "$compiler-readelf" -h "$candidate" | grep -q 'Machine:.*ARM'
    "$compiler-readelf" -A "$candidate" | grep -q 'hard-float ABI'
done
"$compiler-readelf" -d "$output_dir/validate_pack_main" | grep -q 'Shared library: \[libInspireFace.so\]'
"$compiler-readelf" -d "$output_dir/libInspireFace.so" | grep -q 'Shared library: \[librknnrt.so\]'
