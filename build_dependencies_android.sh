#!/bin/bash
set -e

# Script to build all dependencies for libfprint on Android NDK
# Dependencies: libusb, GLib, GUSB, pixman

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

export ANDROID_NDK="$NDK_FULL_PATH"
export PATH="$TOOLCHAIN/bin:$PATH"

# Architecture (default to arm64-v8a)
ARCH="${ANDROID_ARCH:-arm64-v8a}"
API_LEVEL="${ANDROID_API_LEVEL:-29}"

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
        exit 1
        ;;
esac

echo "=========================================="
echo "Building dependencies for Android NDK"
echo "=========================================="
echo "NDK: $NDK_FULL_PATH"
echo "Architecture: $ARCH"
echo "API Level: $API_LEVEL"
echo "Target Triple: $TARGET_TRIPLE"
echo "=========================================="

# Setup compiler with full path
export CC="${TOOLCHAIN}/bin/${TARGET_TRIPLE}${API_LEVEL}-clang"
export CXX="${TOOLCHAIN}/bin/${TARGET_TRIPLE}${API_LEVEL}-clang++"
export AR="${TOOLCHAIN}/bin/llvm-ar"
export STRIP="${TOOLCHAIN}/bin/llvm-strip"
export RANLIB="${TOOLCHAIN}/bin/llvm-ranlib"
export LD="${CC}"
export NM="${TOOLCHAIN}/bin/llvm-nm"
export OBJDUMP="${TOOLCHAIN}/bin/llvm-objdump"
export READELF="${TOOLCHAIN}/bin/llvm-readelf"

# Verify compiler exists
if [ ! -f "$CC" ]; then
    echo "Error: Compiler not found at $CC"
    exit 1
fi

# Android prefix (where we install everything)
ANDROID_PREFIX="${SCRIPT_DIR}/android-ndk-prefix"
mkdir -p "$ANDROID_PREFIX"/{lib,include,share,bin}

