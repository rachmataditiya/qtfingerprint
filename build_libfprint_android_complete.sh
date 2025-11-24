#!/bin/bash
set -e

###############################################################################
# Complete Build Script for libfprint on Android NDK
# 
# This script builds all dependencies and libfprint for Android NDK.
# It can be used standalone in any project that needs libfprint.
#
# Usage:
#   ./build_libfprint_android_complete.sh [ARCH] [API_LEVEL]
#
# Examples:
#   ./build_libfprint_android_complete.sh                    # arm64-v8a, API 29
#   ./build_libfprint_android_complete.sh arm64-v8a 29        # Explicit
#   ./build_libfprint_android_complete.sh armeabi-v7a 21      # ARMv7, API 21
#
# Requirements:
#   - Android NDK installed
#   - meson, ninja, pkg-config installed
#   - curl or wget
#   - Python 3 with packaging module (pip install packaging)
#
# Output:
#   All libraries installed to: <SCRIPT_DIR>/android-ndk-prefix/
###############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Configuration
ARCH="${1:-arm64-v8a}"
API_LEVEL="${2:-29}"

# Android NDK setup
NDK_PATH="${ANDROID_NDK:-$HOME/Library/Android/sdk/ndk}"
if [ ! -d "$NDK_PATH" ]; then
    echo "Error: Android NDK not found at $NDK_PATH"
    echo "Please set ANDROID_NDK environment variable or install NDK to default location"
    echo "Example: export ANDROID_NDK=/path/to/android-ndk"
    exit 1
fi

NDK_VERSION=$(ls -1 "$NDK_PATH" | head -1)
NDK_FULL_PATH="$NDK_PATH/$NDK_VERSION"

# Detect toolchain platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    TOOLCHAIN_PLATFORM="darwin-x86_64"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    TOOLCHAIN_PLATFORM="linux-x86_64"
else
    echo "Error: Unsupported OS: $OSTYPE"
    exit 1
fi

TOOLCHAIN="$NDK_FULL_PATH/toolchains/llvm/prebuilt/$TOOLCHAIN_PLATFORM"
SYSROOT="$TOOLCHAIN/sysroot"

case "$ARCH" in
    arm64-v8a)
        TARGET_TRIPLE="aarch64-linux-android"
        ;;
    armeabi-v7a)
        TARGET_TRIPLE="armv7a-linux-androideabi"
        ;;
    x86_64)
        TARGET_TRIPLE="x86_64-linux-android"
        ;;
    *)
        echo "Error: Unsupported architecture: $ARCH"
        echo "Supported: arm64-v8a, armeabi-v7a, x86_64"
        exit 1
        ;;
esac

# Setup compiler
export CC="${TARGET_TRIPLE}${API_LEVEL}-clang"
export CXX="${TARGET_TRIPLE}${API_LEVEL}-clang++"
export AR="llvm-ar"
export STRIP="llvm-strip"
export RANLIB="llvm-ranlib"
export LD="ld.lld"

# Full paths
CC_FULL="${TOOLCHAIN}/bin/${CC}"
CXX_FULL="${TOOLCHAIN}/bin/${CXX}"
AR_FULL="${TOOLCHAIN}/bin/${AR}"
STRIP_FULL="${TOOLCHAIN}/bin/${STRIP}"
RANLIB_FULL="${TOOLCHAIN}/bin/${RANLIB}"

# Installation prefix
ANDROID_PREFIX="${SCRIPT_DIR}/android-ndk-prefix"
mkdir -p "$ANDROID_PREFIX"/{lib,include,share,bin,lib/pkgconfig}

