# Barebox Bootloader – QEMU (ARM) Learning Setup

This repository documents how to **build, configure, and test the Barebox bootloader** using **QEMU on Linux**, targeting a **generic ARM (multi_v7)** platform.

The goal is to learn Barebox fundamentals without real hardware constraints (such as SRAM limits, complex ROM boot stages, or vendor-specific quirks).

---

## 📌 What is Barebox?

Barebox is a **modern, flexible bootloader for embedded Linux systems**. It sits between the **SoC ROM code** and the **Linux kernel**.

It provides:
* A Linux-like shell context
* Scriptable boot logic (via `/env`)
* A clean device model
* Strong support for custom boards

## 🧠 Why Use `multi_v7`?

While Barebox supports many **real hardware boards**, those targets often have strict SRAM size limits, require multi-stage boot mechanisms (PBL/X-Load), and can be difficult for beginners to debug.

The `multi_v7_defconfig` target is ideal for learning because it:
* Works seamlessly with QEMU.
* Has no strict SRAM overflow limits.
* Uses a simple single-stage boot process.
* Fully supports Linux boot testing.

---

## 🛠️ Prerequisites

### Host System
* **OS:** Linux (Ubuntu/Debian recommended)

### Install Packages
Run the following to install the cross-compiler, QEMU, and build dependencies:

```bash
sudo apt update
sudo apt install \
  gcc-arm-linux-gnueabihf \
  qemu-system-arm \
  bison flex \
  libssl-dev \
  bc \
  device-tree-compiler
```

---

## 🏗️ Build Steps

### 1. Verify Toolchain
Ensure the ARM cross-compiler is accessible:

```bash
arm-linux-gnueabihf-gcc --version
```

### 2. Get Barebox Source

```bash
git clone [https://github.com/barebox/barebox.git](https://github.com/barebox/barebox.git)
cd barebox
```

### 3. Configure Barebox
Set the architecture and cross-compiler environment variables, then clean and configure the project.

```bash
# Set environment variables
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

# Clean previous builds
make distclean

# Configure for QEMU (ARMv7 generic)
make multi_v7_defconfig
```

> **Optional:** To customize the build (e.g., enable debugging or specific drivers), run:
> ```bash
> make menuconfig
> ```

### 4. Compile
Build the bootloader using all available processor cores.

```bash
make -j$(nproc)
```

**Expected Artifact:**
The build process should produce the binary image at: `images/barebox.bin`

---

## ▶️ Run Barebox in QEMU

Execute the following command to boot the generated image in QEMU.

```bash
qemu-system-arm \
  -M virt \
  -cpu cortex-a15 \
  -m 512M \
  -nographic \
  -kernel images/barebox.bin
```

### Expected Output
You should see the Barebox startup log and shell prompt:

```text
barebox 20xx.xx
barebox@multi-v7:/
```

---

## 🧪 Basic Barebox Commands

Once inside the shell, try these commands to explore the environment:

* `help` - List available commands.
* `ls` - List files in the virtual filesystem.
* `devinfo` - Display device information.
* `printenv` - Show current environment variables.