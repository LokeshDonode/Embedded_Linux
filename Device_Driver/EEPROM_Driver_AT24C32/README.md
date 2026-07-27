# AT24C32 EEPROM Driver Implementation

A Linux kernel module for interfacing with the **AT24C32 EEPROM** (ZS-042 module) via I2C, using raw 16-bit addressing with ioctl-based userspace control.

---

## Overview

### What is AT24C32?

The **AT24C32** is a 32Kbit (4096 bytes) serial EEPROM chip with the following characteristics:

| Parameter | Value |
|-----------|-------|
| **Capacity** | 4096 bytes (32 Kbits) |
| **Addressing** | 16-bit word address (2 bytes: MSB first) |
| **I2C Address Range** | 0x50–0x57 (configurable via A0/A1/A2 pins) |
| **Default Address** | 0x57 (ZS-042 module, all pins pulled high) |
| **Page Size** | 32 bytes (page write buffer) |
| **Write Cycle Time** | 10ms (tWR) |
| **Address Space** | 0x0000 to 0x0FFF |

### Why This Driver?

Standard I2C SMBus functions (`i2c_smbus_read_byte_data()`, etc.) use **8-bit command fields**, which cannot express the 16-bit addresses required by AT24C32. This driver uses raw **i2c_transfer()[...]

---

## Hardware Setup

### AT24C32 ZS-042 Module Pinout

```
GND   ----1-|o|-8---- VCC   (3.3V or 5V)
A0    ----2-|  |-7---- WP   (Write Protect: GND=enable, VCC=disable)
A1    ----3-|  |-6---- SCL  (I2C Clock)
A2    ----4-|  |-5---- SDA  (I2C Data)
```

### I2C Bus Connection

| Signal | Raspberry Pi / Beaglebone | ZS-042 Module |
|--------|---------------------------|---------------|
| **SDA** | GPIO2 (I2C1_SDA) | Pin 5 |
| **SCL** | GPIO3 (I2C1_SCL) | Pin 6 |
| **GND** | Ground | Pin 1 |
| **VCC** | 3.3V | Pin 8 |
| **WP** | - | Pin 7 (GND for normal operation) |

### Address Configuration

The I2C address is determined by A0, A1, A2 pins:

```
Address = 0b1010 [A2][A1][A0] (7-bit I2C address)

Default (ZS-042): A2=A1=A0=1 → 0b1010111 = 0x57 (decimal 87)
```

---

## Building the Driver

### Prerequisites

- Linux kernel headers: `sudo apt-get install linux-headers-$(uname -r)`
- GCC compiler: `sudo apt-get install build-essential`
- I2C tools (optional, for debugging): `sudo apt-get install i2c-tools`

### Compilation

```bash
cd Device_Driver/EEPROM_Driver_AT24C32
make all
```

**Output files:**
- `eeprom_at24c32_driver.ko` — kernel module (compiled object)
- `eeprom_at24c32_test` — userspace test application

---

## Installing and Loading the Driver

### 1. Load the Kernel Module

```bash
# Instantiate the I2C device in sysfs (if not using DT)
echo at24c32 0x57 > /sys/bus/i2c/devices/i2c-1/new_device

# Insert the kernel module
sudo insmod eeprom_at24c32_driver.ko

# Check kernel logs for success message
dmesg | tail -10
```

**Expected kernel message:**
```
at24c32: AT24C32 ready — 4096 bytes, 16-bit addr, major=<N>
```

### 2. Verify Device Creation

```bash
ls -la /dev/at24c32
```

**Expected output:**
```
crw------- 1 root root <major>,0 Jul 27 10:30 /dev/at24c32
```

### 3. Unload the Driver

```bash
sudo rmmod eeprom_at24c32_driver
echo 0x57 > /sys/bus/i2c/devices/i2c-1/delete_device
```

---

## Running Tests

### Using Makefile

```bash
# Compile and load everything
make load

# Run the test suite
make test

# Cleanup
make unload clean
```

### Manual Testing

