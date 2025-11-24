#!/bin/bash
set -e

###############################################################################
# Build Qt Application for Android
#
# This script builds the Qt fingerprint application for Android using
# the pre-built libfprint libraries.
#
# Prerequisites:
#   1. Run build_libfprint_android_complete.sh first
#   2. Qt Creator configured for Android (or Qt Android SDK installed)
#   3. Android NDK installed
#
# Usage:
#   ./build_qt_android.sh [ARCH] [API_LEVEL]
#
# Examples:
#   ./build_qt_android.sh                    # arm64-v8a, API 29
#   ./build_qt_android.sh arm64-v8a 29        # Explicit
###############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Configuration
ARCH="${1:-arm64-v8a}"
API_LEVEL="${2:-29}"

echo "=========================================="
echo "Qt Android Build Script"
echo "=========================================="
echo "Architecture: $ARCH"
echo "API Level: $API_LEVEL"
echo "=========================================="
echo ""

# Check if libfprint is built
ANDROID_PREFIX="${SCRIPT_DIR}/android-ndk-prefix"
if [ ! -f "${ANDROID_PREFIX}/lib/libfprint-2.so" ]; then
    echo "Error: libfprint not found at ${ANDROID_PREFIX}/lib/libfprint-2.so"
    echo "Please run build_libfprint_android_complete.sh first"
    exit 1
fi

# Detect qmake6 (Qt 6 required) - Prefer Android Qt installation
QT_ANDROID_PATH="/Users/rachmataditiya/Qt/6.10.0/android_arm64_v8a"
QT_BASE_PATH="/Users/rachmataditiya/Qt/6.10.0"

if [ -f "$QT_ANDROID_PATH/bin/qmake" ]; then
    QMAKE_CMD="$QT_ANDROID_PATH/bin/qmake"
    echo "Using Qt Android qmake: $QMAKE_CMD"
elif [ -f "$QT_BASE_PATH/android_arm64_v8a/bin/qmake" ]; then
    QMAKE_CMD="$QT_BASE_PATH/android_arm64_v8a/bin/qmake"
    echo "Using Qt Android qmake: $QMAKE_CMD"
elif command -v qmake6 &> /dev/null; then
    QMAKE_CMD="qmake6"
    echo "Using qmake6 from PATH"
elif [ -f "/opt/homebrew/opt/qt@6/bin/qmake" ]; then
    QMAKE_CMD="/opt/homebrew/opt/qt@6/bin/qmake"
    echo "Using Homebrew Qt 6 qmake"
elif [ -f "/opt/homebrew/opt/qt/bin/qmake" ]; then
    # Check if it's Qt 6
    VER=$("/opt/homebrew/opt/qt/bin/qmake" -query QT_VERSION 2>/dev/null || echo "")
    if [[ "$VER" == 6.* ]]; then
        QMAKE_CMD="/opt/homebrew/opt/qt/bin/qmake"
        echo "Using Homebrew Qt qmake"
    else
        echo "Error: Qt 6 not found at /opt/homebrew/opt/qt/bin/qmake (found Qt $VER)"
        exit 1
    fi
elif [ -f "/usr/local/opt/qt@6/bin/qmake" ]; then
    QMAKE_CMD="/usr/local/opt/qt@6/bin/qmake"
    echo "Using /usr/local Qt 6 qmake"
elif command -v qmake &> /dev/null; then
    VER=$(qmake -query QT_VERSION 2>/dev/null || echo "")
    if [[ "$VER" == 6.* ]]; then
        QMAKE_CMD="qmake"
        echo "Using qmake from PATH"
    else
        echo "Error: Qt 6 not found (found Qt $VER)"
        exit 1
    fi
else
    echo "Error: qmake6 or Qt 6 qmake not found."
    echo "Please install Qt 6 or set Qt Android path"
    exit 1
fi

echo "Using qmake: $QMAKE_CMD"
QT_VERSION=$($QMAKE_CMD -query QT_VERSION 2>/dev/null || echo "unknown")
echo "Qt Version: $QT_VERSION"

if [[ ! "$QT_VERSION" == 6.* ]]; then
    echo "Error: Qt 6 is required, but found Qt $QT_VERSION"
    echo "Please install Qt 6: brew install qt@6"
    echo "Or use qmake6 from Qt 6 installation"
    exit 1
fi

# Check Android SDK/NDK
if [ -z "$ANDROID_NDK_ROOT" ] && [ -z "$ANDROID_NDK" ]; then
    NDK_PATH="${HOME}/Library/Android/sdk/ndk"
    if [ -d "$NDK_PATH" ]; then
        NDK_VERSION=$(ls -1 "$NDK_PATH" | head -1)
        export ANDROID_NDK_ROOT="$NDK_PATH/$NDK_VERSION"
        export ANDROID_NDK="$NDK_PATH/$NDK_VERSION"
        echo "Using Android NDK: $ANDROID_NDK_ROOT"
    else
        echo "Warning: Android NDK not found. Please set ANDROID_NDK_ROOT"
    fi
fi

# Set Android SDK if not set
if [ -z "$ANDROID_SDK_ROOT" ]; then
    SDK_PATH="${HOME}/Library/Android/sdk"
    if [ -d "$SDK_PATH" ]; then
        export ANDROID_SDK_ROOT="$SDK_PATH"
        echo "Using Android SDK: $ANDROID_SDK_ROOT"
    fi
fi

# Find Android spec
QT_PREFIX=$($QMAKE_CMD -query QT_INSTALL_PREFIX)
QT_ANDROID_SPEC="$QT_PREFIX/mkspecs/android-clang"
if [ ! -d "$QT_ANDROID_SPEC" ]; then
    echo "Error: Android spec not found at $QT_ANDROID_SPEC"
    echo "Please install Qt Android components via Qt Maintenance Tool"
    exit 1
fi
echo "Using Android spec: $QT_ANDROID_SPEC"

# Build digitalpersonalib first
echo ""
echo "=========================================="
echo "Building digitalpersonalib for Android"
echo "=========================================="

cd digitalpersonalib
rm -rf build-android-${ARCH}
mkdir -p build-android-${ARCH}
cd build-android-${ARCH}

# Find Android spec
QT_ANDROID_SPEC=$(find "$($QMAKE_CMD -query QT_INSTALL_PREFIX)" -name "android-clang" -type d 2>/dev/null | head -1)
if [ -z "$QT_ANDROID_SPEC" ]; then
    QT_ANDROID_SPEC="android-clang"
fi

$QMAKE_CMD ../digitalpersonalib.pro \
    -spec "$QT_ANDROID_SPEC" \
    CONFIG+=android \
    ANDROID_ABIS="${ARCH}" \
    ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-${ANDROID_NDK}}" \
    ANDROID_PREFIX="${ANDROID_PREFIX}"

make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

cd "$SCRIPT_DIR"

# Build main application
echo ""
echo "=========================================="
echo "Building FingerprintApp for Android"
echo "=========================================="

BUILD_DIR="build/android-${ARCH}"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

$QMAKE_CMD ../../fingerprint_app.pro \
    -spec "$QT_ANDROID_SPEC" \
    CONFIG+=android \
    ANDROID_ABIS="${ARCH}" \
    ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-${ANDROID_NDK}}" \
    ANDROID_PREFIX="${ANDROID_PREFIX}"

make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo ""
echo "APK location:"
find . -name "*.apk" -type f | head -1
echo ""
echo "To deploy to device:"
echo "  adb install -r <path-to-apk>"
echo ""

