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
    
    # PostgreSQL support
    check_dep libpq-dev || MISSING_DEPS+=("libpq-dev")
    check_dep qt6-base-dev || MISSING_DEPS+=("qt6-base-dev") # Already checked, but needed for plugins
    
    # Check for PostgreSQL plugin
    QT_PLUGIN_PATH=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || qmake -query QT_INSTALL_PLUGINS 2>/dev/null || echo "")
    if [ -n "$QT_PLUGIN_PATH" ] && [ ! -f "$QT_PLUGIN_PATH/sqldrivers/libqsqlpsql.so" ]; then
        echo "Warning: PostgreSQL SQL driver not found. Installing qt6-base-dev should include it."
        echo "If missing, you may need to install qt6-tools-dev or compile it manually."
    fi

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
echo "========================================"

# Check USB permissions setup
echo ""
echo "Checking USB permissions setup..."
NEED_SETUP=false

# Check if user is in plugdev group
if ! groups | grep -q plugdev; then
    echo "⚠ Warning: User not in plugdev group."
    NEED_SETUP=true
fi

# Check if udev rules exist
if [ ! -f "/etc/udev/rules.d/99-fingerprint-reader.rules" ]; then
    echo "⚠ Warning: USB udev rules not found."
    NEED_SETUP=true
fi

if [ "$NEED_SETUP" = true ]; then
    echo ""
    echo "⚠ USB permissions not configured!"
    echo "Please run the USB permissions setup script:"
    echo "  ./setup_usb_permissions.sh"
    echo ""
    echo "After running the script, you may need to:"
    echo "  - Log out and log back in (for group membership)"
    echo "  - Unplug and replug your fingerprint reader"
    echo ""
else
    echo "✓ USB permissions appear to be configured."
fi
echo ""