# Setup PKG_CONFIG paths
export PKG_CONFIG_LIBDIR="${ANDROID_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_PATH="${ANDROID_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="${ANDROID_PREFIX}"

# Common CFLAGS and LDFLAGS
export CFLAGS="-DANDROID -fPIC -ffunction-sections -funwind-tables -fstack-protector-strong -fno-addrsig --sysroot=${SYSROOT} -I${ANDROID_PREFIX}/include -I${SYSROOT}/usr/include"
export CPPFLAGS="-DANDROID -I${ANDROID_PREFIX}/include -I${SYSROOT}/usr/include"
export LDFLAGS="-L${ANDROID_PREFIX}/lib --sysroot=${SYSROOT} -Wl,--gc-sections -Wl,--build-id -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -landroid"

# Export for subscripts
export NDK_FULL_PATH
export TOOLCHAIN
export SYSROOT
export ANDROID_PREFIX
export ARCH
export API_LEVEL
export TARGET_TRIPLE

echo "=========================================="
echo "libfprint Android NDK Build Script"
echo "=========================================="
echo "NDK: $NDK_FULL_PATH"
echo "Architecture: $ARCH"
echo "API Level: $API_LEVEL"
echo "Target Triple: $TARGET_TRIPLE"
echo "Prefix: $ANDROID_PREFIX"
echo "=========================================="
echo ""

# Check required tools
for tool in meson ninja pkg-config; do
    if ! command -v $tool &> /dev/null; then
        echo "Error: $tool is not installed"
        echo "Please install: brew install meson ninja pkg-config"
        exit 1
    fi
done

# Check Python packaging module
if ! python3 -c "import packaging" 2>/dev/null; then
    echo "Warning: Python packaging module not found"
    echo "Installing packaging module..."
    pip3 install --break-system-packages packaging 2>/dev/null || pip3 install --user packaging || {
        echo "Error: Failed to install packaging module"
        echo "Please run: pip3 install packaging"
        exit 1
    }
fi

# Create exe wrapper for Meson
EXE_WRAPPER="${SCRIPT_DIR}/android_exe_wrapper.sh"
cat > "$EXE_WRAPPER" <<'EOF'
#!/bin/sh
# Android exe wrapper - skip execution for cross-compiled binaries
exit 0
EOF
chmod +x "$EXE_WRAPPER"

# Build directory
BUILD_DIR="${SCRIPT_DIR}/build-android-${ARCH}"
mkdir -p "$BUILD_DIR"

###############################################################################
# Function: build_library (Autotools)
###############################################################################
build_library() {
    local name=$1
    local url=$2
    local configure_args=$3
    
    echo ""
    echo "=========================================="
    echo "Building $name"
    echo "=========================================="
    
    cd "$BUILD_DIR"
    
    # Download and extract if needed
    if [ ! -d "$name" ]; then
        echo "Downloading $name..."
        if [[ "$url" == *.git ]]; then
            git clone "$url" "$name"
        else
            local ext=$(echo "$url" | sed 's/.*\.\(tar\.[^.]*\|zip\)$/\1/')
            local archive="${name}.${ext}"
            curl -L "$url" -o "$archive" 2>/dev/null || wget -q "$url" -O "$archive"
            
            case "$ext" in
                tar.gz|tgz) tar -xzf "$archive" ;;
                tar.bz2) tar -xjf "$archive" ;;
                tar.xz) tar -xf "$archive" ;;
                zip) unzip -q "$archive" ;;
            esac
            
            # Find extracted directory
            EXTRACTED_DIR=$(find . -maxdepth 1 -type d -name "${name}*" | head -1)
            if [ "$EXTRACTED_DIR" != "./$name" ]; then
                mv "$EXTRACTED_DIR" "$name"
            fi
        fi
    fi
    
    cd "$name"
    
    # Clean previous build
    if [ -f "Makefile" ]; then
        make distclean || true
    fi
    
    # Configure
    echo "Configuring $name..."
    CC="$CC_FULL" \
    CXX="$CXX_FULL" \
    AR="$AR_FULL" \
    STRIP="$STRIP_FULL" \
    RANLIB="$RANLIB_FULL" \
    CFLAGS="$CFLAGS" \
    CPPFLAGS="$CPPFLAGS" \
    LDFLAGS="$LDFLAGS" \
    ./configure \
        --host="${TARGET_TRIPLE}" \
        --prefix="${ANDROID_PREFIX}" \
        --enable-shared \
        --disable-static \
        $configure_args
    
    # Build
    echo "Building $name..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    # Install (with fallback manual copy)
    if ! make install 2>&1 | tee /tmp/${name}_install.log; then
        echo "make install failed, trying manual copy..."
        find . -path "*/.libs/*.so" -type f -exec cp {} "${ANDROID_PREFIX}/lib/" \;
        find . -path "*/.libs/*.so.*" -type f -exec cp {} "${ANDROID_PREFIX}/lib/" \;
        if [ -d ".libs" ]; then
            cp .libs/*.so "${ANDROID_PREFIX}/lib/" 2>/dev/null || true
            cp .libs/*.so.* "${ANDROID_PREFIX}/lib/" 2>/dev/null || true
        fi
        if [ -d "include" ]; then
            mkdir -p "${ANDROID_PREFIX}/include"
            cp -r include/* "${ANDROID_PREFIX}/include/" 2>/dev/null || true
        fi
    fi
    
    # Verify .so file was created
    if [ -z "$(find "${ANDROID_PREFIX}/lib" -name "*${name}*.so*" 2>/dev/null)" ]; then
        echo "Warning: No .so file found for $name after build"
        find . -name "*.so" -type f | head -5
    else
        echo "✅ $name built successfully"
        ls -lh "${ANDROID_PREFIX}/lib"/*${name}*.so* 2>/dev/null | head -3
    fi
    
    cd "$SCRIPT_DIR"
}

###############################################################################
# Build Dependencies
###############################################################################

# Build libusb
build_library "libusb" \
    "https://github.com/libusb/libusb/releases/download/v1.0.27/libusb-1.0.27.tar.bz2" \
    "--disable-udev --disable-static"

# Copy libusb headers
mkdir -p "${ANDROID_PREFIX}/include/libusb-1.0"
cp -r "${BUILD_DIR}/libusb/libusb"/*.h "${ANDROID_PREFIX}/include/libusb-1.0/" 2>/dev/null || true

# Create libusb pkg-config file
cat > "${ANDROID_PREFIX}/lib/pkgconfig/libusb-1.0.pc" <<EOF
prefix=${ANDROID_PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: libusb-1.0
Description: C API for USB device access
Version: 1.0.27
Libs: -L\${libdir} -lusb-1.0
Cflags: -I\${includedir}/libusb-1.0
EOF

# Build libffi
build_library "libffi" \
    "https://github.com/libffi/libffi/releases/download/v3.4.6/libffi-3.4.6.tar.gz" \
    "--enable-shared --disable-static"

# Manual fix: libffi sometimes doesn't create .so
if [ ! -f "${ANDROID_PREFIX}/lib/libffi.so" ]; then
    cd "${BUILD_DIR}/libffi"
    echo "Creating libffi.so manually..."
    OBJ_FILES=$(find . -path "*/.libs/*.o" -type f | grep -E "(prep_cif|types|raw_api|java_raw_api|closures|tramp|ffi|sysv)" | sort)
    if [ -n "$OBJ_FILES" ]; then
        "${CC_FULL}" -shared -Wl,-soname,libffi.so -o "${ANDROID_PREFIX}/lib/libffi.so" \
            $OBJ_FILES ${LDFLAGS}
        echo "✅ libffi.so created manually"
    fi
    # Create pkg-config file
    mkdir -p "${ANDROID_PREFIX}/lib/pkgconfig"
    cat > "${ANDROID_PREFIX}/lib/pkgconfig/libffi.pc" <<EOF
prefix=${ANDROID_PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: libffi
Description: Library supporting Foreign Function Interfaces
Version: 3.4.6
Libs: -L\${libdir} -lffi
Cflags: -I\${includedir}
EOF
    cd "$SCRIPT_DIR"
fi

# Build GLib (uses meson)
echo ""
echo "=========================================="
echo "Building GLib (using Meson)"
echo "=========================================="

cd "$BUILD_DIR"

if [ ! -d "glib" ]; then
    echo "Downloading GLib..."
    curl -L "https://download.gnome.org/sources/glib/2.80/glib-2.80.0.tar.xz" -o glib.tar.xz 2>/dev/null || wget -q "https://download.gnome.org/sources/glib/2.80/glib-2.80.0.tar.xz" -O glib.tar.xz
    tar -xf glib.tar.xz
    EXTRACTED_DIR=$(find . -maxdepth 1 -type d -name "glib-*" | head -1)
    if [ "$EXTRACTED_DIR" != "./glib" ]; then
        mv "$EXTRACTED_DIR" glib
    fi
fi

cd glib
rm -rf build-android

meson setup build-android \
    --cross-file <(cat <<EOF
[binaries]
c = '${CC_FULL}'
cpp = '${CXX_FULL}'
ar = '${AR_FULL}'
strip = '${STRIP_FULL}'
pkg-config = 'pkg-config'

[built-in options]
c_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
cpp_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
c_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']
cpp_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']

[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
EOF
) \
    --prefix="${ANDROID_PREFIX}" \
    -Ddefault_library=shared \
    -Dselinux=disabled \
    -Dxattr=false \
    -Dlibmount=disabled \
    -Ddtrace=false \
    -Dsystemtap=false \
    -Dtests=false \
    -Dglib_debug=disabled \
    -Dglib_assert=false \
    -Dglib_checks=false \
    -Dnls=disabled \
    -Dman-pages=disabled \
    -Ddocumentation=false

cd build-android
ninja
ninja install
cd "$SCRIPT_DIR"

# Build json-glib (required by libgusb)
echo ""
echo "=========================================="
echo "Building json-glib (required by libgusb)"
echo "=========================================="

cd "$BUILD_DIR"

if [ ! -d "json-glib" ]; then
    echo "Downloading json-glib..."
    curl -L "https://download.gnome.org/sources/json-glib/1.8/json-glib-1.8.0.tar.xz" -o json-glib.tar.xz 2>/dev/null || wget -q "https://download.gnome.org/sources/json-glib/1.8/json-glib-1.8.0.tar.xz" -O json-glib.tar.xz
    tar -xf json-glib.tar.xz
    EXTRACTED_DIR=$(find . -maxdepth 1 -type d -name "json-glib-*" | head -1)
    if [ "$EXTRACTED_DIR" != "./json-glib" ]; then
        mv "$EXTRACTED_DIR" json-glib
    fi
fi

cd json-glib
rm -rf build-android

meson setup build-android \
    --cross-file <(cat <<EOF
[binaries]
c = '${CC_FULL}'
cpp = '${CXX_FULL}'
ar = '${AR_FULL}'
strip = '${STRIP_FULL}'
pkg-config = 'pkg-config'

[built-in options]
c_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
cpp_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
c_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']
cpp_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']

[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
EOF
) \
    --prefix="${ANDROID_PREFIX}" \
    -Ddefault_library=shared \
    -Dintrospection=disabled \
    -Dtests=false \
    -Dgtk_doc=disabled

cd build-android
ninja
ninja install
cd "$SCRIPT_DIR"

# Build libgusb
echo ""
echo "=========================================="
echo "Building libgusb (using Meson)"
echo "=========================================="

cd "$BUILD_DIR"

if [ ! -d "libgusb" ]; then
    echo "Downloading libgusb..."
    curl -L "https://github.com/hughsie/libgusb/releases/download/0.4.9/libgusb-0.4.9.tar.xz" -o libgusb.tar.xz 2>/dev/null || wget -q "https://github.com/hughsie/libgusb/releases/download/0.4.9/libgusb-0.4.9.tar.xz" -O libgusb.tar.xz
    tar -xf libgusb.tar.xz
    EXTRACTED_DIR=$(find . -maxdepth 1 -type d -name "libgusb-*" | head -1)
    if [ "$EXTRACTED_DIR" != "./libgusb" ]; then
        mv "$EXTRACTED_DIR" libgusb
    fi
fi

cd libgusb
rm -rf build-android

meson setup build-android \
    --cross-file <(cat <<EOF
[binaries]
c = '${CC_FULL}'
cpp = '${CXX_FULL}'
ar = '${AR_FULL}'
strip = '${STRIP_FULL}'
pkg-config = 'pkg-config'

[built-in options]
c_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
cpp_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
c_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']
cpp_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']

[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
EOF
) \
    --prefix="${ANDROID_PREFIX}" \
    -Ddefault_library=shared \
    -Dintrospection=false \
    -Dvapi=false \
    -Dtests=false \
    -Ddocs=false

cd build-android
ninja
ninja install
cd "$SCRIPT_DIR"

# Build OpenSSL
echo ""
echo "=========================================="
echo "Building OpenSSL (required by libfprint)"
echo "=========================================="

cd "$BUILD_DIR"

if [ ! -d "openssl" ]; then
    echo "Downloading OpenSSL..."
    curl -L "https://www.openssl.org/source/openssl-3.2.0.tar.gz" -o openssl.tar.gz 2>/dev/null || wget -q "https://www.openssl.org/source/openssl-3.2.0.tar.gz" -O openssl.tar.gz
    tar -xzf openssl.tar.gz
    mv openssl-* openssl
fi

cd openssl
make distclean 2>/dev/null || true

export PATH="${TOOLCHAIN}/bin:${PATH}"

CC="${CC_FULL}" \
CXX="${CXX_FULL}" \
AR="${AR_FULL}" \
RANLIB="${RANLIB_FULL}" \
NM="${TOOLCHAIN}/bin/llvm-nm" \
STRIP="${STRIP_FULL}" \
./Configure android-arm64 \
    --prefix="${ANDROID_PREFIX}" \
    --openssldir="${ANDROID_PREFIX}/ssl" \
    shared \
    no-asm \
    no-tests \
    -D__ANDROID_API__=${API_LEVEL} \
    --sysroot="${SYSROOT}" \
    -fPIC \
    ${CFLAGS} \
    ${LDFLAGS}

make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
make install_sw

# Create pkg-config files for OpenSSL
mkdir -p "${ANDROID_PREFIX}/lib/pkgconfig"
cat > "${ANDROID_PREFIX}/lib/pkgconfig/openssl.pc" <<EOF
prefix=${ANDROID_PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: OpenSSL
Description: Secure Sockets Layer and cryptography libraries
Version: 3.2.0
Requires: libssl libcrypto
Libs: -L\${libdir} -lssl -lcrypto
Cflags: -I\${includedir}
EOF

cat > "${ANDROID_PREFIX}/lib/pkgconfig/libssl.pc" <<EOF
prefix=${ANDROID_PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: OpenSSL-libssl
Description: Secure Sockets Layer and cryptography libraries
Version: 3.2.0
Requires: libcrypto
Libs: -L\${libdir} -lssl
Cflags: -I\${includedir}
EOF

cat > "${ANDROID_PREFIX}/lib/pkgconfig/libcrypto.pc" <<EOF
prefix=${ANDROID_PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: OpenSSL-libcrypto
Description: OpenSSL cryptography library
Version: 3.2.0
Libs: -L\${libdir} -lcrypto
Cflags: -I\${includedir}
EOF

cd "$SCRIPT_DIR"

###############################################################################
# Build libfprint
###############################################################################

echo ""
echo "=========================================="
echo "Building libfprint"
echo "=========================================="

# Check if libfprint source exists
LIBFPRINT_SOURCE="${SCRIPT_DIR}/libfprint_repo"
if [ ! -d "$LIBFPRINT_SOURCE" ]; then
    echo "Error: libfprint source not found at $LIBFPRINT_SOURCE"
    echo "Please clone libfprint repository first:"
    echo "  git clone https://gitlab.freedesktop.org/libfprint/libfprint.git libfprint_repo"
    exit 1
fi

cd "$LIBFPRINT_SOURCE"

# Create Meson cross-file
CROSS_FILE="${SCRIPT_DIR}/android-ndk-cross-file-${ARCH}.txt"
cat > "$CROSS_FILE" <<EOF
[binaries]
c = '${CC_FULL}'
cpp = '${CXX_FULL}'
ar = '${AR_FULL}'
strip = '${STRIP_FULL}'
pkg-config = 'pkg-config'
exe_wrapper = '${EXE_WRAPPER}'

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

ANDROID_BUILD_DIR="builddir-android-${ARCH}"
rm -rf "$ANDROID_BUILD_DIR"

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

cd "$ANDROID_BUILD_DIR"
ninja
ninja install
cd "$SCRIPT_DIR"

###############################################################################
# Summary
###############################################################################

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo ""
echo "Installation prefix: $ANDROID_PREFIX"
echo ""
echo "Libraries installed:"
ls -lh "${ANDROID_PREFIX}/lib"/*.so 2>/dev/null | awk '{printf "   %-35s %8s\n", $9, $5}'
echo ""
echo "Main library:"
ls -lh "${ANDROID_PREFIX}/lib/libfprint-2.so"
echo ""
echo "Headers:"
ls -1 "${ANDROID_PREFIX}/include/libfprint-2/" 2>/dev/null | head -5
echo ""
echo "Usage in Android project:"
echo "  1. Copy all .so files to: app/src/main/jniLibs/${ARCH}/"
echo "  2. Add to CMakeLists.txt:"
echo "     target_link_libraries(your_target fprint-2 glib-2.0 gobject-2.0 gio-2.0 gusb usb-1.0 ssl crypto ffi)"
echo "  3. Include headers:"
echo "     target_include_directories(your_target PRIVATE ${ANDROID_PREFIX}/include)"
echo ""