```bash
# Build
gcc -Wall -o eeprom_at24c32_test eeprom_at24c32_test.c -I.

# Run (requires driver loaded)
sudo ./eeprom_at24c32_test
```

### Test Sequence

The test application (`eeprom_at24c32_test.c`) performs:

1. **WRITE_BYTE** — Write 0xAB to address 0x0010
2. **READ_BYTE** — Read back and verify (expect 0xAB)
3. **WRITE_PAGE** — Write 32 bytes (0x40–0x5F) to page 1 (0x0020–0x003F)
4. **READ_PAGE** — Read page back with hex dump
5. **BOUNDARY TEST** — Attempt invalid cross-page write (should be rejected)
6. **CHIP_ERASE** — Fill all 4096 bytes with 0xFF (~1.3 seconds)
7. **POST-ERASE VERIFY** — Confirm erase with read

**Expected output:**
```
Opened /dev/at24c32

=== WRITE BYTE: [0x0010] = 0xAB ===
  Done (10ms write cycle elapsed in driver).

=== READ BYTE: [0x0010] ===
  [0x0010] = 0xab  (expect 0xAB: PASS)

=== WRITE PAGE: [0x0020..0x003F] ===
  Written 32 bytes (0x40–0x5F).

=== READ PAGE: [0x0020..0x003F] ===
  [0x0020] 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f
  [0x0030] 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f

=== WRITE PAGE: boundary violation test (expect EINVAL) ===
  Correctly rejected: Invalid argument

=== CHIP ERASE (wait ~1.3s) ===
  Done. Verifying byte 0x0010...
  [0x0010] = 0xff  (expect 0xFF: PASS)
```

---

## Driver Architecture

### File Structure

```
Device_Driver/EEPROM_Driver_AT24C32/
├── README.md                          # This file
├── eeprom_at24c32_driver.c            # Kernel module (main driver)
├── eeprom_at24c32_driver.h            # Shared ioctl definitions
├── eeprom_at24c32_test.c              # Userspace test application
├── Makefile                           # Build configuration
├── eeprom_driver_test.jpg             # Hardware test photos
└── eeprom_driver_test_logs.jpg        # Test execution logs
```

### Module Components

#### 1. **at24c32_read()** — 16-bit Addressed I2C Read

```c
int at24c32_read(u16 addr, u8 *buf, u16 len)
```

- Constructs **two i2c_msg** structures:
  - **msg[0]:** Writes 2-byte word address (sets EEPROM internal pointer)
  - **msg[1]:** Reads `len` bytes with auto-increment
- Uses **REPEATED START** (Sr) instead of STOP between messages
- Returns 0 on success, negative error code on failure

**I2C Protocol:**
```
[0x57|W] [addr_hi] [addr_lo] Sr [0x57|R] [data_byte_0] ... [data_byte_n] STOP
```

#### 2. **at24c32_write()** — 16-bit Addressed I2C Write

```c
int at24c32_write(u16 addr, const u8 *data, u8 len)
```

- Constructs **one i2c_msg** with address + data
- Enforces page boundary rules (no write > 32 bytes)
- **CRITICAL:** Calls `msleep(10)` to wait for EEPROM write cycle
- Returns 0 on success, negative error code on failure

**I2C Protocol:**
```
[0x57|W] [addr_hi] [addr_lo] [data_0] [data_1] ... [data_n] STOP
```

**Page Write Rules:**
- All bytes in a single write must fall within the same 32-byte page
- If write crosses page boundary → EEPROM wraps within page → **data corruption**
- Driver enforces: `(addr >> 5) == ((addr + len - 1) >> 5)`

#### 3. **at24c32_ioctl()** — Command Handler

Supports five ioctl commands:

| Command | Type | Purpose |
|---------|------|---------|
| `AT24C32_READ_BYTE` | `_IOWR` | Single byte read |
| `AT24C32_WRITE_BYTE` | `_IOW` | Single byte write |
| `AT24C32_READ_PAGE` | `_IOWR` | Read up to 32 bytes |
| `AT24C32_WRITE_PAGE` | `_IOW` | Write up to 32 bytes (with boundary check) |
| `AT24C32_CHIP_ERASE` | `_IO` | Fill all 4096 bytes with 0xFF |

