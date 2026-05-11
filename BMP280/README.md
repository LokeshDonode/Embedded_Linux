# BMP280 Sensor Implementation Guide

## Overview

This project demonstrates the implementation and interfacing of the **BMP280 Barometric Pressure Sensor** with an **ESP32 microcontroller** over **I2C communication protocol**. The sensor readings (temperature, pressure, and altitude) are transmitted over a network using **MQTT protocol** for real-time monitoring and data acquisition.

---

## Table of Contents

- [About BMP280 Sensor](#about-bmp280-sensor)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration & Wiring](#pin-configuration--wiring)
- [I2C Communication Protocol](#i2c-communication-protocol)
- [Software Architecture](#software-architecture)
- [File Descriptions](#file-descriptions)
- [Setup Instructions](#setup-instructions)
- [Usage](#usage)
- [MQTT Topics](#mqtt-topics)
- [Troubleshooting](#troubleshooting)

---

## About BMP280 Sensor

### Sensor Specifications

| Parameter | Value |
|-----------|-------|
| **Sensor Type** | Barometric Pressure Sensor |
| **Manufacturer** | Bosch Sensortec |
| **Chip ID** | 0x58 |
| **Operating Voltage** | 1.7V - 3.6V |
| **I2C Address** | 0x76 (if SDO pin is LOW) or 0x77 (if SDO pin is HIGH) |
| **Operating Temperature** | -40°C to +85°C |
| **Pressure Range** | 300 hPa to 1100 hPa |
| **Measurement Accuracy** | ±1 hPa (pressure), ±1°C (temperature) |

### Key Features

- **Ultra-Low Power Consumption**: Ideal for battery-operated applications
- **High Accuracy**: Precise pressure and temperature measurements
- **Altitude Calculation**: Derived from pressure readings relative to sea level
- **I2C Interface**: Simple 2-wire communication protocol
- **Multiple Output Formats**: Raw ADC values, calibrated measurements

### Measured Parameters

1. **Temperature**: Ambient temperature in °C
2. **Pressure**: Atmospheric pressure in hPa (hectopascals)
3. **Altitude**: Height above sea level in meters (calculated)

---

## Hardware Requirements

### Components

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32 NANO / ESP32 Microcontroller | 1 | WiFi + Bluetooth enabled |
| BMP280 Sensor Module | 1 | I2C enabled |
| Breadboard | 1 | For connections |
| Jumper Wires (Male-Female) | 5-6 | For connections |
| USB Cable | 1 | For programming and power |
| Pull-up Resistors (4.7kΩ) | 2 | Optional (many modules have built-in) |

### Tools & Software

- Arduino IDE with ESP32 board support
- Adafruit BMP280 Library
- ArduinoMqttClient Library
- Python 3.x with paho-mqtt for subscriber
- MQTT Broker (Mosquitto or similar)

---

## Pin Configuration & Wiring

### ESP32 to BMP280 I2C Connection

```
ESP32 Pin Configuration (I2C):
┌─────────────────────────────────────────┐
│          ESP32 NANO / ESP32             │
├─────────────────────────────────────────┤
│ GPIO 21 (SDA) ─────────────────────────→ SDA (BMP280)
│ GPIO 22 (SCL) ─────────────────────────→ SCL (BMP280)
│ GND (0V)      ─────────────────────────→ GND (BMP280)
│ 3V3           ─────────────────────────→ VCC (BMP280)
└─────────────────────────────────────────┘

Additional Notes:
- Default I2C pins for ESP32: SDA=GPIO21, SCL=GPIO22
- Pull-up resistors (4.7kΩ) on SDA and SCL (usually built-in on modules)
- SDO pin on BMP280: Connect to GND for I2C Address 0x76 or VCC for 0x77
```

### Wiring Diagram

```
BMP280 Module:
     [VCC]  ─── 3V3 (ESP32)
     [GND]  ─── GND (ESP32)
     [SDA]  ─── GPIO 21 (ESP32)
     [SCL]  ─── GPIO 22 (ESP32)
     [SDO]  ─── GND (for I2C address 0x77)
```

---

## I2C Communication Protocol

### Protocol Overview

**I2C (Inter-Integrated Circuit)** is a synchronous serial protocol for short-distance communication:

- **Bus Type**: Two-wire (SDA + SCL)
- **Speed**: 100 kHz (Standard Mode), 400 kHz (Fast Mode)
- **Voltage**: 3.3V for ESP32
- **Master Device**: ESP32 (controls communication)
- **Slave Device**: BMP280 (responds to master)

### Communication Flow

```
1. Master (ESP32) sends START condition
2. Master sends BMP280 I2C Address (0x77) + READ/WRITE bit
3. Slave (BMP280) acknowledges with ACK bit
4. Data transfer:
   - For Reading: Master reads sensor register data
   - For Writing: Master writes configuration values
5. Master sends STOP condition
```

### Register Communication

```c
// Reading Chip ID (0xD0)
WRITE: 0x77 + 0xD0 (register address)
READ:  0x58 (chip ID confirmation)

// Reading Temperature Data
WRITE: 0x77 + 0xFA (temp MSB register)
READ:  [Temperature data bytes]

// Reading Pressure Data
WRITE: 0x77 + 0xF7 (pressure MSB register)
READ:  [Pressure data bytes]
```

### Key Registers

| Register | Address | Function |
|----------|---------|----------|
| Chip ID | 0xD0 | Identifies sensor (should read 0x58) |
| Calibration Data | 0x88-0xA1 | Factory calibration coefficients |
| Control Measure | 0xF4 | Sampling and mode configuration |
| Config | 0xF5 | Filter and standby settings |
| Temperature Data | 0xFA-0xFC | Raw temperature readings |
| Pressure Data | 0xF7-0xF9 | Raw pressure readings |

---

## Software Architecture

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      ESP32 NANO / ESP32                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐    │
│  │   WiFi Core  │───│  MQTT Client │───│  I2C Driver  │    │
│  └──────────────┘   └──────────────┘   └──────────────┘    │
│         │                   │                    │           │
│         └───────────────────┼────────────────────┘           │
│                             │                                │
│                      ┌──────▼───────┐                        │
│                      │  Main Code   │                        │
│                      │ (Arduino Sk.) │                        │
│                      └──────┬───────┘                        │
│                             │                                │
└─────────────────────────────┼────────────────────────────────┘
                              │
                ┌─────────────┴─────────────┐
                │                           │
        ┌───────▼────────┐         ┌────────▼──────┐
        │  BMP280 Sensor │         │  MQTT Broker  │
        │  (I2C Protocol)│         │  (WiFi)       │
        └────────────────┘         └───────────────┘
                │
        ┌───────▼────────────┐
        │ Sensor Readings    │
        │ - Temperature (°C) │
        │ - Pressure (hPa)   │
        │ - Altitude (m)     │
        └────────────────────┘
```

### Code Flow

```
START
  │
  ├─► Initialize Serial (115200 baud)
  │
  ├─► Connect to WiFi Network
  │    │
  │    ├─► Set WiFi mode (Station)
  │    ├─► Connect with SSID & Password
  │    └─► Wait for connection
  │
  ├─► Connect to MQTT Broker
  │    │
  │    ├─► Set broker address & port
  │    └─► Establish MQTT connection
  │
  ├─► Initialize BMP280 Sensor
  │    │
  │    ├─► Check I2C Address (0x77)
  │    ├─► Verify Chip ID (0x58)
  │    ├─► Configure sampling modes
  │    └─► Set operating parameters
  │
  ├─► Main Loop (Every 5000 ms)
  │    │
  │    ├─► Read Temperature Register
  │    ├─► Read Pressure Register
  │    ├─► Calculate Altitude
  │    │
  │    ├─► Print to Serial
  │    │
  │    ├─► Publish to MQTT:
  │    │    ├─► sensor/bmp280/temperature
  │    │    ├─► sensor/bmp280/pressure
  │    │    └─► sensor/bmp280/altitude
  │    │
  │    └─► Wait 5 seconds
  │
  └─► GOTO Main Loop

```

---

## File Descriptions

### 1. **main.cpp** - Arduino Sketch for ESP32

**Purpose**: Main implementation for ESP32 that interfaces BMP280 sensor with MQTT

**Key Components**:

- **BMP280 Initialization** (Lines 80-105):
  - Initializes sensor at I2C address 0x77
  - Configures sampling modes for accuracy
  - Sets filter and standby parameters

- **WiFi Connection** (Lines 40-60):
  - Connects to WiFi network
  - Displays local IP address
  - Waits for successful connection

- **MQTT Connection** (Lines 62-78):
  - Connects to MQTT broker at specified IP:port
  - Implements reconnection logic
  - Handles connection failures

- **Sensor Reading & Publishing** (Lines 136-182):
  - Reads temperature, pressure, altitude every 5 seconds
  - Publishes values to respective MQTT topics
  - Logs data to serial monitor

**Dependencies**:
- `Adafruit_BMP280.h` - Sensor library
- `ArduinoMqttClient.h` - MQTT protocol library

**Configuration**:
```cpp
ssid = "Your WiFi Name"              // WiFi SSID
password = "Your WiFi Password"      // WiFi Password
broker = "192.168.1.4"               // MQTT Broker IP
port = 1883                          // MQTT Port
SEALEVELPRESSURE_HPA = 1013.25       // Sea level pressure for altitude calculation
```

---

### 2. **bmp280_test.c** - Linux I2C Communication Test

**Purpose**: Low-level C program for testing BMP280 I2C communication on Linux

**Functionality**:
- Opens I2C bus (`/dev/i2c-1`)
- Sets I2C slave address (0x77)
- Reads Chip ID register (0xD0)
- Verifies sensor presence (should read 0x58)

**Output**:
```
BME280 Chip ID: 0x58
```

**Usage**:
```bash
gcc -o bmp280_test bmp280_test.c
sudo ./bmp280_test
```

**Key System Calls**:
- `open()` - Open I2C device file
- `ioctl()` - Configure I2C slave address
- `write()` - Send register address
- `read()` - Read register data
- `close()` - Close device file

---

### 3. **mqtt_sub.py** - Python MQTT Subscriber

**Purpose**: Python program to subscribe and display MQTT messages from BMP280

**Features**:
- Subscribes to all BMP280 topics
- Displays received data with timestamps
- Formats output for easy reading

**Topics Subscribed**:
```
sensor/bmp280/temperature
sensor/bmp280/pressure
sensor/bmp280/altitude
```

**Dependencies**:
```bash
pip install paho-mqtt
```

**Configuration**:
```python
BROKER_IP = "192.168.1.4"     # MQTT Broker IP address
BROKER_PORT = 1883             # MQTT Broker port
```

**Output Example**:
```
===================================
Timestamp : 2026-05-11 14:30:45
Topic     : sensor/bmp280/temperature
Data      : 25.47
Temperature = 25.47 °C
===================================
```

---

### 4. **Wiring Diagrams** (Image Files)

- **BMP280_Arduino_I2C.jpg** - Detailed wiring schematic
- **BMP280_I2C.png** - Quick reference pinout diagram

---

## Setup Instructions

### Step 1: Hardware Setup

1. **Connect BMP280 to ESP32**:
   ```
   BMP280 VCC  → ESP32 3V3
   BMP280 GND  → ESP32 GND
   BMP280 SDA  → ESP32 GPIO 21
   BMP280 SCL  → ESP32 GPIO 22
   BMP280 SDO  → ESP32 GND (for address 0x77)
   ```

2. **Connect ESP32 to Computer** via USB

### Step 2: Arduino IDE Configuration

1. **Install ESP32 Board Support**:
   - Open Arduino IDE → Preferences
   - Add URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Install ESP32 boards from Board Manager

2. **Install Required Libraries**:
   ```
   Sketch → Include Library → Manage Libraries
   
   Search for:
   - Adafruit BMP280 Library
   - Arduino MQTT Client
   ```

3. **Select Board**:
   ```
   Tools → Board → ESP32-PICO-KIT (or your ESP32 variant)
   Tools → Port → COM# (or /dev/ttyUSB0 on Linux)
   ```

### Step 3: Configure Code

1. **Edit WiFi Credentials** in `main.cpp`:
   ```cpp
   const char* ssid = "Your WiFi SSID";
   const char* password = "Your WiFi Password";
   ```

2. **Set MQTT Broker IP**:
   ```cpp
   const char broker[] = "192.168.1.X";  // Your broker IP
   ```

### Step 4: Upload Code

1. Click Upload button (→) in Arduino IDE
2. Wait for compilation and upload
3. Monitor output in Serial Monitor (Tools → Serial Monitor, 115200 baud)

### Step 5: Setup MQTT Broker

**On Linux**:
```bash
sudo apt-get install mosquitto mosquitto-clients

# Start Mosquitto
sudo systemctl start mosquitto
sudo systemctl enable mosquitto

# Check status
sudo systemctl status mosquitto
```

### Step 6: Run Python Subscriber

```bash
python3 mqtt_sub.py
```

---

## Usage

### Expected Serial Output (Arduino IDE)

```
ESP32 BMP280 MQTT Example
=================================
Connecting to WiFi : Your WiFi Name
.....
WiFi Connected Successfully
IP Address : 192.168.1.100
=================================
Connecting to MQTT Broker : 192.168.1.4
MQTT Connected Successfully!
=================================
Initializing BMP280 Sensor...
BMP280 Initialization Successful
=================================
=========== BMP280 DATA ===========
Temperature : 25.47 °C
Pressure    : 1013.25 hPa
Altitude    : 0.50 meters
===================================
Published -> sensor/bmp280/temperature : 25.47
Published -> sensor/bmp280/pressure : 1013.25
Published -> sensor/bmp280/altitude : 0.50
```

### Python Subscriber Output

```
===================================
Connected to MQTT Broker
===================================
Subscribed Topics:
  -> sensor/bmp280/temperature
  -> sensor/bmp280/pressure
  -> sensor/bmp280/altitude
===================================

===================================
Timestamp : 2026-05-11 14:30:45
Topic     : sensor/bmp280/temperature
Data      : 25.47
Temperature = 25.47 °C
===================================

===================================
Timestamp : 2026-05-11 14:30:45
Topic     : sensor/bmp280/pressure
Data      : 1013.25
Pressure    = 1013.25 hPa
===================================

===================================
Timestamp : 2026-05-11 14:30:45
Topic     : sensor/bmp280/altitude
Data      : 0.50
Altitude    = 0.50 meters
===================================
```

---

## MQTT Topics

### Topic Structure

```
Base Topic: sensor/bmp280/

├── sensor/bmp280/temperature  [QoS: 0]
│   └── Payload: Temperature in °C (float)
│       Example: 25.47
│
├── sensor/bmp280/pressure     [QoS: 0]
│   └── Payload: Pressure in hPa (float)
│       Example: 1013.25
│
└── sensor/bmp280/altitude     [QoS: 0]
    └── Payload: Altitude in meters (float)
        Example: 0.50
```

### Publishing Interval

- **Interval**: 5000 milliseconds (5 seconds)
- **Frequency**: 12 readings per minute
- **QoS Level**: 0 (At Most Once)

### Subscribing to Topics

**Using mosquitto_sub**:
```bash
# Subscribe to all BMP280 topics
mosquitto_sub -h 192.168.1.4 -t "sensor/bmp280/#"

# Subscribe to specific topic
mosquitto_sub -h 192.168.1.4 -t "sensor/bmp280/temperature"
```

---

## Troubleshooting

### Issue 1: "BMP280 Sensor Not Found!"

**Symptoms**: Serial output shows initialization error

**Causes & Solutions**:
1. **Incorrect I2C Address**:
   - Check SDO pin connection
   - If SDO=GND → use 0x76
   - If SDO=VCC → use 0x77
   ```cpp
   if (!bmp.begin(0x76))  // Try 0x76 instead of 0x77
   ```

2. **Loose Wiring**:
   - Verify all connections are secure
   - Check for bent pins
   - Test with multimeter for continuity

3. **Defective Sensor**:
   - Test with Linux I2C tool:
   ```bash
   i2cdetect -y 1
   # Should show device at address 0x77
   ```

### Issue 2: WiFi Connection Fails

**Symptoms**: ESP32 stuck at WiFi connection

**Solutions**:
1. Verify SSID and password are correct
2. Check WiFi signal strength
3. Ensure WiFi is not using WPA3 (use WPA2)
4. Try manually connecting to network in code:
   ```cpp
   WiFi.mode(WIFI_STA);
   WiFi.disconnect(true);  // Turn off RF
   delay(100);
   WiFi.begin(ssid, password);
   ```

### Issue 3: MQTT Connection Fails

**Symptoms**: Cannot connect to MQTT broker

**Solutions**:
1. Verify broker IP address is correct:
   ```bash
   ping 192.168.1.4
   ```

2. Check if Mosquitto is running:
   ```bash
   sudo systemctl status mosquitto
   ```

3. Verify network connectivity from ESP32:
   - Check Serial output for IP address
   - Ping ESP32 from your computer

4. Check firewall settings:
   ```bash
   sudo ufw allow 1883
   ```

5. Test with mosquitto_pub:
   ```bash
   mosquitto_pub -h 192.168.1.4 -t "test" -m "hello"
   ```

### Issue 4: Inaccurate Readings

**Symptoms**: Temperature/Pressure values seem wrong

**Solutions**:
1. **Calibration**:
   - Compare with known reference
   - BMP280 has internal calibration data (auto-applied)

2. **Environmental Factors**:
   - Sensor needs time to stabilize (≈10 minutes after power-on)
   - Keep sensor away from heat sources
   - Ensure proper ventilation

3. **Sampling Configuration**:
   - Increase sampling rate in setup:
   ```cpp
   bmp.setSampling(
     Adafruit_BMP280::MODE_NORMAL,
     Adafruit_BMP280::SAMPLING_X4,   // Increase from X2
     Adafruit_BMP280::SAMPLING_X16,
     Adafruit_BMP280::FILTER_X16,
     Adafruit_BMP280::STANDBY_MS_500
   );
   ```

### Issue 5: Python Subscriber Not Receiving Data

**Symptoms**: Python script connects but receives no messages

**Solutions**:
1. Verify MQTT topics match:
   ```python
   # Ensure topics in python script match Arduino code
   TOPICS = [
       ("sensor/bmp280/temperature", 0),
       ("sensor/bmp280/pressure", 0),
       ("sensor/bmp280/altitude", 0)
   ]
   ```

2. Check broker connectivity:
   ```bash
   mosquitto_sub -v -h 192.168.1.4 -t "sensor/bmp280/#"
   ```

3. Enable debug logging:
   ```python
   client.enable_logger()  # Add this before connect()
   ```

---

## Performance Metrics

| Metric | Value | Notes |
|--------|-------|-------|
| I2C Communication Speed | 400 kHz | Fast Mode |
| Sensor Reading Interval | 5000 ms | Configurable |
| Temperature Accuracy | ±1°C | Typical |
| Pressure Accuracy | ±1 hPa | Typical |
| Power Consumption (Active) | ~500 µA | Approximate |
| MQTT Publish Frequency | 0.2 Hz | 1 reading per 5 seconds |

---

## References

- [Bosch BMP280 Datasheet](https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/bmp280/)
- [Adafruit BMP280 Library Docs](https://github.com/adafruit/Adafruit_BMP280_Library)
- [I2C Protocol Specification](https://en.wikipedia.org/wiki/I%C2%B2C)
- [MQTT Protocol Documentation](https://mqtt.org/)
- [ESP32 Official Documentation](https://docs.espressif.com/)

---

## License

This project is part of the Embedded_Linux repository. Please refer to the repository's main LICENSE file.

---

## Author Notes

- Ensure all connections are verified before powering on
- Start with Serial monitor to debug before using MQTT
- Use Mosquitto tools (`mosquitto_sub`, `mosquitto_pub`) to test broker independently
- Keep a backup of your WiFi and MQTT credentials securely

For questions or issues, refer to the troubleshooting section or consult the sensor datasheet.

---

**Last Updated**: May 11, 2026  
**Project**: BMP280 Sensor Interfacing with ESP32  
**Repository**: LokeshDonode/Embedded_Linux
