/* at24c32_test.c — userspace ioctl test for AT24C32 EEPROM driver
 *
 * Build: gcc -Wall -o at24c32_test at24c32_test.c -I.
 * Run:   sudo ./at24c32_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>

#define __KERNEL__
#include "at24c32_ioctl.h"
#undef __KERNEL__

#define DEV_PATH "/dev/at24c32"

static void hexdump(const uint8_t *buf, uint16_t len, uint16_t base_addr)
{
    for (uint16_t i = 0; i < len; i++) {
        if (i % 16 == 0)
            printf("  [0x%04x] ", base_addr + i);
        printf("%02x ", buf[i]);
        if (i % 16 == 15 || i == len - 1)
            printf("\n");
    }
}

int main(void)
{
    int fd;
    struct at24c32_byte_rw brw;
    struct at24c32_page_rw prw;

    fd = open(DEV_PATH, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open %s: %s\n", DEV_PATH, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("Opened %s\n\n", DEV_PATH);

    /* ══════════════════════════════════════════════════════════════════════
     * 1. WRITE BYTE at address 0x0010
     *
     * ioctl(fd, AT24C32_WRITE_BYTE, &brw)
     *   → kernel: copy_from_user → at24c32_write(0x0010, 0xAB, 1)
     *   → I2C wire: [0x57|W][0x00][0x10][0xAB] STOP
     *   → msleep(10) — wait for write cycle to complete
     * ══════════════════════════════════════════════════════════════════════ */
    printf("=== WRITE BYTE: [0x0010] = 0xAB ===\n");
    brw.addr = 0x0010;
    brw.data = 0xAB;
    if (ioctl(fd, AT24C32_WRITE_BYTE, &brw) < 0) {
        perror("AT24C32_WRITE_BYTE"); goto err;
    }
    printf("  Done (10ms write cycle elapsed in driver).\n\n");

    /* ══════════════════════════════════════════════════════════════════════
     * 2. READ BYTE at address 0x0010
     *
     * ioctl(fd, AT24C32_READ_BYTE, &brw)
     *   → user fills brw.addr=0x0010
     *   → kernel: copy_from_user → at24c32_read(0x0010, &data, 1)
     *   → I2C wire: [0x57|W][0x00][0x10] Sr [0x57|R][0xAB] STOP
     *   → kernel: copy_to_user (brw.data = 0xAB)
     * ══════════════════════════════════════════════════════════════════════ */
    printf("=== READ BYTE: [0x0010] ===\n");
    brw.addr = 0x0010;
    brw.data = 0x00;
    if (ioctl(fd, AT24C32_READ_BYTE, &brw) < 0) {
        perror("AT24C32_READ_BYTE"); goto err;
    }
    printf("  [0x%04x] = 0x%02x  (expect 0xAB: %s)\n\n",
           brw.addr, brw.data, brw.data == 0xAB ? "PASS" : "FAIL");

    /* ══════════════════════════════════════════════════════════════════════
     * 3. WRITE PAGE — write 32 bytes to page 1 (addr 0x0020–0x003F)
     *
     * Page 1 = addresses 32..63 (0x0020..0x003F).
     * Must NOT cross page boundary → addr % 32 == 0 and len <= 32.
     * Driver enforces this; will return -EINVAL if violated.
     * ══════════════════════════════════════════════════════════════════════ */
    printf("=== WRITE PAGE: [0x0020..0x003F] ===\n");
    prw.addr = 0x0020;   /* page-aligned start */
    prw.len  = 32;
    for (int i = 0; i < 32; i++)
        prw.data[i] = (uint8_t)(0x40 + i);   /* 0x40, 0x41, ..., 0x5F */

    if (ioctl(fd, AT24C32_WRITE_PAGE, &prw) < 0) {
        perror("AT24C32_WRITE_PAGE"); goto err;
    }
    printf("  Written 32 bytes (0x40–0x5F).\n\n");

    /* ══════════════════════════════════════════════════════════════════════
     * 4. READ PAGE — read 32 bytes from 0x0020
     *
     * Sequential read: no page boundary restriction.
     * AT24C32 auto-increments internal address pointer per byte.
     * ══════════════════════════════════════════════════════════════════════ */
    printf("=== READ PAGE: [0x0020..0x003F] ===\n");
    prw.addr = 0x0020;
    prw.len  = 32;
    memset(prw.data, 0, sizeof(prw.data));

    if (ioctl(fd, AT24C32_READ_PAGE, &prw) < 0) {
        perror("AT24C32_READ_PAGE"); goto err;
    }
    hexdump(prw.data, prw.len, prw.addr);
    printf("\n");

    /* ══════════════════════════════════════════════════════════════════════
     * 5. Cross-page boundary test — EXPECT FAILURE (-EINVAL from driver)
     *
     * addr=0x003C, len=8 → spans 0x003C..0x0043 → crosses page 1/2 boundary
     * Driver should reject with EINVAL before even touching I2C bus.
     * ══════════════════════════════════════════════════════════════════════ */
    printf("=== WRITE PAGE: boundary violation test (expect EINVAL) ===\n");
    prw.addr = 0x003C;   /* last 4 bytes of page 1 */
    prw.len  = 8;        /* would cross into page 2 */
    memset(prw.data, 0xDE, 8);
    if (ioctl(fd, AT24C32_WRITE_PAGE, &prw) < 0) {
        printf("  Correctly rejected: %s\n\n", strerror(errno));
    } else {
        printf("  ERROR: should have been rejected!\n\n");
    }

    /* ══════════════════════════════════════════════════════════════════════
     * 6. CHIP ERASE — fills all 4096 bytes with 0xFF (~1.3 seconds)
     * ══════════════════════════════════════════════════════════════════════ */
    printf("=== CHIP ERASE (wait ~1.3s) ===\n");
    if (ioctl(fd, AT24C32_CHIP_ERASE) < 0) {
        perror("AT24C32_CHIP_ERASE"); goto err;
    }
    printf("  Done. Verifying byte 0x0010...\n");

    brw.addr = 0x0010;
    ioctl(fd, AT24C32_READ_BYTE, &brw);
    printf("  [0x0010] = 0x%02x  (expect 0xFF: %s)\n\n",
           brw.data, brw.data == 0xFF ? "PASS" : "FAIL");

    close(fd);
    return EXIT_SUCCESS;

err:
    close(fd);
    return EXIT_FAILURE;
}
