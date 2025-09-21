#!/bin/bash
set -e
IMG=sd.img
SIZE_MB=1024

echo "Creating ${IMG} (${SIZE_MB}MB) ..."
dd if=/dev/zero of=$IMG bs=1M count=$SIZE_MB

echo "Run cfdisk $IMG to partition (example: p1 FAT16 64MB boot, p2 ext4 8MB root, p3 ext4 rest)"
echo "After partitioning, use losetup --partscan to get /dev/loopXpY devices"
