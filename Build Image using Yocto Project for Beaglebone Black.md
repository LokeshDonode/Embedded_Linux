# Build Custom Linux Image for BeagleBone Black using Yocto Project

## Overview
**BeagleBone Black** is a low-cost, ARM Cortex-A8-based development board.  
It provides the flexibility to boot from either an **SD card** or the **onboard eMMC**.

The **Yocto Project** officially supports BeagleBone Black and offers a minimal bootable BSP (Board Support Package).  
This guide explains how to build a **custom Linux image** for BeagleBone Black using the Yocto Project.

---
## 🧰 Prerequisites

Before starting, prepare your **host system** for Yocto Project development.

### Host Requirements
- **OS:** Ubuntu 16.04 LTS (or newer)
- **Disk Space:** Minimum 50 GB free
- **Internet Connection:** Required to download build dependencies and source packages

### Install Required Packages
Run the following command on your Ubuntu host
# Install the below required package on your host machine
$ sudo apt-get install gawk wget git-core diffstat unzip texinfo gcc-multilib \
build-essential chrpath socat cpio python python3 python3-pip \
python3-pexpect xz-utils debianutils iputils-ping libsdl1.2-dev xterm

## Download Poky
Poky is the Yocto reference distribution.

#Clone the Poky repository
$ git clone git://git.yoctoproject.org/poky

<img width="721" height="136" alt="image" src="https://github.com/user-attachments/assets/f8e7f984-468a-4e3a-9a42-b03705dbc4e7" />

## Setup build environment
Run the "oe-init-build-env" script with the below command from the poky directory and it will auto-create the build directory.
Now your current working directory would be the build directory.

# Setup the environment
$ source oe-init-build-env

<img width="878" height="346" alt="image" src="https://github.com/user-attachments/assets/d4bf3947-d85d-4c7a-a724-f354df07dcd4" />

# Set machine name in local.conf
We are building the image for the beaglebone black board so you need to set the machine name in conf/local.conf

* local.conf is auto-generated inside build/conf location.
* This file is used for the user-based configuration.
  
#Open local.conf file
$ vim conf/local.conf

#For Beaglebone, machine variable should be beaglebone-yocto
MACHINE ??= “beaglebone-yocto”

<img width="998" height="463" alt="image" src="https://github.com/user-attachments/assets/36ce94f5-eea6-4c48-a10b-95f0e7afe0af" />

# Start bitbake to build the image
Bitbake is a task executor in the Yocto Project.

* core-minimal-image recipe provides the minimal bootable image for the beaglebone black.
* Now we are ready to lunch bitbake (Yocto build engine) to create the image. it takes several hours to complete the build also depends on your internet speed.
* Once the build is completed successfully, you will get the image, root file system, and device tree file at the build/tmp/deploy/images/beaglebone-yocto/ directory.

# Run bitbake 
$ bitbake core-minimal-image

<img width="915" height="463" alt="image" src="https://github.com/user-attachments/assets/8c8b896b-d793-4ef4-a579-d58562bd375b" />

# Create SD card Partition to boot the board
Create the partition in the SD card and copy the generated file in those partitions.

You can use GParted to create the partition.

1. Create the first partition with the following details.
  * Name of the partition - BOOT
  * Size - 150 MB.
  * Flag - Set the boot flag
  * Type - FAT32
2. Create 2nd partition with the below details.
  * Name of the partition - ROOT.
  * Size - greater than 1024 MB or remaining space of SD card.
  * Type- ext4
    
# Copy generated image in SD card partition
Make sure your SD card is mounted on the host pc and you can access both SD card partitions.

#copy files from build/tmp/deploy/images/beaglebone-yocto/ to SD card boot and root partition.
$ cp MLO-beaglebone-yocto /media/tutorialadda/BOOT/
$ cp u-boot.img /media/tutorialadda/BOOT/
$ sudo tar -xvf core-image-minimal-beaglebone-yocto.tar.bz2 /media/tutorialadda/ROOT/

# Booting
Follow the below steps and boot the board with your Custom Linux image.

* Insert SD card into beaglebone black card slot.
* Connect the serial cable to monitor bootup logs and login to beaglebone black.
* Press the S2 button while connecting power to beagle bone.
  
You will get the bootup log on the serial console.
Poky (Yocto Project Reference Distro) 2.6.4 beaglebone /dev/ttyUSB0
beaglebone login: root
root@beaglebone:~#





