#!/bin/bash
# Script to check which dependencies are already available in Android NDK

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

NDK_PATH="${ANDROID_NDK:-$HOME/Library/Android/sdk/ndk}"
NDK_VERSION=$(ls -1 "$NDK_PATH" | head -1)
NDK_FULL_PATH="$NDK_PATH/$NDK_VERSION"
TOOLCHAIN_PLATFORM="darwin-x86_64"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    TOOLCHAIN_PLATFORM="linux-x86_64"
fi
TOOLCHAIN="$NDK_FULL_PATH/toolchains/llvm/prebuilt/$TOOLCHAIN_PLATFORM"
SYSROOT="$TOOLCHAIN/sysroot"

echo "=========================================="
echo "Checking Android NDK Dependencies"
echo "=========================================="
echo "NDK: $NDK_FULL_PATH"
echo "Sysroot: $SYSROOT"
echo ""

# Check system libraries (already available)
echo "=== System Libraries (Available) ==="
echo "✅ libc - Available in Android NDK"
echo "✅ libm - Available in Android NDK"
echo "✅ libdl - Available in Android NDK"
echo "✅ liblog - Available in Android NDK"
echo "✅ libandroid - Available in Android NDK"
echo ""

# Check libffi (look for .so or .pc files)
echo "=== Checking libffi ==="
if find "$SYSROOT" -name "libffi.so*" -o -name "libffi.pc" 2>/dev/null | grep -q .; then
    echo "✅ libffi - Found in NDK"
    find "$SYSROOT" -name "libffi.so*" -o -name "libffi.pc" 2>/dev/null | head -3
else
    echo "❌ libffi - NOT found, needs to be built"
    NEEDS_BUILD="$NEEDS_BUILD libffi"
fi
echo ""

# Check GLib (look for .so or .pc files)
echo "=== Checking GLib ==="
if find "$SYSROOT" -name "libglib*.so*" -o -name "libgobject*.so*" -o -name "libgio*.so*" -o -name "glib-2.0.pc" -o -name "gobject-2.0.pc" -o -name "gio-2.0.pc" 2>/dev/null | grep -q .; then
    echo "✅ GLib - Found in NDK"
    find "$SYSROOT" -name "libglib*.so*" -o -name "libgobject*.so*" -o -name "libgio*.so*" -o -name "glib-2.0.pc" -o -name "gobject-2.0.pc" -o -name "gio-2.0.pc" 2>/dev/null | head -3
else
    echo "❌ GLib - NOT found, needs to be built"
    NEEDS_BUILD="$NEEDS_BUILD glib"
fi
echo ""

# Check GUSB (look for .so or .pc files)
echo "=== Checking GUSB ==="
if find "$SYSROOT" -name "libgusb*.so*" -o -name "gusb.pc" 2>/dev/null | grep -q .; then
    echo "✅ GUSB - Found in NDK"
    find "$SYSROOT" -name "libgusb*.so*" -o -name "gusb.pc" 2>/dev/null | head -3
else
    echo "❌ GUSB - NOT found, needs to be built"
    NEEDS_BUILD="$NEEDS_BUILD gusb"
fi
echo ""

# Check libusb (look for .so or .pc files)
echo "=== Checking libusb ==="
if find "$SYSROOT" -name "libusb*.so*" -o -name "libusb-1.0.pc" 2>/dev/null | grep -q .; then
    echo "✅ libusb - Found in NDK"
    find "$SYSROOT" -name "libusb*.so*" -o -name "libusb-1.0.pc" 2>/dev/null | head -3
else
    echo "❌ libusb - NOT found, needs to be built"
    NEEDS_BUILD="$NEEDS_BUILD libusb"
fi
echo ""

# Check pixman (optional)
echo "=== Checking pixman (optional) ==="
if find "$NDK_FULL_PATH" -name "*pixman*" 2>/dev/null | grep -q .; then
    echo "✅ pixman - Found in NDK"
    find "$NDK_FULL_PATH" -name "*pixman*" 2>/dev/null | head -3
else
    echo "⚠️  pixman - NOT found (optional, only needed for aes3k driver)"
    echo "   We're using uru4000 driver, so pixman is NOT required"
fi
echo ""

echo "=========================================="
echo "Summary"
echo "=========================================="
echo ""
echo "✅ Already Available (no build needed):"
echo "   - libc, libm, libdl, liblog, libandroid"
echo ""
if [ -z "$NEEDS_BUILD" ]; then
    echo "✅ All required dependencies are available!"
else
    echo "❌ Needs to be built:"
    for dep in $NEEDS_BUILD; do
        echo "   - $dep"
    done
    echo ""
    echo "Build order:"
    echo "   1. libffi (if needed for GLib)"
    echo "   2. libusb"
    echo "   3. GLib (glib-2.0, gobject-2.0, gio-2.0)"
    echo "   4. GUSB"
fi
echo ""

