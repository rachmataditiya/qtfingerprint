#!/bin/bash
set -e

# Script to build libfprint for Android NDK
# Prerequisites: Dependencies must be built first using build_dependencies_android.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Android NDK setup
NDK_PATH="${ANDROID_NDK:-$HOME/Library/Android/sdk/ndk}"
if [ ! -d "$NDK_PATH" ]; then
    echo "Error: Android NDK not found at $NDK_PATH"
    echo "Please set ANDROID_NDK environment variable or install NDK to default location"
    exit 1
fi

NDK_VERSION=$(ls -1 "$NDK_PATH" | head -1)
NDK_FULL_PATH="$NDK_PATH/$NDK_VERSION"
TOOLCHAIN="$NDK_FULL_PATH/toolchains/llvm/prebuilt/darwin-x86_64"

# Check if running on Linux
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    TOOLCHAIN="$NDK_FULL_PATH/toolchains/llvm/prebuilt/linux-x86_64"
fi

if [ ! -d "$TOOLCHAIN" ]; then
    echo "Error: Toolchain not found at $TOOLCHAIN"
    exit 1
fi

export ANDROID_NDK="$NDK_FULL_PATH"
export PATH="$TOOLCHAIN/bin:$PATH"

# Architecture (default to arm64-v8a)
ARCH="${ANDROID_ARCH:-arm64-v8a}"
API_LEVEL="${ANDROID_API_LEVEL:-29}"

case "$ARCH" in
    arm64-v8a)
        TARGET_TRIPLE="aarch64-linux-android"
        SYSROOT="$NDK_FULL_PATH/toolchains/llvm/prebuilt/darwin-x86_64/sysroot"
        ;;
    armeabi-v7a)
        TARGET_TRIPLE="armv7a-linux-androideabi"
        SYSROOT="$NDK_FULL_PATH/toolchains/llvm/prebuilt/$(uname -m | sed 's/x86_64/linux-x86_64/;s/amd64/linux-x86_64/')/sysroot"
        ;;
    x86_64)
        TARGET_TRIPLE="x86_64-linux-android"
        SYSROOT="$NDK_FULL_PATH/toolchains/llvm/prebuilt/$(uname -m | sed 's/x86_64/linux-x86_64/;s/amd64/linux-x86_64/')/sysroot"
        ;;
    *)
        echo "Error: Unsupported architecture: $ARCH"
        exit 1
        ;;
esac

echo "=========================================="
echo "Building libfprint for Android NDK"
echo "=========================================="
echo "NDK: $NDK_FULL_PATH"
echo "Architecture: $ARCH"
echo "API Level: $API_LEVEL"
echo "Target Triple: $TARGET_TRIPLE"
echo "=========================================="

# Check if dependencies are built
ANDROID_PREFIX="${SCRIPT_DIR}/android-ndk-prefix"
if [ ! -d "$ANDROID_PREFIX" ] || [ ! -f "$ANDROID_PREFIX/lib/libglib-2.0.so" ]; then
    echo "Error: Dependencies not found at $ANDROID_PREFIX"
    echo "Please run build_dependencies_android.sh first"
    exit 1
fi

# Setup compiler
export CC="${TARGET_TRIPLE}${API_LEVEL}-clang"
export CXX="${TARGET_TRIPLE}${API_LEVEL}-clang++"
export AR="llvm-ar"
export STRIP="llvm-strip"
export RANLIB="llvm-ranlib"

# Setup PKG_CONFIG paths
export PKG_CONFIG_LIBDIR="${ANDROID_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_PATH="${ANDROID_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="${ANDROID_PREFIX}"

# Create Android build directory
ANDROID_BUILD_DIR="libfprint_repo/builddir-android-${ARCH}"
mkdir -p "$ANDROID_BUILD_DIR"

# Create Meson cross-file dynamically
CROSS_FILE="${SCRIPT_DIR}/android-ndk-cross-file-${ARCH}.txt"
cat > "$CROSS_FILE" <<EOF
[binaries]
c = '${TOOLCHAIN}/bin/${CC}'
cpp = '${TOOLCHAIN}/bin/${CXX}'
ar = '${TOOLCHAIN}/bin/${AR}'
strip = '${TOOLCHAIN}/bin/${STRIP}'
pkgconfig = 'pkg-config'
exe_wrapper = '${SCRIPT_DIR}/android_exe_wrapper.sh'

[built-in options]
c_args = ['-DANDROID', '-fPIC', '-ffunction-sections', '-funwind-tables', '-fstack-protector-strong', '-fno-addrsig', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
cpp_args = ['-DANDROID', '-fPIC', '-ffunction-sections', '-funwind-tables', '-fstack-protector-strong', '-fno-addrsig', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include', '-std=c++17']
c_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib', '-Wl,--gc-sections', '-Wl,--build-id', '-Wl,--no-undefined', '-Wl,-z,noexecstack', '-Wl,-z,relro', '-Wl,-z,now', '-landroid']
cpp_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib', '-Wl,--gc-sections', '-Wl,--build-id', '-Wl,--no-undefined', '-Wl,-z,noexecstack', '-Wl,-z,relro', '-Wl,-z,now', '-landroid']

[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
EOF

# Configure libfprint with Meson
cd libfprint_repo

if [ ! -f "meson.build" ]; then
    echo "Error: libfprint_repo/meson.build not found"
    exit 1
fi

# Check if meson is installed
if ! command -v meson &> /dev/null; then
    echo "Error: meson is not installed. Please install meson build system."
    exit 1
fi

echo "Configuring libfprint with Meson..."
meson setup "$ANDROID_BUILD_DIR" \
    --cross-file "$CROSS_FILE" \
    --prefix "$ANDROID_PREFIX" \
    -Ddrivers=uru4000 \
    -Dintrospection=false \
    -Dudev_rules=disabled \
    -Dudev_hwdb=disabled \
    -Ddoc=false \
    -Ddefault_library=shared \
    -Dbuildtype=release

echo "Building libfprint..."
cd "$ANDROID_BUILD_DIR"
ninja

echo "Installing libfprint..."
ninja install

echo ""
echo "=========================================="
echo "libfprint build complete!"
echo "=========================================="
echo "Library installed to: $ANDROID_PREFIX/lib/libfprint-2.so"
echo "Headers installed to: $ANDROID_PREFIX/include/libfprint-2"
echo ""
echo "Next steps:"
echo "1. Copy libraries to Android project:"
echo "   cp $ANDROID_PREFIX/lib/*.so android-poc/app/src/main/jniLibs/${ARCH}/"
echo "2. Update CMakeLists.txt to link against libfprint-2"