# Setup PKG_CONFIG paths
export PKG_CONFIG_LIBDIR="${ANDROID_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_PATH="${ANDROID_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="${ANDROID_PREFIX}"

# Common CFLAGS and LDFLAGS
export CFLAGS="-DANDROID -fPIC -ffunction-sections -funwind-tables -fstack-protector-strong -fno-addrsig --sysroot=${SYSROOT} -I${ANDROID_PREFIX}/include -I${SYSROOT}/usr/include -I${NDK_FULL_PATH}/sources/android/support/include"
export CPPFLAGS="-DANDROID -I${ANDROID_PREFIX}/include -I${SYSROOT}/usr/include"
export LDFLAGS="-L${ANDROID_PREFIX}/lib --sysroot=${SYSROOT} -Wl,--gc-sections -Wl,--build-id -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -landroid"

# Function to build a library
build_library() {
    local name=$1
    local url=$2
    local configure_args=$3
    local force_rebuild=${4:-false}  # Optional: force rebuild even if .so exists
    
    echo ""
    echo "=========================================="
    echo "Building $name"
    echo "=========================================="
    
    BUILD_DIR="${SCRIPT_DIR}/build-android-${ARCH}"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Check if library already exists and source hasn't changed
    if [ "$force_rebuild" != "true" ]; then
        # Check if .so file exists
        if [ -f "${ANDROID_PREFIX}/lib/lib${name}.so" ] || [ -f "${ANDROID_PREFIX}/lib/${name}.so" ]; then
            SO_FILE=$(find "${ANDROID_PREFIX}/lib" -name "*${name}*.so" -type f | head -1)
            if [ -n "$SO_FILE" ] && [ -d "$name" ]; then
                # Check if source directory is newer than .so file
                SO_MTIME=$(stat -f %m "$SO_FILE" 2>/dev/null || stat -c %Y "$SO_FILE" 2>/dev/null || echo 0)
                SRC_MTIME=$(find "$name" -type f -name "*.c" -o -name "*.h" -o -name "*.cpp" | xargs stat -f %m 2>/dev/null | sort -rn | head -1 || \
                           find "$name" -type f -name "*.c" -o -name "*.h" -o -name "*.cpp" | xargs stat -c %Y 2>/dev/null | sort -rn | head -1 || echo 0)
                if [ "$SRC_MTIME" -le "$SO_MTIME" ]; then
                    echo "✅ $name already built and up-to-date, skipping..."
                    cd "$SCRIPT_DIR"
                    return 0
                fi
            fi
        fi
    fi
    
    # Download and extract if needed
    if [ ! -d "$name" ]; then
        echo "Downloading $name..."
        if [[ "$url" == *.git ]]; then
            git clone "$url" "$name"
        else
            wget -q "$url" -O "${name}.tar.gz" || curl -L "$url" -o "${name}.tar.gz"
            tar -xzf "${name}.tar.gz"
            # Find extracted directory (might have version suffix)
            EXTRACTED_DIR=$(find . -maxdepth 1 -type d -name "${name}*" | head -1)
            if [ "$EXTRACTED_DIR" != "./$name" ]; then
                mv "$EXTRACTED_DIR" "$name"
            fi
        fi
    fi
    
    cd "$name"
    
    # Clean previous build only if forcing rebuild
    if [ "$force_rebuild" = "true" ]; then
        if [ -f "Makefile" ]; then
            make distclean || true
        fi
    fi
    
    # Configure
    echo "Configuring $name..."
    echo "CC=$CC"
    echo "CFLAGS=$CFLAGS"
    echo "LDFLAGS=$LDFLAGS"
    
    if [ -f "configure" ]; then
        # For autotools-based projects, we need to set CC and CXX explicitly
        CC="$CC" CXX="$CXX" AR="$AR" RANLIB="$RANLIB" STRIP="$STRIP" \
        ./configure \
            --host="${TARGET_TRIPLE}" \
            --prefix="${ANDROID_PREFIX}" \
            --enable-static=no \
            --enable-shared=yes \
            CC="$CC" \
            CXX="$CXX" \
            CFLAGS="$CFLAGS" \
            CPPFLAGS="$CPPFLAGS" \
            LDFLAGS="$LDFLAGS" \
            $configure_args
    elif [ -f "meson.build" ]; then
        # Use meson for meson-based projects
        MESON_BUILD_DIR="build-android"
        # Only remove build directory if forcing rebuild
        if [ "$force_rebuild" = "true" ] || [ ! -d "$MESON_BUILD_DIR" ]; then
            rm -rf "$MESON_BUILD_DIR"
        fi
        # Only reconfigure if build directory doesn't exist
        if [ ! -d "$MESON_BUILD_DIR" ]; then
            meson setup "$MESON_BUILD_DIR" \
                --cross-file <(cat <<EOF
[binaries]
c = '${CC}'
cpp = '${CXX}'
ar = '${AR}'
strip = '${STRIP}'
pkgconfig = 'pkg-config'

[properties]
c_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
cpp_args = ['-DANDROID', '-fPIC', '--sysroot=${SYSROOT}', '-I${ANDROID_PREFIX}/include']
c_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']
cpp_link_args = ['--sysroot=${SYSROOT}', '-L${ANDROID_PREFIX}/lib']

[host_machine]
system = 'android'
cpu_family = '${ARCH%%-*}'
cpu = '${ARCH%%-*}'
endian = 'little'
EOF
) \
                --prefix="${ANDROID_PREFIX}" \
                -Ddefault_library=shared \
                $configure_args
        fi
        cd "$MESON_BUILD_DIR"
    else
        echo "Error: No configure script or meson.build found for $name"
        exit 1
    fi
    
    # Build
    echo "Building $name..."
    if [ -f "Makefile" ]; then
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
        
        # Try make install, but if it fails, manually copy .so files
        if ! make install 2>&1 | tee /tmp/${name}_install.log; then
            echo "make install failed, trying manual copy..."
            # Find and copy .so files from .libs directories
            find . -path "*/.libs/*.so" -type f -exec cp {} "${ANDROID_PREFIX}/lib/" \;
            find . -path "*/.libs/*.so.*" -type f -exec cp {} "${ANDROID_PREFIX}/lib/" \;
            # Also check root .libs
            if [ -d ".libs" ]; then
                cp .libs/*.so "${ANDROID_PREFIX}/lib/" 2>/dev/null || true
                cp .libs/*.so.* "${ANDROID_PREFIX}/lib/" 2>/dev/null || true
            fi
            # Copy headers if they exist
            if [ -d "include" ]; then
                mkdir -p "${ANDROID_PREFIX}/include"
                cp -r include/* "${ANDROID_PREFIX}/include/" 2>/dev/null || true
            fi
            # Copy pkg-config files if they exist
            if [ -f "*.pc" ]; then
                mkdir -p "${ANDROID_PREFIX}/lib/pkgconfig"
                cp *.pc "${ANDROID_PREFIX}/lib/pkgconfig/" 2>/dev/null || true
            fi
        fi
    elif [ -f "build.ninja" ]; then
        # Only build if forcing rebuild or if .so doesn't exist
        if [ "$force_rebuild" = "true" ] || [ ! -f "${ANDROID_PREFIX}/lib/lib${name}.so" ] && [ ! -f "${ANDROID_PREFIX}/lib/${name}.so" ]; then
            ninja
            ninja install
        else
            echo "Skipping ninja build (already built)..."
            # Still run install in case headers changed
            ninja install || true
        fi
    else
        echo "Error: Build files not found for $name"
        exit 1
    fi
    
    # Verify .so file was created
    if [ -z "$(find "${ANDROID_PREFIX}/lib" -name "*${name}*.so*" 2>/dev/null)" ]; then
        echo "Warning: No .so file found for $name after build"
        echo "Searching in build directory..."
        find . -name "*.so" -type f | head -5
    else
        echo "✅ $name built successfully"
        ls -lh "${ANDROID_PREFIX}/lib"/*${name}*.so* 2>/dev/null | head -3
    fi
    
    cd "$SCRIPT_DIR"
}

# Build libusb
build_library "libusb" \
    "https://github.com/libusb/libusb/releases/download/v1.0.27/libusb-1.0.27.tar.bz2" \
    "--disable-udev --disable-static"

# Build pixman (disable all SIMD to avoid assembly issues)
# Note: pixman is optional for libfprint, we can skip it if it fails
build_library "pixman" \
    "https://www.cairographics.org/releases/pixman-0.42.2.tar.gz" \
    "--disable-gtk --disable-libpng --disable-timers --disable-arm-neon --disable-arm-iwmmxt --disable-arm-simd --disable-mmx --disable-sse2 --disable-ssse3 --disable-vmx" || echo "Warning: pixman build failed, continuing..."

# Build libffi if needed (required by GLib)
if [ ! -f "${ANDROID_PREFIX}/lib/libffi.so" ]; then
    echo ""
    echo "=========================================="
    echo "Building libffi (required by GLib)"
    echo "=========================================="
    build_library "libffi" \
        "https://github.com/libffi/libffi/releases/download/v3.4.6/libffi-3.4.6.tar.gz" \
        "--enable-shared --disable-static"
    
    # Manual fix: libffi sometimes doesn't create .so, create it manually
    if [ ! -f "${ANDROID_PREFIX}/lib/libffi.so" ]; then
        cd "${BUILD_DIR}/libffi"
        echo "Creating libffi.so manually..."
        # Find object files in subdirectories
        OBJ_FILES=$(find . -path "*/.libs/*.o" -type f | grep -E "(prep_cif|types|raw_api|java_raw_api|closures|tramp|ffi|sysv)" | sort)
        if [ -n "$OBJ_FILES" ]; then
            "${CC}" -shared -Wl,-soname,libffi.so -o "${ANDROID_PREFIX}/lib/libffi.so" \
                $OBJ_FILES ${LDFLAGS}
            echo "✅ libffi.so created manually"
            ls -lh "${ANDROID_PREFIX}/lib/libffi.so"
        else
            echo "Warning: Could not find object files for libffi"
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
fi

# Build GLib (uses meson, not autotools)
echo ""
echo "=========================================="
echo "Building GLib (using Meson)"
echo "=========================================="

BUILD_DIR="${SCRIPT_DIR}/build-android-${ARCH}"
mkdir -p "$BUILD_DIR"
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

# Clean previous build
rm -rf build-android

# Use meson for GLib
echo "Configuring GLib with Meson..."
# Create pkg-config file for libffi if it doesn't exist
if [ ! -f "${ANDROID_PREFIX}/lib/pkgconfig/libffi.pc" ]; then
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
fi

meson setup build-android \
    --cross-file <(cat <<EOF
[binaries]
c = '${CC}'
cpp = '${CXX}'
ar = '${AR}'
strip = '${STRIP}'
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

echo "Building GLib..."
cd build-android
ninja
ninja install
cd "$SCRIPT_DIR"

# Build json-glib (required by libgusb)
echo ""
echo "=========================================="
echo "Building json-glib (required by libgusb)"
echo "=========================================="

BUILD_DIR="${SCRIPT_DIR}/build-android-${ARCH}"
mkdir -p "$BUILD_DIR"
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

# Clean previous build
rm -rf build-android

# Use meson for json-glib
echo "Configuring json-glib with Meson..."
meson setup build-android \
    --cross-file <(cat <<EOF
[binaries]
c = '${CC}'
cpp = '${CXX}'
ar = '${AR}'
strip = '${STRIP}'
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

echo "Building json-glib..."
cd build-android
ninja
ninja install
cd "$SCRIPT_DIR"

# Build GUSB (uses meson, not autotools)
echo ""
echo "=========================================="
echo "Building libgusb (using Meson)"
echo "=========================================="

BUILD_DIR="${SCRIPT_DIR}/build-android-${ARCH}"
mkdir -p "$BUILD_DIR"
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

# Clean previous build
rm -rf build-android

# Use meson for libgusb
echo "Configuring libgusb with Meson..."
meson setup build-android \
    --cross-file <(cat <<EOF
[binaries]
c = '${CC}'
cpp = '${CXX}'
ar = '${AR}'
strip = '${STRIP}'
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

echo "Building libgusb..."
cd build-android
ninja
ninja install
cd "$SCRIPT_DIR"

# Build OpenSSL (required by libfprint uru4000 driver)
echo ""
echo "=========================================="
echo "Building OpenSSL (required by libfprint)"
echo "=========================================="

BUILD_DIR="${SCRIPT_DIR}/build-android-${ARCH}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -d "openssl" ]; then
    echo "Downloading OpenSSL..."
    curl -L "https://www.openssl.org/source/openssl-3.2.0.tar.gz" -o openssl.tar.gz 2>/dev/null || wget -q "https://www.openssl.org/source/openssl-3.2.0.tar.gz" -O openssl.tar.gz
    tar -xzf openssl.tar.gz
    mv openssl-* openssl
fi

cd openssl

# Clean previous build
make distclean 2>/dev/null || true

# Set PATH to include NDK toolchain
export PATH="${TOOLCHAIN}/bin:${PATH}"

# Configure OpenSSL for Android
echo "Configuring OpenSSL for Android..."
CC="${CC}" \
CXX="${CXX}" \
AR="${AR}" \
RANLIB="${TOOLCHAIN}/bin/llvm-ranlib" \
NM="${TOOLCHAIN}/bin/llvm-nm" \
STRIP="${STRIP}" \
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

echo "Building OpenSSL..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Installing OpenSSL..."
make install_sw

# Create pkg-config file for OpenSSL
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

echo ""
echo "=========================================="
echo "All dependencies built successfully!"
echo "=========================================="
echo "Installation prefix: $ANDROID_PREFIX"
echo ""
echo "Libraries installed:"
ls -lh "${ANDROID_PREFIX}/lib"/*.so 2>/dev/null || echo "No .so files found"
echo ""
echo "Next step: Run build_libfprint_android.sh to build libfprint"

