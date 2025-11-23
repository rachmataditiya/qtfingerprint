#!/bin/bash
set -e

echo "========================================"
echo "   Building FingerprintApp for Linux"
echo "========================================"

# Function to check dependency
check_dep() {
    if ! dpkg -s $1 &> /dev/null; then
        echo "Missing dependency: $1"
        return 1
    fi
    return 0
}

# Install Dependencies (Debian/Ubuntu)
if command -v apt-get &> /dev/null; then
    echo "Checking dependencies..."
    MISSING_DEPS=()
    
    # Core dependencies
    check_dep libfprint-2-dev || MISSING_DEPS+=("libfprint-2-dev")
    check_dep libglib2.0-dev || MISSING_DEPS+=("libglib2.0-dev")
    check_dep qt6-base-dev || MISSING_DEPS+=("qt6-base-dev")
    check_dep libqt6sql6-sqlite || MISSING_DEPS+=("libqt6sql6-sqlite")
    check_dep build-essential || MISSING_DEPS+=("build-essential")

    if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
        echo "Installing missing dependencies: ${MISSING_DEPS[*]}"
        sudo apt-get update
        sudo apt-get install -y "${MISSING_DEPS[@]}"
    else
        echo "All dependencies satisfied."
    fi
else
    echo "Warning: apt-get not found. Please ensure dependencies are installed manually (libfprint-2, qt6, etc)."
fi

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

