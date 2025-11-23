#!/bin/bash
set -e

echo "========================================"
echo "   Building FingerprintApp for macOS"
echo "========================================"

# Detect qmake
if [ -f "/opt/homebrew/opt/qt/bin/qmake" ]; then
    QMAKE_CMD="/opt/homebrew/opt/qt/bin/qmake"
elif command -v qmake6 &> /dev/null; then
    QMAKE_CMD="qmake6"
elif [ -f "/opt/homebrew/opt/qt@5/bin/qmake" ]; then
    QMAKE_CMD="/opt/homebrew/opt/qt@5/bin/qmake"
elif command -v qmake &> /dev/null; then
    QMAKE_CMD="qmake"
else
    echo "Error: qmake/qmake6 not found. Please install Qt."
    exit 1
fi

echo "Using qmake: $QMAKE_CMD"

# 1. Build Dependencies (libfprint custom & digitalpersonalib)
./build_dependencies.sh

# 2. Build Main Application
echo "========================================"
echo "Building Main Application..."
echo "========================================"

if [ -f "Makefile" ]; then
    make clean
fi

$QMAKE_CMD fingerprint_app.pro
make

echo "========================================"
echo "Build Complete! Run with: open bin/FingerprintApp.app"
echo "========================================"

