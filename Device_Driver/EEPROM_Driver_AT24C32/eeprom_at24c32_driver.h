/* eeprom_at24c32_driver.h — shared between kernel module and userspace
 *
 * AT24C32 key facts:
 *   Capacity   : 32Kbit = 4096 bytes
 *   I2C addr   : 0x50–0x57 (set by A0/A1/A2 solder pads on ZS-042)
 *                ZS-042 default: 0x57 (all pads open = pulled high)
 *   Word addr  : 16-bit (2 bytes: MSB first, then LSB)
 *   Page size  : 32 bytes (page write buffer in EEPROM silicon)
 *   Write cycle: 10 ms (tWR) — must wait before next write
 *
 * WHY 16-bit addressing:
 *   4096 locations need at least 12 address bits.
 *   12 bits > 8 bits → i2c_smbus_* (8-bit cmd field) CANNOT address AT24C32.
 *   Must use raw i2c_transfer() and pack address into two bytes.
 */

#ifndef EEPROM_AT24C32_DRIVER_H
#define EEPROM_AT24C32_DRIVER_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define AT24C32_MAGIC     'E'          /* 'E' for EEPROM */
#define AT24C32_MEM_SIZE  4096U        /* total bytes */
#define AT24C32_PAGE_SIZE 32U          /* hardware page write buffer */

/* ── Data structures ────────────────────────────────────────────────────────
 *
 * Single-byte access:
 *   addr  : 16-bit word address (0x0000 – 0x0FFF)
 *   data  : one byte value
 */
struct at24c32_byte_rw {
    __u16 addr;
    __u8  data;
    __u8  _pad;   /* keep struct size aligned */
};

/* Page access (up to 32 bytes):
 *   addr  : start address — for WRITE must be page-aligned (addr % 32 == 0)
 *   len   : number of bytes (1–32)
 *   data  : payload buffer
 *
 * Page-write restriction: all bytes in one write transaction must fall
 * within the SAME 32-byte page. If addr + len crosses a page boundary,
 * EEPROM wraps within that page (corrupts data). Driver enforces this.
 */
struct at24c32_page_rw {
    __u16 addr;
    __u8  len;
    __u8  _pad;
    __u8  data[AT24C32_PAGE_SIZE];
};

/* ── ioctl commands ─────────────────────────────────────────────────────────
 *
 * nr=0  READ_BYTE  : single byte read  → _IOR (data flows to user)
 * nr=1  WRITE_BYTE : single byte write → _IOW (data flows from user)
 * nr=2  READ_PAGE  : up to 32 bytes    → _IOWR (user sends addr/len, gets data)
 * nr=3  WRITE_PAGE : up to 32 bytes    → _IOW
 * nr=4  CHIP_ERASE : fill 0xFF via sequential page writes → _IO (no arg)
 */
#define AT24C32_READ_BYTE   _IOWR(AT24C32_MAGIC, 0, struct at24c32_byte_rw)
#define AT24C32_WRITE_BYTE  _IOW (AT24C32_MAGIC, 1, struct at24c32_byte_rw)
#define AT24C32_WRITE_PAGE  _IOW (AT24C32_MAGIC, 3, struct at24c32_page_rw)
#define AT24C32_CHIP_ERASE  _IO  (AT24C32_MAGIC, 4)

#define AT24C32_IOC_MAXNR   4

#endif /* AT24C32_IOCTL_H */
