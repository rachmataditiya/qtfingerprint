#!/bin/bash

echo "🔍 Checking USB Devices..."
echo ""

# Check on macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "📍 macOS Detected"
    echo ""
    
    echo "📱 DigitalPersona Fingerprint Reader:"
    ioreg -p IOUSB -l -w 0 | grep -A 30 "U.are.U\|DigitalPersona" | grep -E "USB Vendor Name|USB Product Name|idVendor|idProduct|locationID|kUSBSerialNumberString" || echo "  ⚠️  Device not found"
    
    echo ""
    echo "📊 All USB Devices:"
    system_profiler SPUSBDataType | grep -E "Product ID|Vendor ID|Manufacturer|Product" | head -20
    
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "📍 Linux Detected"
    echo ""
    
    if command -v lsusb &> /dev/null; then
        echo "📱 DigitalPersona Fingerprint Reader:"
        lsusb | grep -i "digital\|05ba" || echo "  ⚠️  Device not found"
        
        echo ""
        echo "📊 All USB Devices:"
        lsusb
    else
        echo "⚠️  lsusb not installed. Install with: sudo apt-get install usbutils"
    fi
else
    echo "⚠️  Unsupported OS: $OSTYPE"
fi

echo ""
echo "💡 For Android Emulator USB Passthrough:"
echo "   - See USB_PASSTHROUGH.md for detailed instructions"
echo "   - Recommended: Use physical Android device with USB OTG"
