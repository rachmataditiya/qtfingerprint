#!/bin/bash

echo "========================================="
echo "  Android Emulator with USB Passthrough"
echo "========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macOS"
    echo "📍 Detected: macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="Linux"
    echo "📍 Detected: Linux"
else
    echo -e "${RED}⚠️  Unsupported OS: $OSTYPE${NC}"
    exit 1
fi

echo ""

# Function to find USB device info
find_usb_device() {
    local vendor_id="1466"  # DigitalPersona
    local product_id="10"   # U.are.U 4500
    
    if [[ "$OS" == "Linux" ]]; then
        # Linux: use lsusb
        if ! command -v lsusb &> /dev/null; then
            echo -e "${RED}❌ lsusb not found. Install with: sudo apt-get install usbutils${NC}"
            return 1
        fi
        
        local device=$(lsusb | grep -i "${vendor_id}:${product_id}\|digitalpersona")
        if [[ -z "$device" ]]; then
            echo -e "${RED}❌ Fingerprint reader not found!${NC}"
            return 1
        fi
        
        echo -e "${GREEN}✅ Device found:${NC}"
        echo "   $device"
        echo ""
        
        # Extract bus and device number
        # Format: Bus 001 Device 023: ID 05ba:000a ...
        local bus=$(echo "$device" | sed -n 's/.*Bus \([0-9]*\).*/\1/p')
        local addr=$(echo "$device" | sed -n 's/.*Device \([0-9]*\).*/\1/p')
        
        if [[ -z "$bus" || -z "$addr" ]]; then
            echo -e "${RED}❌ Could not extract bus/address${NC}"
            return 1
        fi
        
        echo "📊 USB Device Info:"
        echo "   Bus: $bus"
        echo "   Address: $addr"
        echo "   Vendor ID: 0x05ba (1466)"
        echo "   Product ID: 0x000a (10)"
        echo ""
        
        USB_BUS=$bus
        USB_ADDR=$addr
        return 0
        
    elif [[ "$OS" == "macOS" ]]; then
        # macOS: use ioreg
        local device_info=$(ioreg -p IOUSB -l -w 0 | grep -A 30 "U.are.U\|DigitalPersona" | grep -E "idVendor|idProduct|locationID")
        
        if [[ -z "$device_info" ]]; then
            echo -e "${RED}❌ Fingerprint reader not found!${NC}"
            return 1
        fi
        
        echo -e "${GREEN}✅ Device found: U.are.U 4500 Fingerprint Reader${NC}"
        echo ""
        
        # Extract locationID (macOS uses locationID instead of bus/addr)
        local location_id=$(echo "$device_info" | grep "locationID" | head -1 | sed -E 's/.*"locationID" = ([0-9]+).*/\1/')
        local vendor=$(echo "$device_info" | grep "idVendor" | sed -E 's/.*"idVendor" = ([0-9]+).*/\1/')
        local product=$(echo "$device_info" | grep "idProduct" | sed -E 's/.*"idProduct" = ([0-9]+).*/\1/')
        
        if [[ -z "$location_id" ]]; then
            echo -e "${RED}❌ Could not extract locationID${NC}"
            return 1
        fi
        
        echo "📊 USB Device Info:"
        echo "   Location ID: $location_id"
        echo "   Vendor ID: $vendor (0x$(printf '%04x' $vendor))"
        echo "   Product ID: $product (0x$(printf '%04x' $product))"
        echo ""
        
        # For macOS, we might need vendor/product ID instead
        USB_VENDOR=$(printf '%04x' $vendor)
        USB_PRODUCT=$(printf '%04x' $product)
        USB_LOCATION=$location_id
        return 0
    fi
}

# Find USB device
echo "🔍 Searching for fingerprint reader..."
find_usb_device

if [[ $? -ne 0 ]]; then
    exit 1
fi

# List available AVDs
echo "📱 Available Android Virtual Devices:"
emulator -list-avds 2>&1 | nl -w 2 -s '. '
echo ""

# Ask for AVD name
read -p "Enter AVD name (or press Enter for first one): " AVD_NAME

