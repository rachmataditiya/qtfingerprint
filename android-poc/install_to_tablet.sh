#!/bin/bash

echo "========================================="
echo "  Installing Fingerprint PoC to Tablet"
echo "========================================="

APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
API_URL="http://192.168.1.2:3000"

# Check if APK exists
if [ ! -f "$APK_PATH" ]; then
    echo "❌ APK not found. Building..."
    ./gradlew assembleDebug
fi

echo ""
echo "📱 Installing APK to connected device..."
echo ""

# Install APK
adb install -r "$APK_PATH"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ APK installed successfully!"
    echo ""
    echo "🌐 Server Configuration:"
    echo "   API URL: $API_URL"
    echo ""
    echo "📋 Next Steps:"
    echo "   1. Open app on tablet"
    echo "   2. Tap 'Initialize Reader'"
    echo "   3. Tap 'Refresh' to load users from server"
    echo ""
    echo "🔍 Verify server is accessible:"
    echo "   Test in browser on tablet: $API_URL/health"
    echo ""
    
    # Ask if user wants to launch app
    read -p "Launch app now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        adb shell am start -n com.arkana.fingerprintpoc/.MainActivity
        echo "App launched!"
    fi
else
    echo ""
    echo "❌ Installation failed!"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Check if device is connected: adb devices"
    echo "  2. Enable USB debugging on tablet"
    echo "  3. Allow USB debugging authorization"
fi