#### 4. **Character Device Interface**

- **Major number:** Dynamically allocated
- **Minor number:** 0
- **Device file:** `/dev/at24c32`
- **Operations:** `open()`, `release()`, `unlocked_ioctl()`

---

## Ioctl Command Reference

### AT24C32_READ_BYTE

**Definition:**
```c
#define AT24C32_READ_BYTE _IOWR(AT24C32_MAGIC, 0, struct at24c32_byte_rw)
```

**Usage:**
```c
struct at24c32_byte_rw brw;
brw.addr = 0x0010;  // Set address
ioctl(fd, AT24C32_READ_BYTE, &brw);
printf("Data: 0x%02x\n", brw.data);  // Read result
```

**Data Structure:**
```c
struct at24c32_byte_rw {
    __u16 addr;     // 16-bit word address
    __u8  data;     // Byte value
    __u8  _pad;     // Padding for alignment
};
```

---

### AT24C32_WRITE_BYTE

**Definition:**
```c
#define AT24C32_WRITE_BYTE _IOW(AT24C32_MAGIC, 1, struct at24c32_byte_rw)
```

**Usage:**
```c
struct at24c32_byte_rw brw;
brw.addr = 0x0010;
brw.data = 0xAB;
ioctl(fd, AT24C32_WRITE_BYTE, &brw);
// Driver waits 10ms for write cycle
```

---

### AT24C32_READ_PAGE

**Definition:**
```c
#define AT24C32_READ_PAGE _IOWR(AT24C32_MAGIC, 2, struct at24c32_page_rw)
```

**Usage:**
```c
struct at24c32_page_rw prw;
prw.addr = 0x0020;  // Page-aligned address
prw.len = 32;       // Read 32 bytes
ioctl(fd, AT24C32_READ_PAGE, &prw);
// prw.data[] contains 32 bytes
```

**Data Structure:**
```c
struct at24c32_page_rw {
    __u16 addr;                    // Start address
    __u8  len;                     // Byte count (1–32)
    __u8  _pad;                    // Padding
    __u8  data[AT24C32_PAGE_SIZE]; // 32-byte buffer
};
```

---

### AT24C32_WRITE_PAGE

**Definition:**
```c
#define AT24C32_WRITE_PAGE _IOW(AT24C32_MAGIC, 3, struct at24c32_page_rw)
```

**Usage:**
```c
struct at24c32_page_rw prw;
prw.addr = 0x0020;      // Must be page-aligned
prw.len = 32;           // Up to 32 bytes
for (int i = 0; i < 32; i++)
    prw.data[i] = 0x40 + i;
ioctl(fd, AT24C32_WRITE_PAGE, &prw);
// Driver enforces page boundary; rejects if write crosses page
```

**Boundary Enforcement:**
```
Page 0: 0x0000–0x001F (32 bytes)
Page 1: 0x0020–0x003F (32 bytes)
...
Page 127: 0x0FE0–0x0FFF (32 bytes)
```

**Example — Invalid Write:**
```c
prw.addr = 0x003C;  // Last 4 bytes of page 1
prw.len = 8;        // Would extend into page 2
ioctl(fd, AT24C32_WRITE_PAGE, &prw);  // Returns -EINVAL
```

---

### AT24C32_CHIP_ERASE

**Definition:**
```c
#define AT24C32_CHIP_ERASE _IO(AT24C32_MAGIC, 4)
```

**Usage:**
```c
ioctl(fd, AT24C32_CHIP_ERASE);  // No argument
// Fills all 4096 bytes with 0xFF (~1.3 seconds)
```

**Implementation:**
- Writes 32 bytes of 0xFF to each of 128 pages
- 128 pages × 10ms per page = ~1.28 seconds total
- Kernel logs progress to dmesg

---

## Technical Reference

### Key Concepts

