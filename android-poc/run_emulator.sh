#!/bin/bash

echo "========================================="
echo "  Running Android PoC on Emulator"
echo "========================================="

# Check if emulator is running
if ! adb devices | grep -q "emulator"; then
    echo "No emulator detected. Please start an Android emulator first."
    echo "You can start one from Android Studio: Tools > Device Manager"
    exit 1
fi

# Build APK
echo "Building APK..."
./gradlew assembleDebug

if [ $? -ne 0 ]; then
    echo "Build failed! Check errors above."
    exit 1
fi

# Install APK
echo "Installing APK on emulator..."
adb install -r app/build/outputs/apk/debug/app-debug.apk

if [ $? -ne 0 ]; then
    echo "Install failed! Make sure emulator is running."
    exit 1
fi

# Launch app
echo "Launching app..."
adb shell am start -n com.arkana.fingerprintpoc/.MainActivity

echo "========================================="
echo "  App launched! Check emulator screen"
echo "========================================="

