#!/bin/bash
set -e
KERNEL=zImage
DTB=vexpress-v2p-ca9.dtb
if [ ! -f "$KERNEL" ] || [ ! -f "$DTB" ]; then
  echo "Missing zImage or DTB. Place zImage and vexpress-v2p-ca9.dtb here."
  exit 1
fi

qemu-system-arm -M vexpress-a9 -m 256M -nographic \
  -kernel "$KERNEL" -dtb "$DTB" \
  -append "console=ttyAMA0 root=/dev/nfs rw ip=dhcp"