#### **I2C Protocol Overview**
- **Start Condition (S):** SDA and SCL both pulled low
- **Stop Condition (P):** SDA and SCL both released (pulled high)
- **Repeated Start (Sr):** START without STOP; allows direction change mid-transaction
- **Slave Address:** 7-bit address + 1 read/write bit

#### **SMBus vs. Raw I2C**

| Feature | SMBus | Raw I2C |
|---------|-------|---------|
| **Command Field** | 8 bits | None (manual control) |
| **Max Data** | Protocol-limited | User-defined |
| **Repeated Start** | Limited | Full support |
| **Addressing** | 8-bit registers | Custom (16-bit for AT24C32) |
| **Use Case** | Simple sensors | Complex EEPROMs, arbitration |

#### **Page Write Buffer**

AT24C32 has a **32-byte internal write buffer**:
- All bytes written in one I2C transaction are latched into buffer
- After STOP, entire buffer is programmed to NVM simultaneously
- If write crosses page boundary, address pointer wraps → **data corruption**

**Example — Page Wrap Corruption:**
```
Write to 0x003E (page 1, offset 62) with 4 bytes:
  [0x003E] = 0xAA
  [0x003F] = 0xBB
  [0x0040] = 0xCC  ← Page boundary! Wraps to 0x0020
  [0x0041] = 0xDD  ← Wraps to 0x0021

Result: Page 1 is corrupted! [0x0020]=0xCC, [0x0021]=0xDD
```

#### **Write Cycle Time (tWR)**

After each EEPROM write, the chip is **busy** for up to 10ms:
- I2C bus is held in NACK state
- Cannot start new transaction until tWR expires
- Driver uses `msleep(10)` for simplicity
- Faster: **Acknowledge Polling** (probe ACK in loop, but adds complexity)

#### **Address Format**

16-bit word address transmitted as **two bytes, MSB first**:

```c
u8 addr_buf[2] = {
    (addr >> 8) & 0xFF,   // High byte (bits 15–8)
    (addr)      & 0xFF    // Low byte (bits 7–0)
};
```

Example: Address 0x0123
```
High: (0x0123 >> 8) & 0xFF = 0x01
Low:  (0x0123)      & 0xFF = 0x23
I2C wire: [0x01][0x23]
```

#### **Repeated Start Protocol**

Required for **random read** (reading from arbitrary address):

```
Standard READ_BYTE with Repeated Start:
  msg[0] (write):    [slave_addr|W] [addr_hi] [addr_lo]  ← no STOP
  msg[1] (read):     [slave_addr|R] [data_byte]
           Between:  Repeated START (Sr) — not a STOP+START
```

**Why Repeated Start?**
- Prevents bus arbitration (multi-master safety)
- Maintains slave focus on same address in one "session"

---

## Debugging and Troubleshooting

### Check Kernel Logs

```bash
dmesg | tail -20
dmesg -w        # Live kernel log
```

**Common Messages:**
```
[OK]     at24c32: AT24C32 ready — 4096 bytes, 16-bit addr, major=<N>
[ERROR]  at24c32: adapter does not support raw I2C
[ERROR]  i2c_transfer read failed at addr=0x0010: -5
[ERROR]  WRITE_PAGE crosses page boundary: 0x003c+8
```

### List I2C Devices

```bash
i2cdetect -l          # List all I2C buses
i2cdetect -y 1        # Scan bus 1 for devices (requires removal of module)
```

### Check Device File

```bash
ls -la /dev/at24c32
cat /proc/devices | grep at24c32   # Verify major number
```

### Test with i2c-tools (without kernel module)

```bash
# Read from address 0x0010 using raw i2cget
i2cget -y -f 1 0x57 0x00 0x10 w   # 'w' = word (16-bit address)
```

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `No such device` | `/dev/at24c32` not found | Load module: `insmod eeprom_at24c32_driver.ko` |
| `Permission denied` | Non-root user | Run with `sudo` |
| `ENOTTY` | Wrong magic/command number | Check ioctl header definitions |
| `EINVAL` | Address/length out of bounds | Verify address < 4096, len ≤ 32 |
| `EIO` | I2C transfer failed | Check I2C bus, WP pin, pull-up resistors |
| `adapter does not support raw I2C` | Adapter doesn't support i2c_transfer() | Use different I2C controller or adapter |

