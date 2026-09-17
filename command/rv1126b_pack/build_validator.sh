#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository=$(cd -- "$script_dir/../.." && pwd)
compiler=${GCC_COMPILER:-arm-linux-gnueabihf}
sdk_build=${SDK_BUILD_DIR:-"$repository/build/inspireface-linux-armhf-rv1126b"}
output_dir=${1:-"$repository/build/rv1126b-pack-validator"}

check_armhf() {
    local header attributes
    header=$("$compiler-readelf" -h "$1")
    attributes=$("$compiler-readelf" -A "$1")
    grep -q 'Class:.*ELF32' <<< "$header"
    grep -q 'Machine:.*ARM' <<< "$header"
    grep -q 'Flags:.*Version5 EABI.*hard-float ABI' <<< "$header"
    grep -q 'Tag_ABI_VFP_args:.*VFP registers' <<< "$attributes"
}

check_runtime_version() {
    local text major minor patch
    text=$(strings "$1")
    if [[ $text =~ librknnrt\ version:\ ([0-9]+)\.([0-9]+)\.([0-9]+) ]]; then
        major=$((10#${BASH_REMATCH[1]}))
        minor=$((10#${BASH_REMATCH[2]}))
        patch=$((10#${BASH_REMATCH[3]}))
        (( major > 2 || (major == 2 && (minor > 3 || (minor == 3 && patch >= 2))) )) && return 0
    fi
    echo 'SDK RKNN runtime is older than 2.3.2 or has no valid version' >&2
    return 1
}

[[ $(uname -s) == Linux ]] || { echo 'Linux cross-build host required' >&2; exit 1; }
[[ $($compiler-g++ -dumpmachine) == arm*-linux-gnueabihf ]] || { echo 'ARM EABI hard-float compiler required' >&2; exit 1; }
if [[ -n ${SDK_INSTALL_DIR:-} ]]; then
    sdk_dir=$SDK_INSTALL_DIR
else
    : "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to the official Toolkit2 2.3.2 Linux/librknn_api directory}"
    [[ -s "$RKNN_RUNTIME_DIR/include/rknn_api.h" && -s "$RKNN_RUNTIME_DIR/armhf/librknnrt.so" ]] || {
        echo 'Missing official RKNN 2.3.2 ARMHF runtime' >&2; exit 1;
    }
    check_armhf "$RKNN_RUNTIME_DIR/armhf/librknnrt.so"
    check_runtime_version "$RKNN_RUNTIME_DIR/armhf/librknnrt.so"
    BUILD_DIR="$sdk_build" "$repository/command/build_cross_rv1126b_armhf.sh"
    sdk_dir="$sdk_build/install/InspireFace"
fi
[[ -s "$sdk_dir/include/inspireface.h" && -s "$sdk_dir/lib/libInspireFace.so" && -s "$sdk_dir/lib/librknnrt.so" ]] || {
    echo 'Existing RV1126B SDK build is missing its public header or runtime libraries' >&2; exit 1;
}
check_armhf "$sdk_dir/lib/librknnrt.so"
check_runtime_version "$sdk_dir/lib/librknnrt.so"

mkdir -p "$output_dir"
"$compiler-g++" -std=c++14 -O2 -Wall -Wextra -Werror \
    -I"$sdk_dir/include" "$script_dir/validate_pack_main.cpp" \
    -L"$sdk_dir/lib" -lInspireFace -Wl,-rpath,'$ORIGIN' -o "$output_dir/validate_pack_main"
cp "$sdk_dir/lib/libInspireFace.so" "$sdk_dir/lib/librknnrt.so" "$output_dir/"

for candidate in "$output_dir/validate_pack_main" "$output_dir/libInspireFace.so" "$output_dir/librknnrt.so"; do
    file "$candidate"
    check_armhf "$candidate"
done
"$compiler-readelf" -d "$output_dir/validate_pack_main" | grep -q 'Shared library: \[libInspireFace.so\]'
"$compiler-readelf" -d "$output_dir/libInspireFace.so" | grep -q 'Shared library: \[librknnrt.so\]'
