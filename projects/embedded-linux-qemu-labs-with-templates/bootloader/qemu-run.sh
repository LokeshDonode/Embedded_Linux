#!/bin/bash
set -e
qemu-system-arm -M vexpress-a9 -m 128M -nographic -kernel u-boot/u-boot
