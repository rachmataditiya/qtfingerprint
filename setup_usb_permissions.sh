#!/bin/bash

echo "========================================"
echo "   Setting up USB permissions for Linux"
echo "========================================"

# Check if running as root for some operations
if [ "$EUID" -eq 0 ]; then 
    echo "Please run this script as regular user (not root)."
    echo "You will be prompted for sudo password when needed."
    exit 1
fi

# Get current user
CURRENT_USER=$(whoami)
echo "Current user: $CURRENT_USER"

# 1. Add user to plugdev group
echo ""
echo "Step 1: Adding user to plugdev group..."
if groups | grep -q plugdev; then
    echo "User already in plugdev group."
else
    echo "Adding $CURRENT_USER to plugdev group..."
    sudo usermod -aG plugdev "$CURRENT_USER"
    echo "✓ User added to plugdev group."
    echo "  Note: You may need to log out and log back in for this to take effect."
fi

# 2. Create udev rules for fingerprint readers
echo ""
echo "Step 2: Setting up udev rules for fingerprint readers..."

UDEV_RULES_FILE="/etc/udev/rules.d/99-fingerprint-reader.rules"
UDEV_RULES_CONTENT="# Rules for DigitalPersona U.are.U fingerprint readers
# Digital Persona U.are.U 4000/4000B/4500
SUBSYSTEM==\"usb\", ATTR{idVendor}==\"05ba\", ATTR{idProduct}==\"0007\", MODE=\"0664\", GROUP=\"plugdev\"
SUBSYSTEM==\"usb\", ATTR{idVendor}==\"05ba\", ATTR{idProduct}==\"000a\", MODE=\"0664\", GROUP=\"plugdev\"
SUBSYSTEM==\"usb\", ATTR{idVendor}==\"05ba\", ATTR{idProduct}==\"000b\", MODE=\"0664\", GROUP=\"plugdev\"

# Generic USB fingerprint devices (backup rule)
SUBSYSTEM==\"usb\", ENV{DEVTYPE}==\"usb_device\", ATTR{idVendor}==\"05ba\", MODE=\"0664\", GROUP=\"plugdev\"
"

if [ -f "$UDEV_RULES_FILE" ]; then
    echo "Udev rules file already exists: $UDEV_RULES_FILE"
    read -p "Do you want to overwrite it? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Skipping udev rules creation."
    else
        echo "$UDEV_RULES_CONTENT" | sudo tee "$UDEV_RULES_FILE" > /dev/null
        echo "✓ Udev rules file created/updated."
    fi
else
    echo "Creating udev rules file..."
    echo "$UDEV_RULES_CONTENT" | sudo tee "$UDEV_RULES_FILE" > /dev/null
    echo "✓ Udev rules file created: $UDEV_RULES_FILE"
fi

# 3. Reload udev rules
echo ""
echo "Step 3: Reloading udev rules..."
sudo udevadm control --reload-rules
sudo udevadm trigger

echo "✓ Udev rules reloaded."

# 4. Verify current groups
echo ""
echo "Step 4: Verifying groups..."
echo "Current groups for $CURRENT_USER:"
groups

echo ""
echo "========================================"
echo "Setup complete!"
echo "========================================"
echo ""
echo "IMPORTANT:"
echo "1. If you were just added to plugdev group, please LOG OUT and LOG BACK IN"
echo "   for the group membership to take effect."
echo ""
echo "2. If your fingerprint reader is already connected, you may need to:"
echo "   - Unplug and replug the device, OR"
echo "   - Restart your computer"
echo ""
echo "3. To verify permissions, check:"
echo "   ls -l /dev/bus/usb/*/* | grep 05ba"
echo ""
echo "========================================"

