#!/bin/bash
set -e

echo "========================================"
echo "   Building FingerprintApp for Linux"
echo "========================================"

# Detect qmake
if command -v qmake6 &> /dev/null; then
    QMAKE_CMD="qmake6"
elif command -v qmake &> /dev/null; then
    QMAKE_CMD="qmake"
else
    echo "Error: qmake not found. Please install Qt."
    exit 1
fi

echo "Using qmake: $QMAKE_CMD"

# 1. Build digitalpersonalib (Wrapper Library)
echo "========================================"
echo "Building libdigitalpersona..."
echo "========================================"
cd digitalpersonalib

# Clean previous build
if [ -f "Makefile" ]; then
    make clean
    rm Makefile
fi

$QMAKE_CMD digitalpersonalib.pro
make
cd ..

# Check if library exists
if [ ! -f "digitalpersonalib/lib/libdigitalpersona.so" ] && [ ! -f "digitalpersonalib/lib/libdigitalpersona.so.1" ]; then
    echo "Error: Failed to build libdigitalpersona.so"
    exit 1
fi

# 2. Build Main Application
echo "========================================"
echo "Building Main Application..."
echo "========================================"

# Clean previous build
if [ -f "Makefile" ]; then
    make clean
    rm Makefile
fi

$QMAKE_CMD fingerprint_app.pro
make

echo "========================================"
echo "Build Complete!"
echo "Run with: ./bin/FingerprintApp"
echo "Note: Ensure you have permissions to access the fingerprint reader (udev rules)."
echo "========================================"

