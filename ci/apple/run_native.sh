#!/bin/bash
# Run each architecture's contracts on its existing native macOS runner.
set -euo pipefail
repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
backend="${1:-cpu}"
case "$backend" in cpu|coreml|all) ;; *) echo "Invalid Apple backend: $backend" >&2; exit 2 ;; esac
host_arch="$(uname -m)"
python3 "$repo_dir/command/apple/build_sdk.py" --platform macosx --arch "$host_arch" \
  --backend "$backend" --verify --coverage
if [[ "$host_arch" == arm64 ]]; then
  python3 "$repo_dir/command/apple/build_sdk.py" --platform iphoneos --backend "$backend" --verify
fi
python3 "$repo_dir/command/apple/build_sdk.py" --platform iphonesimulator --arch "$host_arch" \
  --backend "$backend" --verify --run-simulator
