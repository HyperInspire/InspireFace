#!/usr/bin/env bash
# Build only on Linux: the target is RV1126B's 32-bit ARM EABI5 hard-float ABI.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository=$(cd -- "$script_dir/../.." && pwd)
compiler=${GCC_COMPILER:-arm-linux-gnueabihf}
sdk_build=${SDK_BUILD_DIR:-"$repository/build/inspireface-linux-armhf-rv1126b"}
output_dir=${1:-"$repository/build/rv1126b-runtime-parity"}

check_armhf() {
    local header attributes
    header=$("$compiler-readelf" -h "$1")
    attributes=$("$compiler-readelf" -A "$1")
    grep -q 'Class:.*ELF32' <<< "$header"
    grep -q 'Machine:.*ARM' <<< "$header"
    grep -q 'Flags:.*Version5 EABI.*hard-float ABI' <<< "$header"
    grep -q 'Tag_ABI_VFP_args:.*VFP registers' <<< "$attributes"
}

[[ $(uname -s) == Linux ]] || { echo 'Linux cross-build host required' >&2; exit 1; }
[[ $("$compiler-g++" -dumpmachine) == arm*-linux-gnueabihf ]] || { echo 'ARM EABI hard-float compiler required' >&2; exit 1; }
if [[ -n ${SDK_INSTALL_DIR:-} ]]; then
    sdk_dir=$SDK_INSTALL_DIR
else
    : "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to the official Toolkit2 Linux/librknn_api directory}"
    [[ -s "$RKNN_RUNTIME_DIR/include/rknn_api.h" && -s "$RKNN_RUNTIME_DIR/armhf/librknnrt.so" ]] || {
        echo 'Missing official ARMHF RKNN runtime/header' >&2; exit 1;
    }
    check_armhf "$RKNN_RUNTIME_DIR/armhf/librknnrt.so"
    BUILD_DIR="$sdk_build" "$repository/command/build_cross_rv1126b_armhf.sh"
    sdk_dir="$sdk_build/install/InspireFace"
fi
[[ -s "$sdk_dir/include/inspireface.h" && -s "$sdk_dir/lib/libInspireFace.so" && -s "$sdk_dir/lib/librknnrt.so" ]] || {
    echo 'RV1126B SDK build is missing public header or runtime libraries' >&2; exit 1;
}
check_armhf "$sdk_dir/lib/libInspireFace.so"
check_armhf "$sdk_dir/lib/librknnrt.so"

mkdir -p "$output_dir"
# The wrapper class is intentionally an internal implementation detail and is
# not guaranteed to be exported by libInspireFace.  Compile the production
# RKNN2 adapter object into this executable, while linking the same SDK/runtime
# libraries as the RV1126B product build.
adapter_source="$repository/cpp/inspireface/middleware/inference_wrapper/inference_wrapper_rknn_adapter_nano.cpp"
"$compiler-g++" -std=c++14 -O2 -Wall -Wextra -Werror -DINFERENCE_WRAPPER_ENABLE_RKNN2 \
    -I"$sdk_dir/include" -I"$sdk_dir/include/inspireface" -I"$repository/cpp/inspireface" -I"$repository/cpp/inspireface/middleware/inference_wrapper" \
    "$script_dir/rknn2_parity_runner.cpp" "$adapter_source" -L"$sdk_dir/lib" -lInspireFace -lrknnrt -ldl \
    -Wl,-rpath,'$ORIGIN' -o "$output_dir/rknn2_parity_runner"
cp "$sdk_dir/lib/libInspireFace.so" "$sdk_dir/lib/librknnrt.so" "$output_dir/"

for candidate in "$output_dir/rknn2_parity_runner" "$output_dir/libInspireFace.so" "$output_dir/librknnrt.so"; do
    file "$candidate"
    check_armhf "$candidate"
done
"$compiler-readelf" -d "$output_dir/rknn2_parity_runner" | grep -q 'Shared library: \[libInspireFace.so\]'
"$compiler-readelf" -d "$output_dir/rknn2_parity_runner" | grep -q 'Shared library: \[librknnrt.so\]'
