# RTC Reader (Device_Driver/rtc_read.c)

Overview
--------
This program reads time (and temperature if supported) from an I2C Real-Time Clock (RTC) such as the DS3231 or DS1307 at I2C address 0x68. It converts the RTC time (typically UTC) to IST (UTC+5:30) by default and prints a single-line live status (weekday, date, time, temperature).

Features
--------
- Reads time registers (seconds, minutes, hours, day, date, month, year).
- Handles both 12-hour and 24-hour RTC hour encodings.
- Converts BCD-encoded RTC registers to integers.
- Converts UTC to IST with proper handling of minute/hour/day/month/year rollovers.
- Reads temperature registers for DS3231 (registers 0x11 and 0x12) and prints temperature with fractional 0.25°C resolution (when supported).

Hardware & OS Requirements
--------------------------
- Linux system with I2C device support (e.g., Raspberry Pi).
- RTC connected to I2C bus (default code uses `/dev/i2c-1` and address `0x68`).
- I2C kernel module and i2c-dev enabled.

Dependencies
------------
- C compiler (gcc/clang)
- Linux headers for I2C: `<linux/i2c-dev.h>` (i2c-dev kernel module)
- Proper user permissions to access `/dev/i2c-1` (run as root or add user to the `i2c` group)

Build
-----
Compile with:

```
gcc -o rtc_read rtc_read.c
```

Run
---
Example:

```
sudo ./rtc_read
```

- Running as root or via sudo is typically required to access `/dev/i2c-1`.
- The program continuously prints the current time and temperature and updates every second.

Behavior & Notes
----------------
- The program expects the RTC at address `0x68` on bus `/dev/i2c-1`. Modify `I2C_BUS` and `RTC_ADDR` in the source if your setup differs.
- Timezone conversion: the program adds +5:30 to the RTC value (assumes RTC is in UTC). If your RTC already stores local time, disable or remove the UTC->IST conversion.
- 12-hour vs 24-hour mode: the hour register is decoded for both formats. The code checks the mode bit and handles AM/PM conversion to 24-hour representation.
- Temperature: DS3231 reports integer MSB and fractional bits in the next register (0.25°C steps). DS1307 does not have temperature registers — temperature read will fail for that device.
- Be mindful of daylight saving adjustments and leap-second handling — this program does naive timezone arithmetic; for production usage, synchronize/maintain RTC using NTP or more robust timezone-aware logic.

Troubleshooting
---------------
- Permission denied: run with sudo or add your user to the `i2c` group.
- No device found / EIO errors: check wiring (SDA, SCL), address, and use `i2cdetect -y 1` to verify the device is present.
- Wrong time: verify RTC battery installed and that RTC is running. Consider setting RTC from system time once and then using RTC afterward.

Extending
---------
- Add a command-line flag to select timezone or disable timezone conversion.
- Add an option to set RTC time from system time (write registers), optionally only when a flag is passed to avoid repeated writes.
- Add graceful termination handling (capture SIGINT and close file descriptor cleanly).
- Add logging to file or syslog.

License
-------
Add a license to this repository (for example, MIT) to clarify reuse and distribution.