if [[ -z "$AVD_NAME" ]]; then
    AVD_NAME=$(emulator -list-avds 2>&1 | head -1)
fi

if [[ -z "$AVD_NAME" ]]; then
    echo -e "${RED}❌ No AVD found! Create one in Android Studio first.${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}✅ Selected AVD: $AVD_NAME${NC}"
echo ""

# Build QEMU command
if [[ "$OS" == "Linux" ]]; then
    echo "🚀 Starting emulator with USB passthrough..."
    echo ""
    echo "Command:"
    echo "  emulator -avd $AVD_NAME \\"
    echo "    -qemu -usb \\"
    echo "    -device usb-host,hostbus=$USB_BUS,hostaddr=$USB_ADDR"
    echo ""
    echo -e "${YELLOW}⚠️  Note: You may need to run with sudo${NC}"
    echo ""
    
    # Auto-start without prompt
    echo "🚀 Starting emulator in background..."
    nohup emulator -avd "$AVD_NAME" \
        -qemu -usb \
        -device usb-host,hostbus=$USB_BUS,hostaddr=$USB_ADDR > /tmp/emulator_usb.log 2>&1 &
    
    EMULATOR_PID=$!
    echo "✅ Emulator started (PID: $EMULATOR_PID)"
    echo "📋 Log: tail -f /tmp/emulator_usb.log"
    echo ""
    echo "⏳ Waiting for emulator to boot (this may take 30-60 seconds)..."
    
    # Wait for device
    timeout 60 adb wait-for-device 2>/dev/null
    
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✅ Emulator is ready!${NC}"
        adb devices
        echo ""
        echo "🔍 Check USB device in emulator:"
        echo "   adb shell ls /dev/bus/usb/*/*"
    else
        echo -e "${YELLOW}⚠️  Emulator taking longer than expected. Check log:${NC}"
        echo "   tail -f /tmp/emulator_usb.log"
    fi
    
elif [[ "$OS" == "macOS" ]]; then
    echo "🚀 Starting emulator with USB passthrough..."
    echo ""
    echo "⚠️  macOS Emulator Limitations:"
    echo "   - Android emulator on macOS uses Hypervisor.framework"
    echo "   - Direct QEMU USB passthrough may not work"
    echo "   - Alternative: Use vendor/product ID instead"
    echo ""
    
    echo "Command (try this):"
    echo "  emulator -avd $AVD_NAME \\"
    echo "    -qemu -usb \\"
    echo "    -device usb-host,vendorid=0x$USB_VENDOR,productid=0x$USB_PRODUCT"
    echo ""
    echo "Or try with locationID (if supported):"
    echo "  emulator -avd $AVD_NAME \\"
    echo "    -qemu -usb \\"
    echo "    -device usb-host,hostbus=1,hostaddr=$USB_LOCATION"
    echo ""
    
    # Auto-start without prompt
    echo "🚀 Starting emulator in background..."
    nohup emulator -avd "$AVD_NAME" \
        -qemu -usb \
        -device usb-host,vendorid=0x$USB_VENDOR,productid=0x$USB_PRODUCT > /tmp/emulator_usb.log 2>&1 &
    
    EMULATOR_PID=$!
    echo "✅ Emulator started (PID: $EMULATOR_PID)"
    echo "📋 Log: tail -f /tmp/emulator_usb.log"
    echo ""
    echo "⏳ Waiting for emulator to boot (this may take 30-60 seconds)..."
    
    # Wait for device
    timeout 60 adb wait-for-device 2>/dev/null
    
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✅ Emulator is ready!${NC}"
        adb devices
        echo ""
        echo "🔍 Check USB device in emulator:"
        echo "   adb shell ls /dev/bus/usb/*/*"
    else
        echo -e "${YELLOW}⚠️  Emulator taking longer than expected. Check log:${NC}"
        echo "   tail -f /tmp/emulator_usb.log"
    fi
fi

echo ""
echo "========================================="
echo "  For more info, see USB_PASSTHROUGH.md"
echo "========================================="

