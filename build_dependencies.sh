#!/bin/bash
set -e

# Detect qmake
if command -v qmake &> /dev/null; then
    QMAKE_CMD="qmake"
elif [ -f "/opt/homebrew/opt/qt@5/bin/qmake" ]; then
    QMAKE_CMD="/opt/homebrew/opt/qt@5/bin/qmake"
else
    echo "Error: qmake not found. Please install Qt5."
    exit 1
fi

echo "Using qmake: $QMAKE_CMD"

echo "========================================"
echo "Building libfprint (Meson/Ninja)..."
echo "========================================"
cd libfprint_repo

if [ ! -d "builddir" ]; then
    echo "Configuring meson build..."
    meson setup builddir -Dudev_rules=disabled -Dudev_hwdb=disabled -Ddoc=false
fi

ninja -C builddir
cd ..

echo "========================================"
echo "Building libdigitalpersona (Qt)..."
echo "========================================"
cd digitalpersonalib
$QMAKE_CMD digitalpersonalib.pro
make
cd ..

echo "========================================"
echo "Dependencies built successfully!"
echo "You can now build FingerprintApp in Qt Creator."
echo "========================================"

