#!/bin/sh
echo "=== Listing some devices in /dev (top 40) ==="
ls -l /dev | head -n 40
echo
echo "=== Network interfaces (/sys/class/net) ==="
ls -l /sys/class/net || true
echo
echo "=== MMC devices (/sys/bus/mmc/devices) ==="
ls -l /sys/bus/mmc/devices || true
echo
if [ -e /sys/class/net/eth0/address ]; then
  cat /sys/class/net/eth0/address
else
  echo "eth0 not present"
fi
