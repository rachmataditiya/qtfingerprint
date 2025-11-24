#!/bin/bash
set -e

# Detect qmake
# Prefer Homebrew Qt 6 if available
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
    echo "Error: qmake/qmake6 not found. Please install Qt 6."
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

