#!/bin/bash

# Build script for Fingerprint PoC

set -e

echo "==================================="
echo "Building Fingerprint PoC"
echo "==================================="

# Create build directory
BUILD_DIR="build_poc"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release -S .. -B . -f ../CMakeLists_poc.txt

echo ""
echo "Building..."
cmake --build . -j$(nproc)

echo ""
echo "==================================="
echo "Build complete!"
echo "==================================="
echo ""
echo "Executable: $BUILD_DIR/bin/FingerprintPoC"
echo ""
echo "To run:"
echo "  cd $BUILD_DIR"
echo "  ./bin/FingerprintPoC"
echo ""

