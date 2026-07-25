# GPIO Character Device Driver (STM32MP157 - PB10)

## Overview

This project demonstrates a simple Linux Character Device Driver to control a GPIO pin from user space. The driver creates a character device `/dev/my_device` and allows the user to control GPIO PB10 by writing `1` or `0` to the device.

The driver was developed and tested on an STM32MP157 Embedded Linux platform running Linux Kernel `4.19.9-stm32-r1`.

---

## Features

- Loadable Kernel Module (LKM)
- Character Device Driver
- Dynamic Major Number Allocation
- GPIO Request and Configuration
- GPIO Read and Write Support
- User Space Interface via `/dev/my_device`
- Proper Resource Cleanup on Module Removal

---

## Hardware Platform

- Processor : STM32MP157
- Kernel    : 4.19.9-stm32-r1
- GPIO Used : PB10
- Global GPIO Number : 26

---

## Driver Flow

```
Application
      |
      |
echo 1 > /dev/my_device
      |
      |
Virtual File System (VFS)
      |
      |
File Operations
(open/read/write/release)
      |
      |
copy_from_user()
      |
      |
gpio_set_value()
      |
      |
GPIO Hardware (PB10)
```

---

## File Operations Implemented

- open()
- release()
- read()
- write()

---

## Driver Initialization

The module performs the following steps:

1. Allocate Character Device Number
2. Initialize cdev
3. Register cdev
4. Create Device Class
5. Create Device Node
6. Request GPIO
7. Configure GPIO as Output
8. Export GPIO
9. Driver Ready

---

## Driver Exit

The module performs the following cleanup:

- Unexport GPIO
- Free GPIO
- Destroy Device
- Destroy Class
- Delete cdev
- Unregister Character Device Number

---

## Build

```bash
make
```

Clean

```bash
make clean
```

---

## Load Driver

```bash
sudo insmod gpio_driver.ko
```

Verify

```bash
dmesg | tail
```

Expected

```
GPIO Driver Loaded Successfully
```

---

## Check Module

```bash
lsmod | grep gpio_driver
```

---

## Check Device Node

```bash
ls -l /dev/my_device
```

---

## Turn GPIO ON

```bash
echo 1 | sudo tee /dev/my_device
```

---

## Turn GPIO OFF

```bash
echo 0 | sudo tee /dev/my_device
```

---

## Read GPIO

```bash
cat /dev/my_device
```

or

```bash
hexdump -C /dev/my_device
```

---

## Unload Driver

```bash
sudo rmmod gpio_driver
```

---

# GPIO Number Calculation

STM32 GPIOs are grouped into banks of 16 pins.

```
GPIOA : 0 - 15
GPIOB : 16 - 31
GPIOC : 32 - 47
GPIOD : 48 - 63
GPIOE : 64 - 79
GPIOF : 80 - 95
GPIOG : 96 -111
GPIOH :112 -127
GPIOI :128 -143
```

PB10 belongs to GPIOB.

```
GPIOB Base = 16

PB10

Global GPIO Number

16 + 10 = 26
```

Therefore,

```c
#define GPIO_NUM 26
```

---

# Issue Faced During Development

## Problem

Initially the driver used

```c
#define GPIO_18 18
```

Loading the module produced:

```
pin PB2 already requested by 40010000.serial
cannot claim for GPIOB:18
Failed to request GPIO
```

The module failed to load.

---

## Root Cause

GPIO 18 corresponds to PB2.

```
GPIOB Base = 16

16 + 2 = 18
```

PB2 was already reserved by the UART driver.

```
40010000.serial
```

Since Linux allows only one driver to own a GPIO, `gpio_request()` returned an error.

---

## Solution

Verified GPIO mapping using

```bash
cat /sys/kernel/debug/gpio
```

Output

```
gpiochip1 : GPIOs 16-31 (GPIOB)
```

The required GPIO was PB10.

Global GPIO Number

```
16 + 10 = 26
```

Modified the driver

```c
#define GPIO_NUM 26
```

Recompiled

```bash
make clean
make
```

Loaded the module

```bash
sudo insmod gpio_driver.ko
```

Verification

```bash
dmesg | tail
```

Output

```
GPIO Driver Loaded Successfully
```

The GPIO could then be controlled successfully from user space.

---

# Commands Used During Debugging

Check GPIO Mapping

```bash
cat /sys/kernel/debug/gpio
```

Check Kernel Messages

```bash
dmesg | tail -20
```

Check Loaded Modules

```bash
lsmod
```

Remove Module

```bash
sudo rmmod gpio_driver
```

Insert Module

```bash
sudo insmod gpio_driver.ko
```

Verify Device

```bash
ls -l /dev/my_device
```

---

# Important Learnings

- Linux GPIO uses global GPIO numbers with the legacy GPIO API.
- GPIO numbers are calculated as:
  - Global GPIO = GPIO Bank Base + Pin Offset
- GPIOs already owned by another driver cannot be requested.
- `copy_from_user()` and `copy_to_user()` must be used for communication between user space and kernel space.
- Character drivers expose interfaces through `/dev`.
- `tee` is useful when writing to device files requiring root privileges.
- Always verify GPIO ownership using `/sys/kernel/debug/gpio`.
- Check `dmesg` first whenever a kernel module fails to load.

---

# Future Improvements

- Replace legacy GPIO API with descriptor-based `gpiod` API.
- Obtain GPIO from Device Tree instead of hardcoding the GPIO number.
- Add IOCTL support.
- Add interrupt handling.
- Convert the driver into a Platform Driver.
- Implement Device Tree compatible driver.

---

## Author

Lokesh Donode

Embedded Linux Device Driver Practice