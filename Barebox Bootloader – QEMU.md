# Barebox Bootloader – QEMU (ARM) Learning Setup

This repository documents how to **build, configure, and test the Barebox bootloader**
using **QEMU on Linux**, targeting a **generic ARM (multi_v7)** platform.

The goal is to learn Barebox fundamentals without real hardware constraints
(SRAM limits, ROM boot stages, vendor quirks).

---

## 📌 What is Barebox?

Barebox is a **modern, flexible bootloader for embedded Linux systems**.
It provides:

- A Linux-like shell
- Scriptable boot logic (`/env`)
- Clean device model
- Strong support for custom boards

Barebox sits between **SoC ROM code** and the **Linux kernel**.

---

## 🧠 Why Use `multi_v7`?

Barebox supports many **real hardware boards**, but those targets often:

- Have strict SRAM size limits
- Require multi-stage boot (PBL/X-Load)
- Are difficult for beginners

The `multi_v7_defconfig` target is ideal for learning because it:

- Works well with QEMU
- Has no SRAM overflow limits
- Uses a single-stage boot
- Supports Linux boot testing

---

## 🛠️ Prerequisites

### Host System
- Linux (Ubuntu/Debian recommended)

### Packages
```bash
sudo apt update
sudo apt install \
  gcc-arm-linux-gnueabihf \
  qemu-system-arm \
  bison flex \
  libssl-dev \
  bc \
  device-tree-compiler


# Verify toolchain:

$arm-linux-gnueabihf-gcc --version

📥 Get Barebox Source
$git clone https://github.com/barebox/barebox.git
$cd barebox

⚙️ Configure Barebox (Recommended)

# Set environment variables:
$export ARCH=arm
$export CROSS_COMPILE=arm-linux-gnueabihf-

# Clean previous builds:
$make distclean

# Configure for QEMU (ARMv7):
$make multi_v7_defconfig  -- for default configuration

# Optional customization:
$ make menuconfig

🏗️ Build Barebox
$make -j$(nproc)

# Expected output:

$images/barebox.bin

▶️ Run Barebox in QEMU
$qemu-system-arm \
  -M virt \
  -cpu cortex-a15 \
  -m 512M \
  -nographic \
  -kernel images/barebox.bin

# Expected console output:

barebox 20xx.xx
barebox@multi-v7:/

🧪 Basic Barebox Commands
$help
$ls /
$devinfo
$printenv




