#!/bin/sh
# MppleOS Installer for Linux Rescue System

export LANG=C

echo "========================================"
echo "     MppleOS Installer"
echo "========================================"
echo "This tool will write MppleOS image (os.img)"
echo "to a target device (USB drive or hard disk)."
echo "All data on the target device will be DESTROYED."
echo ""

# Check for os.img in current directory
if [ ! -f "os.img" ]; then
    echo "Error: os.img not found in current directory."
    exit 1
fi

# List available disks
echo "Available disks:"
lsblk -d -o NAME,SIZE,MODEL | grep -v "loop"
echo ""

# Prompt for target device
printf "Enter target device (e.g., /dev/sdb): "
read DEVICE

# Validate device
if [ ! -b "$DEVICE" ]; then
    echo "Error: $DEVICE is not a valid block device."
    exit 1
fi

echo ""
echo "You selected: $DEVICE"
lsblk "$DEVICE"
echo "WARNING: All data on $DEVICE will be overwritten!"
printf "Type 'YES' to continue: "
read CONFIRM
if [ "$CONFIRM" != "YES" ]; then
    echo "Aborted."
    exit 0
fi

# Unmount any mounted partitions
umount ${DEVICE}?* 2>/dev/null

# Write the image
echo "Writing os.img to $DEVICE ..."
dd if=os.img of="$DEVICE" bs=512 count=2880 conv=notrunc status=progress
sync

echo "========================================"
echo "Installation complete!"
echo "You can now boot from $DEVICE."
echo "========================================"
echo "Press Enter to reboot, or Ctrl-C to exit."
read dummy
reboot