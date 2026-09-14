#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository=$(cd -- "$script_dir/../.." && pwd)
compiler=${GCC_COMPILER:-arm-linux-gnueabihf}
output_dir=${1:-"$repository/build/rv1126b-stability-e2e"}
[[ $(uname -s) == Linux ]] || { echo 'Linux cross-build host required' >&2; exit 1; }
[[ $("$compiler-g++" -dumpmachine) == arm*-linux-gnueabihf ]] || { echo 'ARM EABI hard-float compiler required' >&2; exit 1; }
: "${RKNN_RUNTIME_DIR:?Set RKNN_RUNTIME_DIR to the official Toolkit2 Linux/librknn_api directory}"
if [[ -n ${SDK_INSTALL_DIR:-} ]]; then sdk_dir=$SDK_INSTALL_DIR
else sdk_dir=${SDK_BUILD_DIR:-"$repository/build/inspireface-linux-armhf-rv1126b"}/install/InspireFace; fi
[[ -s "$sdk_dir/include/inspireface.h" ]] || { echo 'Missing public inspireface.h in SDK install' >&2; exit 1; }
[[ -s "$sdk_dir/lib/libInspireFace.so" ]] || { echo 'Missing public libInspireFace.so in SDK install' >&2; exit 1; }
rknn_library="$sdk_dir/lib/librknnrt.so"
if [[ ! -s "$rknn_library" ]]; then rknn_library="$RKNN_RUNTIME_DIR/armhf/librknnrt.so"; fi
[[ -s "$rknn_library" ]] || { echo 'Missing ARMHF librknnrt.so' >&2; exit 1; }
mkdir -p "$output_dir"
"$compiler-g++" -std=c++14 -O2 -Wall -Wextra -Werror \
    -I"$sdk_dir/include" \
    "$script_dir/stability_e2e_runner.cpp" -L"$sdk_dir/lib" -L"$(dirname -- "$rknn_library")" -lInspireFace -Wl,--no-as-needed -lrknnrt -Wl,--as-needed -ldl \
    -Wl,-rpath,'$ORIGIN' -o "$output_dir/stability_e2e_runner"
cp "$sdk_dir/lib/libInspireFace.so" "$rknn_library" "$output_dir/"
for candidate in "$output_dir/stability_e2e_runner" "$output_dir/libInspireFace.so" "$output_dir/librknnrt.so"; do
    file "$candidate"
done
"$compiler-readelf" -d "$output_dir/stability_e2e_runner" | grep -q 'Shared library: \[libInspireFace.so\]'
