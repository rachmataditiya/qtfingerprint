#!/bin/bash

# Quick run script for x86-64 systems

echo "======================================"
echo "U.are.U 4500 Fingerprint PoC"
echo "======================================"
echo ""

# Check architecture
ARCH=$(uname -m)
if [ "$ARCH" != "x86_64" ]; then
    echo "⚠️  WARNING: This system is $ARCH"
    echo "   SDK only supports x86-64"
    echo "   Please run on x86-64 system"
    echo ""
    exit 1
fi

# Check if built
if [ ! -f "bin/FingerprintPoC" ]; then
    echo "Building project..."
    qmake6 fingerprint_poc.pro && make -j$(nproc)
    if [ $? -ne 0 ]; then
        echo "❌ Build failed"
        exit 1
    fi
fi

# Check device
echo "Checking for U.are.U device..."
lsusb | grep -i "DigitalPersona"
if [ $? -ne 0 ]; then
    echo "⚠️  WARNING: U.are.U device not detected"
    echo "   Please connect the fingerprint reader"
    echo ""
fi

# Set library path
export LD_LIBRARY_PATH=/opt/DigitalPersona/urusdk-linux/Linux/lib/x64:$LD_LIBRARY_PATH

echo ""
echo "Starting application..."
echo ""

./bin/FingerprintPoC

echo ""
echo "Application closed."

