# I2C EEPROM Interfacing on Embedded Linux (AT24C02)

## 📌 Overview
This tutorial explains how to interface an **I2C EEPROM** with a board running **Embedded Linux**.  
We demonstrate **reading and writing EEPROM data using C code** and Linux **I2C utilities**.

- EEPROM Used: **AT24C02 (I2C EEPROM)**
- Board Used: **BeagleBone Black** (any Linux-capable board can be used)
- Language: **C**
- Interface: **I2C**

---

## 🧩 What is I2C?
I2C (Inter-Integrated Circuit) is a **serial communication protocol** developed by **Philips (now NXP)** in 1982.  
It is commonly used to connect low-speed peripherals such as **EEPROMs, sensors, ADCs, and RTCs**.


### Key Features
- **Master–Slave architecture**
- Supports **multiple slave devices**
- Uses only **two wires**

### I2C Signals
| Signal | Description |
|------|------------|
| SDA | Serial Data Line (Bi-directional) |
| SCL | Serial Clock Line (Driven by Master) |

- Typical Speed: **100 kbps**
- Maximum Speed: **3.4 Mbps**

---

## 🧭 I2C Slave Addressing
Each I2C slave device has a **unique address** (7-bit or 10-bit).  
Most devices, including EEPROMs, use **7-bit addressing**.

### AT24C02 Address Formation
From the datasheet:

- Fixed bits: `1010`
- Address pins: **A2, A1, A0**
- R/W bit handled by Linux driver


#### Address Configuration Used
- A2 = 0
- A1 = 0
- A0 = 0


Linux reports the EEPROM at address **0x50**.

---

## 💾 AT24C02 EEPROM Details
- Manufacturer: Microchip (Atmel)
- Total Size: **2 Kbits (2048 bits)**
- Organization: **256 × 8-bit words**
- Access Type: Random access

---

## 🔌 Hardware Connections
We use **I2C Bus 2** on BeagleBone Black.

| Signal | BeagleBone Black Pin |
|------|----------------------|
| SDA  | P9_20 |
| SCL  | P9_19 |

---

## 🛠️ I2C Utilities (i2c-tools)
Linux provides user-space utilities to test I2C devices.

### Installation
```bash
I2C Utilities (CLI Testing)
Before writing code, verify the hardware connection using Linux i2c-tools.

1. Installation
Bash
sudo apt-get install i2c-tools

2. Detect Device (i2cdetect)
Scan I2C bus 2 to find the EEPROM.

Bash
$i2cdetect -r 2 0x50 0x50
Output should show 50 at row 50.

3. Write Data (i2cset)
Write the value 0x80 to register address 0x00.

Bash
# Syntax: i2cset -f -y [bus] [chip-addr] [data-addr] [value]
$i2cset -f -y 2 0x50 0x00 0x80

4. Read Data (i2cget)
Read the value back from register 0x00.

Bash
# Syntax: i2cget -y [bus] [chip-addr] [data-addr]
$i2cget -y 2 0x50 0x00
Expected Output: 0x80

>> Writing Multiple Bytes

    int f;
    char buf[10];

    f = open("/dev/i2c-2", O_RDWR);
    ioctl(f, I2C_SLAVE, 0x50);

    buf[0] = 0x50;   // Start address
    buf[1] = 0x30;
    buf[2] = 0x31;
    buf[3] = 0x32;
    buf[4] = 0x33;
    buf[5] = 0x34;
    buf[6] = 0x35;
    buf[7] = 0x36;
    buf[8] = 0x37;
    buf[9] = 0x38;

    write(f, buf, 10);

>> Reading Multiple Bytes

    buf[0] = 0x50;   // Start address
    write(f, buf, 1);

    memset(buf, 0, sizeof(buf));
    read(f, buf, 9);


🚀 Advancing Further

I2C user-space driver: drivers/i2c/i2c-dev.c

SDA/SCL control logic: drivers/i2c/algos/i2c-algo-bit.c

Official Kernel Source:
https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git

