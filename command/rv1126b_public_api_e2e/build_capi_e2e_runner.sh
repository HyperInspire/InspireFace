#!/usr/bin/env bash
# Build the public-C-API E2E runner for RV1126B's ARM EABI5 hard-float ABI.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository=$(cd -- "$script_dir/../.." && pwd)
compiler=${GCC_COMPILER:-arm-linux-gnueabihf}
sdk_build=${SDK_BUILD_DIR:-"$repository/build/inspireface-linux-armhf-rv1126b"}
output_dir=${1:-"$repository/build/rv1126b-public-api-e2e"}

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
: "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to the official Toolkit2 Linux/librknn_api directory}"
[[ -s "$RKNN_RUNTIME_DIR/armhf/librknnrt.so" ]] || { echo 'Missing official ARMHF RKNN runtime' >&2; exit 1; }
check_armhf "$RKNN_RUNTIME_DIR/armhf/librknnrt.so"

if [[ -n ${SDK_INSTALL_DIR:-} ]]; then
    sdk_dir=$SDK_INSTALL_DIR
else
    BUILD_DIR="$sdk_build" "$repository/command/build_cross_rv1126b_armhf.sh"
    sdk_dir="$sdk_build/install/InspireFace"
fi
[[ -s "$sdk_dir/include/inspireface.h" ]] || { echo 'Missing public inspireface.h in SDK install' >&2; exit 1; }
[[ -s "$sdk_dir/lib/libInspireFace.so" ]] || { echo 'Missing public libInspireFace.so in SDK install' >&2; exit 1; }

rknn_library="$sdk_dir/lib/librknnrt.so"
if [[ ! -s "$rknn_library" ]]; then
    rknn_library="$RKNN_RUNTIME_DIR/armhf/librknnrt.so"
fi
[[ -s "$rknn_library" ]] || { echo 'Missing ARMHF librknnrt.so' >&2; exit 1; }
check_armhf "$sdk_dir/lib/libInspireFace.so"
check_armhf "$rknn_library"
rknn_library_directory=$(dirname -- "$rknn_library")

mkdir -p "$output_dir"
"$compiler-g++" -std=c++14 -O2 -Wall -Wextra -Werror \
    -I"$sdk_dir/include" \
    "$script_dir/capi_e2e_runner.cpp" -L"$sdk_dir/lib" -L"$rknn_library_directory" -lInspireFace -Wl,--no-as-needed -lrknnrt -Wl,--as-needed -ldl \
    -Wl,-rpath,'$ORIGIN' -o "$output_dir/capi_e2e_runner"

cp "$sdk_dir/lib/libInspireFace.so" "$rknn_library" "$output_dir/"
for candidate in "$output_dir/capi_e2e_runner" "$output_dir/libInspireFace.so" "$output_dir/librknnrt.so"; do
    file "$candidate"
    check_armhf "$candidate"
done
"$compiler-readelf" -d "$output_dir/capi_e2e_runner" | grep -q 'Shared library: \[libInspireFace.so\]'
"$compiler-readelf" -d "$output_dir/capi_e2e_runner" | grep -q 'Shared library: \[librknnrt.so\]'
