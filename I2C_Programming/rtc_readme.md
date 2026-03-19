# Interfacing RTC (DS3231 - ZS-042) with Raspberry Pi

## 📌 Overview

This project demonstrates how to interface the **DS3231 RTC (ZS-042 module)** with a Raspberry Pi using the I2C protocol.
It includes both **low-level C programming** and **Linux I2C tools (`i2cget`, `i2cdetect`)** to read real-time clock data.

---

## 🧠 Features

* Read current date and time from RTC using C
* Access RTC registers via I2C (`/dev/i2c-1`)
* Understand BCD to decimal conversion
* Debug using `i2cdetect` and `i2cget`
* Learn difference between kernel driver vs user-space access

---

## 🔧 Hardware Requirements

* Raspberry Pi (any model with I2C support)
* DS3231 RTC module (ZS-042)
* CR2032 coin cell battery (optional but recommended)
* Jumper wires

---

## 🔌 Connections

| RTC Module | Raspberry Pi  |
| ---------- | ------------- |
| VCC        | 3.3V (Pin 1)  |
| GND        | GND (Pin 6)   |
| SDA        | GPIO2 (Pin 3) |
| SCL        | GPIO3 (Pin 5) |

⚠️ Use **3.3V only** (Raspberry Pi GPIO is not 5V tolerant)

---

## ⚙️ Setup

Go to:
---

### 1. Install I2C Tools

```bash
sudo apt update
sudo apt install -y i2c-tools
```

---

### 2. Detect RTC

```bash
i2cdetect -y 1
```

Expected:

```
68
```

## 💻 C Program

### Compile

```bash
gcc rtc_read.c -o rtc_read
```

### Run

```bash
sudo ./rtc_read
```

---

## 🧾 Example Output

```
Date: 20-03-2026
Time: 14:35:21
```

---

## 🔍 Understanding the Code

### I2C Communication Flow

1. Open `/dev/i2c-1`
2. Set slave address (`0x68`)
3. Write register address (`0x00`)
4. Read 7 bytes (time + date)

---

## 🛠️ Useful Debug Commands

### Read register using i2cget

```bash
i2cget -y 1 0x68 0x00   # Seconds
i2cget -y 1 0x68 0x01   # Minutes
i2cget -y 1 0x68 0x02   # Hours
```

---

## ⚠️ Notes

* `UU` in `i2cdetect` means device is used by kernel
* Use either:

  * Kernel driver (`hwclock`)
  * OR user-space I2C (C code)

---

## 🔋 Battery Note

* Coin cell keeps time when power is OFF
* Without battery → time resets after shutdown
* ZS-042 may try to charge battery (not safe for CR2032)

---

## 🚀 Future Improvements

* Set time using C program
* Read temperature from DS3231
* Log timestamped sensor data
* Port code to STM32 (HAL I2C)

---

## 📚 Learning Outcomes

* I2C communication in Linux
* Register-level device interaction
* BCD vs Decimal representation
* Kernel vs user-space device access

---

## 📄 License

MIT License
