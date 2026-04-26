#!/bin/bash

echo "=== Testing Built System with QEMU ==="

# Check if required files exist
if [ ! -f "output/images/bzImage" ] || [ ! -f "output/images/rootfs.cpio" ]; then
    echo "ERROR: Required files not found!"
    echo "Run quick_build.sh first"
    exit 1
fi

echo "Starting QEMU. Type 'exit' or press Ctrl+A then X to quit."

qemu-system-x86_64 \
    -kernel output/images/bzImage \
    -initrd output/images/rootfs.cpio \
    -nographic \
    -append "console=ttyS0"

echo "QEMU session ended."
