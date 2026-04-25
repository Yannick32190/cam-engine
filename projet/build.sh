#!/bin/bash
set -e
BUILD_DIR="$(dirname "$0")/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
echo ""
echo "=== cam-engine compilé : $BUILD_DIR/cam-engine ==="
