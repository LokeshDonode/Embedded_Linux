# Buildroot Image with BusyBox

This project demonstrates how to build a minimal Linux system using [Buildroot](https://buildroot.org/) with [BusyBox](https://busybox.net/) as the core userspace.

## 📦 Requirements

Make sure you have the following installed:

- `git`
- `gcc` / `g++` (build tools)
- `make`
- `bison`, `flex`
- `libncurses-dev`
- `wget`, `tar`, `gzip`, `xz-utils`

On Debian/Ubuntu:

```sh
sudo apt update
sudo apt install -y build-essential bison flex libncurses-dev wget tar gzip xz-utils

## Getting Buildroot

git clone https://git.busybox.net/buildroot
$cd buildroot

## Configuring Buildroot

make menuconfig

In the menu:

  1. Target Architecture → choose your CPU (e.g., x86, arm).
  
  2. Toolchain → leave default (Buildroot will build its own).
  
  3. Target packages → make sure BusyBox is selected.
  
  4. Filesystem images → enable ext2/3/4 root filesystem and/or cpio (for initramfs).
  
  Save and exit.

## Build

make

This will:

  1. Download toolchain and source packages
  
  2. Build BusyBox
  
  3. Create a root filesystem
  
  4. Build the kernel (if enabled)

The output file appears in:
  
output/images/

For example:
    
    rootfs.ext2 → root filesystem image
    
    bzImage (for x86) → kernel image

## Running in QEMU (You can test the image using QEMU. For x86)

qemu-system-x86_64 -kernel output/images/bzImage \
  -hda output/images/rootfs.ext2 \
  -append "root=/dev/sda" \
  -nographic

Reference: 
https://buildroot.org/docs.html
https://busybox.net



