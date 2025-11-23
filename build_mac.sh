#!/bin/bash
set -e

echo "========================================"
echo "   Building FingerprintApp for macOS"
echo "========================================"

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew not found. Please install it first."
    exit 1
fi

# Install Dependencies
echo "Checking dependencies..."
brew list qt &>/dev/null || brew install qt
brew list libusb &>/dev/null || brew install libusb
brew list libgusb &>/dev/null || brew install libgusb
brew list pixman &>/dev/null || brew install pixman
brew list glib &>/dev/null || brew install glib
brew list meson &>/dev/null || brew install meson
brew list ninja &>/dev/null || brew install ninja
brew list pkg-config &>/dev/null || brew install pkg-config

# Detect qmake
if [ -f "/opt/homebrew/opt/qt/bin/qmake" ]; then
    QMAKE_CMD="/opt/homebrew/opt/qt/bin/qmake"
elif command -v qmake6 &> /dev/null; then
    QMAKE_CMD="qmake6"
elif [ -f "/usr/local/opt/qt/bin/qmake" ]; then
    QMAKE_CMD="/usr/local/opt/qt/bin/qmake"
elif command -v qmake &> /dev/null; then
    # Check if default qmake is qt6
    VER=$(qmake -query QT_VERSION)
    if [[ "$VER" == 6.* ]]; then
        QMAKE_CMD="qmake"
    else
        echo "Warning: Default qmake is version $VER. Looking for Qt 6..."
        # Try to find qt6 specifically
        if [ -d "/opt/homebrew/opt/qt@6/bin" ]; then
             QMAKE_CMD="/opt/homebrew/opt/qt@6/bin/qmake"
        else
             echo "Error: Qt 6 not found. Please install qt (brew install qt)"
             exit 1
        fi
    fi
else
    echo "Error: qmake not found. Please install Qt 6."
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

