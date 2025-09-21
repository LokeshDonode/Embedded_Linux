# Embedded Linux QEMU Labs

This repository contains practical labs for learning **Embedded Linux System Development** on an ARM platform using **QEMU**.  
The labs are adapted from [Bootlin’s training materials](https://bootlin.com/training) and cover the entire workflow of building, configuring, and running an embedded Linux system.

---

## 📂 Repository Structure

Each directory corresponds to a specific lab:

- **`toolchain/`** – Build a cross-compiling toolchain with `crosstool-ng`.  
- **`bootloader/`** – Configure, compile, and run **U-Boot**.  
- **`kernel/`** – Fetch, configure, and cross-compile the **Linux kernel**.  
- **`tinysystem/`** – Build a minimal root filesystem with **BusyBox** and boot via **NFS**.  
- **`hardware/`** – Explore `/dev` and `/sys` devices in the target system.  
- **`filesystems/`** – Work with block storage (ext4, FAT, SquashFS, tmpfs).  
- **`thirdparty/`** – Cross-compile and integrate third-party libraries & applications (e.g., **ALSA**).  
- **`network/`** – Configure QEMU networking, TFTP, and test connectivity.  

Each lab contains **step-by-step instructions** and example commands.

---

## 🛠️ Requirements

- A Linux host (Ubuntu recommended)  
- At least **4 GB RAM** and **20 GB disk space**  
- Install required dependencies:

```bash
sudo apt update
sudo apt install build-essential git qemu-system-arm qemu-user \
  autoconf bison flex texinfo help2man gawk libtool-bin \
  libncurses5-dev unzip gettext python3 libssl-dev \
  device-tree-compiler swig python3-dev python3-setuptools
