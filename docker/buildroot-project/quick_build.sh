#!/bin/bash

echo "=== Starting Minimal Linux Build ==="

# Apply minimal configuration
make minimal_linux_defconfig

echo "Configuration applied. Starting build..."
echo "This may take 15-30 minutes..."

# Build with limited parallel jobs for stability
make -j2

if [ $? -eq 0 ]; then
    echo "========================================="
    echo "Build completed SUCCESSFULLY!"
    echo "========================================="
    echo "Output files:"
    echo "  Kernel: output/images/bzImage"
    echo "  Rootfs: output/images/rootfs.cpio"
    echo "  Tarball: output/images/rootfs.tar"
    echo "========================================="
    
    # Show build summary
    ls -la output/images/
else
    echo "========================================="
    echo "Build FAILED!"
    echo "Check logs in output/build/ for details"
    echo "========================================="
    exit 1
fi