---

## Performance Notes

### Read Performance

- **Single byte:** ~1ms (I2C overhead)
- **32 bytes:** ~3-5ms (sequential read with auto-increment)
- **Random read:** ~2ms (address + read with repeated start)

### Write Performance

- **Single byte:** ~10ms (includes tWR wait)
- **32 bytes:** ~10ms (page write, tWR same regardless of bytes)
- **Chip erase:** ~1.3 seconds (128 pages × 10ms)

### Optimization Tips

1. **Batch writes** — Use 32-byte page writes instead of individual bytes
2. **Acknowledge polling** — Replace msleep(10) with loop reading slave ACK
3. **DMA transfers** — For bulk operations (not implemented in this driver)

---

## File Descriptions

### eeprom_at24c32_driver.c

Main kernel module implementing:
- I2C client probe/remove
- Character device interface (open, release, ioctl)
- 16-bit I2C read/write primitives
- Page boundary enforcement
- Chip erase logic

### eeprom_at24c32_driver.h

Shared header defining:
- Ioctl command macros
- Data structures (byte_rw, page_rw)
- Device magic number ('E')
- Memory and page size constants

### eeprom_at24c32_test.c

Comprehensive userspace test:
- Single-byte read/write
- Page read/write
- Boundary violation test
- Chip erase verification
- Hexdump utility for data display

### Makefile

Build automation:
- `make module` — Compile kernel object
- `make userapp` — Compile test application
- `make load` — Insert module into kernel
- `make unload` — Remove module
- `make test` — Run test suite
- `make clean` — Remove build artifacts

---

## References

- **AT24C32 Datasheet:** [Atmel 24C32](https://ww1.microchip.com/downloads/en/DeviceDoc/doc0336.pdf)
- **Linux I2C API:** `/usr/src/linux-headers-*/include/linux/i2c.h`
- **Ioctl Design:** Linux kernel ioctl() subsystem documentation
- **ZS-042 Module:** Common breakout board for AT24C32

---

## License

This driver is released under the **GPL v2.0** license.

---

## Author

**Lokesh Donode**

---

## Changelog

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024-07-27 | Initial release with full AT24C32 support |

---

## Appendix: Memory Map

AT24C32 16-bit address space:

```
+--------+--------+
| Page # | Address Range |  Content
+--------+-----------+
|   0    | 0x0000–0x001F |  32 bytes
|   1    | 0x0020–0x003F |  32 bytes
|   2    | 0x0040–0x005F |  32 bytes
|  ...   | ...           |  ...
|  127   | 0x0FE0–0x0FFF |  32 bytes
+--------+-----------+
Total: 4096 bytes (128 pages)
```

---

## Appendix: I2C Waveform Examples

### Random Read (address 0x0010, 1 byte)

```
      ┌─────┐     ┌───────┐     ┌─────┐
SDA   │ 0x57│ 0x00│ 0x10 │Sr│0x57│data│
      │     │     │      │  │ |  │    │
SCL  │      │     │      │  │ |  │
     └─────┘     └───────┘  └─────┘
     
     [S] [0x57|W] [0x00] [0x10] [Sr] [0x57|R] [data] [NACK] [P]
```

### Page Write (address 0x0020, 4 bytes: 0xAA, 0xBB, 0xCC, 0xDD)

```
      ┌─────┐     ┌───────────────────────────────┐
SDA   │0x57 │ 0x00│ 0x20 │0xAA│0xBB│0xCC│0xDD│
      │ |  │     │      │    │    │    │    │
SCL  │ |  │     │      │    │    │    │    │
     └─────┘     └───────────────────────────────┘
     
     [S] [0x57|W] [0x00] [0x20] [0xAA] [0xBB] [0xCC] [0xDD] [P]
     
     EEPROM: latches all 4 bytes → programs to NVM over 10ms
```

---

**End of Documentation**
