/* eeprom_at24c32_driver.c — AT24C32 EEPROM driver (ZS-042 module)
 *
 * Core lesson: WHY i2c_smbus_* fails for AT24C32
 * ─────────────────────────────────────────────────────────────────────────
 * i2c_smbus_read_byte_data(client, reg):
 *   Internally sends: [ADDR|W] [reg — 8 bits] RESTART [ADDR|R] [data byte]
 *   The "command" field is always 8-bit in SMBus protocol.
 *   AT24C32 needs a 16-bit word address → SMBus physically cannot express it.
 *
 * i2c_transfer() with struct i2c_msg:
 *   Full raw I2C control. We manually construct the byte stream:
 *
 *   WRITE sequence:
 *     [ADDR|W] [addr_hi] [addr_lo] [d0] [d1] ... [dn]  STOP
 *
 *   READ sequence (combined/repeated-start):
 *     [ADDR|W] [addr_hi] [addr_lo]  Sr  [ADDR|R] [d0] [d1] ... [dn] STOP
 *     msg[0]: write, no-stop (sets internal address pointer in EEPROM)
 *     msg[1]: read  (EEPROM auto-increments address per byte clocked out)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>       /* i2c_client, i2c_driver, i2c_transfer, i2c_msg */
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>   /* copy_to_user, copy_from_user */
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>     /* msleep — required after every EEPROM write */
#include "eeprom_at24c32_driver.h"

#define DRIVER_NAME       "at24c32"
#define AT24C32_I2C_ADDR  0x57   /* ZS-042 default: A2=A1=A0=1 → 0b1010111 */

/* ── Driver private data ────────────────────────────────────────────────── */
struct at24c32_dev {
    struct i2c_client *client;
    struct cdev        cdev;
    struct class      *class;
    struct device     *device;
    dev_t              devno;
};

static struct at24c32_dev *g_dev;

/* ── 16-bit addressed I2C read ───────────────────────────────────────────────
 *
 * Builds TWO i2c_msg:
 *   msg[0]: WRITE — sends 2-byte word address (sets EEPROM internal pointer)
 *   msg[1]: READ  — clocks out 'len' bytes; EEPROM auto-increments pointer
 *
 * i2c_transfer() issues msg[0] then msg[1] with a REPEATED START between them
 * (no STOP after msg[0]) — mandatory per AT24C32 datasheet random read protocol.
 *
 * @addr : 16-bit word address (0x0000–0x0FFF)
 * @buf  : kernel buffer to store read data
 * @len  : bytes to read
 */
static int at24c32_read(u16 addr, u8 *buf, u16 len)
{
    /*
     * addr_buf holds the 16-bit address split into two bytes.
     * AT24C32 expects MSB first (big-endian word address):
     *   byte 0 = addr[15:8] (high byte, only bits[3:0] used for 4K)
     *   byte 1 = addr[7:0]  (low byte)
     */
    u8 addr_buf[2] = {
        (addr >> 8) & 0x0F,   /* high nibble of 12-bit address */
        (addr)      & 0xFF,   /* low byte */
    };

    struct i2c_msg msgs[2] = {
        {
            /* msg[0]: dummy write to set internal address pointer */
            .addr  = g_dev->client->addr,
            .flags = 0,           /* 0 = write direction */
            .len   = 2,           /* send 2 address bytes */
            .buf   = addr_buf,
        },
        {
            /* msg[1]: read len bytes; EEPROM streams from addr onward */
            .addr  = g_dev->client->addr,
            .flags = I2C_M_RD,   /* I2C_M_RD = read direction */
            .len   = len,
            .buf   = buf,
        },
    };

    /*
     * i2c_transfer(adapter, msgs, num_msgs)
     * Returns number of messages successfully transferred, or <0 on error.
     * Between msg[0] and msg[1] the I2C core issues a REPEATED START (Sr),
     * not a STOP — this is what AT24C32 random read requires.
     */
    int ret = i2c_transfer(g_dev->client->adapter, msgs, 2);
    if (ret != 2) {
        dev_err(&g_dev->client->dev,
                "i2c_transfer read failed at addr=0x%04x: %d\n", addr, ret);
        return (ret < 0) ? ret : -EIO;
    }
    return 0;
}

/* ── 16-bit addressed I2C write ──────────────────────────────────────────────
 *
 * Builds ONE i2c_msg:
 *   [ADDR|W] [addr_hi] [addr_lo] [data0] [data1] ... STOP
 *
 * All bytes in a single transaction — EEPROM latches entire write to
 * its internal page buffer and programs to NVM after STOP condition.
 *
 * CRITICAL: after this function returns, caller MUST wait AT LEAST 10ms
 *           (tWR write cycle time) before next I2C access to AT24C32.
 *           During write cycle the chip NACKs all I2C traffic.
 *
 * Page boundary rule: addr and addr+len-1 must be in same 32-byte page.
 *   Page N spans: [N*32 .. N*32+31]
 *   If write crosses boundary, EEPROM wraps within page → data corruption.
 */
static int at24c32_write(u16 addr, const u8 *data, u8 len)
{
    /*
     * Allocate kernel buffer: 2 address bytes + up to 32 data bytes.
     * Use stack allocation (max 34 bytes — safe, well under stack limit).
     */
    u8 buf[2 + AT24C32_PAGE_SIZE];

    if (len == 0 || len > AT24C32_PAGE_SIZE)
        return -EINVAL;

    /* Pack address + data into single buffer */
    buf[0] = (addr >> 8) & 0x0F;  /* addr high byte */
    buf[1] = (addr)      & 0xFF;  /* addr low byte  */
    memcpy(&buf[2], data, len);

    struct i2c_msg msg = {
        .addr  = g_dev->client->addr,
        .flags = 0,          /* write */
        .len   = 2 + len,   /* address (2) + data (len) */
        .buf   = buf,
    };

    int ret = i2c_transfer(g_dev->client->adapter, &msg, 1);
    if (ret != 1) {
        dev_err(&g_dev->client->dev,
                "i2c_transfer write failed at addr=0x%04x: %d\n", addr, ret);
        return (ret < 0) ? ret : -EIO;
    }

    /*
     * Wait for internal write cycle.
     * AT24C32 tWR = 10ms max. During this time chip is busy programming NVM.
     * Polling ACK (acknowledge polling) is faster but adds complexity.
     * msleep(10) is safe and sufficient here.
     */
    msleep(10);
    return 0;
}

/* ── ioctl handler ───────────────────────────────────────────────────────── */
static long at24c32_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct at24c32_byte_rw brw;
    struct at24c32_page_rw prw;
    u16 page_end;
    u8  erase_buf[AT24C32_PAGE_SIZE];
    u16 erase_addr;
    int ret = 0;

    /* Validate magic and command number — same pattern as DS1307 driver */
    if (_IOC_TYPE(cmd) != AT24C32_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > AT24C32_IOC_MAXNR)
        return -ENOTTY;

    switch (cmd) {

    /* ════════════════════════════════════════════════════════════════════════
     * AT24C32_READ_BYTE  (_IOWR)
     *
     * User fills: brw.addr
     * Driver fills: brw.data
     * Uses _IOWR because user sends addr (write) and gets data back (read)
     * ════════════════════════════════════════════════════════════════════════ */
    case AT24C32_READ_BYTE:
        if (copy_from_user(&brw, (void __user *)arg, sizeof(brw)))
            return -EFAULT;

        /* bounds check */
        if (brw.addr >= AT24C32_MEM_SIZE)
            return -EINVAL;

        ret = at24c32_read(brw.addr, &brw.data, 1);
        if (ret) return ret;

        /* copy back the filled struct (brw.data now has the byte) */
        if (copy_to_user((void __user *)arg, &brw, sizeof(brw)))
            return -EFAULT;

        dev_dbg(&g_dev->client->dev,
                "READ_BYTE [0x%04x] = 0x%02x\n", brw.addr, brw.data);
        break;

    /* ════════════════════════════════════════════════════════════════════════
     * AT24C32_WRITE_BYTE  (_IOW)
     *
     * User fills: brw.addr, brw.data
     * Single byte write — occupies a full 10ms write cycle
     * For bulk writes, use WRITE_PAGE (up to 32 bytes per write cycle)
     * ════════════════════════════════════════════════════════════════════════ */
    case AT24C32_WRITE_BYTE:
        if (copy_from_user(&brw, (void __user *)arg, sizeof(brw)))
            return -EFAULT;

        if (brw.addr >= AT24C32_MEM_SIZE)
            return -EINVAL;

        ret = at24c32_write(brw.addr, &brw.data, 1);
        if (ret) return ret;

        dev_dbg(&g_dev->client->dev,
                "WRITE_BYTE [0x%04x] = 0x%02x\n", brw.addr, brw.data);
        break;

    /* ════════════════════════════════════════════════════════════════════════
     * AT24C32_READ_PAGE  (_IOWR)
     *
     * User fills: prw.addr, prw.len
     * Driver fills: prw.data[]
     * Sequential read: after address is set, EEPROM clocks out bytes
     * automatically with address auto-increment — no page boundary restriction
     * for READS (only writes have page restriction)
     * ════════════════════════════════════════════════════════════════════════ */
    case AT24C32_READ_PAGE:
        if (copy_from_user(&prw, (void __user *)arg, sizeof(prw)))
            return -EFAULT;

        if (prw.len == 0 || prw.len > AT24C32_PAGE_SIZE)
            return -EINVAL;
        if ((u32)prw.addr + prw.len > AT24C32_MEM_SIZE)
            return -EINVAL;

        ret = at24c32_read(prw.addr, prw.data, prw.len);
        if (ret) return ret;

        if (copy_to_user((void __user *)arg, &prw, sizeof(prw)))
            return -EFAULT;
        break;

    /* ════════════════════════════════════════════════════════════════════════
     * AT24C32_WRITE_PAGE  (_IOW)
     *
     * User fills: prw.addr, prw.len, prw.data[]
     *
     * PAGE BOUNDARY ENFORCEMENT:
     *   AT24C32 internal write buffer = 32 bytes, aligned to 32-byte pages.
     *   If write crosses page boundary, EEPROM wraps address pointer within
     *   current page → overwrites start of page. MUST prevent this in driver.
     *
     *   Check: (addr & ~(PAGE_SIZE-1)) == ((addr+len-1) & ~(PAGE_SIZE-1))
     *   i.e., both start and end address must be in same page.
     * ════════════════════════════════════════════════════════════════════════ */
    case AT24C32_WRITE_PAGE:
        if (copy_from_user(&prw, (void __user *)arg, sizeof(prw)))
            return -EFAULT;

        if (prw.len == 0 || prw.len > AT24C32_PAGE_SIZE)
            return -EINVAL;
        if ((u32)prw.addr + prw.len > AT24C32_MEM_SIZE)
            return -EINVAL;

        /*
         * Page boundary check:
         *   page_of(addr)       = addr / 32  = addr >> 5
         *   page_of(addr+len-1) = (addr+len-1) >> 5
         * If different → write would cross page boundary → reject.
         */
        page_end = prw.addr + prw.len - 1;
        if ((prw.addr >> 5) != (page_end >> 5)) {
            dev_err(&g_dev->client->dev,
                    "WRITE_PAGE crosses page boundary: 0x%04x+%u\n",
                    prw.addr, prw.len);
            return -EINVAL;
        }

        ret = at24c32_write(prw.addr, prw.data, prw.len);
        if (ret) return ret;
        break;

    /* ════════════════════════════════════════════════════════════════════════
     * AT24C32_CHIP_ERASE  (_IO — no arg)
     *
     * Fills all 4096 bytes with 0xFF (EEPROM erased state).
     * 4096 / 32 = 128 page writes × 10ms = ~1.28 seconds total.
     * This is expected for EEPROM; warn user in dmesg.
     * ════════════════════════════════════════════════════════════════════════ */
    case AT24C32_CHIP_ERASE:
        dev_info(&g_dev->client->dev,
                 "chip erase started (~1.3 sec)...\n");
        memset(erase_buf, 0xFF, AT24C32_PAGE_SIZE);

        for (erase_addr = 0;
             erase_addr < AT24C32_MEM_SIZE;
             erase_addr += AT24C32_PAGE_SIZE)
        {
            ret = at24c32_write(erase_addr, erase_buf, AT24C32_PAGE_SIZE);
            if (ret) {
                dev_err(&g_dev->client->dev,
                        "erase failed at 0x%04x\n", erase_addr);
                return ret;
            }
        }
        dev_info(&g_dev->client->dev, "chip erase done\n");
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

/* ── File operations ────────────────────────────────────────────────────── */
static int at24c32_open(struct inode *inode, struct file *file)
{
    pr_info("at24c32: opened\n");
    return 0;
}

static int at24c32_release(struct inode *inode, struct file *file)
{
    pr_info("at24c32: released\n");
    return 0;
}

static const struct file_operations at24c32_fops = {
    .owner          = THIS_MODULE,
    .open           = at24c32_open,
    .release        = at24c32_release,
    .unlocked_ioctl = at24c32_ioctl,
};

/* ── I2C probe / remove ─────────────────────────────────────────────────── */
static int at24c32_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    int ret;

    /*
     * Check adapter supports raw I2C (not just SMBus).
     * I2C_FUNC_I2C = raw i2c_transfer() capability.
     * Without this, our 16-bit addressing won't work.
     */
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev,
                "adapter does not support raw I2C — cannot use 16-bit addressing\n");
        return -ENODEV;
    }

    g_dev = devm_kzalloc(&client->dev, sizeof(*g_dev), GFP_KERNEL);
    if (!g_dev)
        return -ENOMEM;

    g_dev->client = client;
    i2c_set_clientdata(client, g_dev);

    ret = alloc_chrdev_region(&g_dev->devno, 0, 1, DRIVER_NAME);
    if (ret) return ret;

    cdev_init(&g_dev->cdev, &at24c32_fops);
    g_dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&g_dev->cdev, g_dev->devno, 1);
    if (ret) goto err_cdev;

    g_dev->class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(g_dev->class)) { ret = PTR_ERR(g_dev->class); goto err_class; }

    g_dev->device = device_create(g_dev->class, NULL,
                                   g_dev->devno, NULL, DRIVER_NAME);
    if (IS_ERR(g_dev->device)) { ret = PTR_ERR(g_dev->device); goto err_dev; }

    dev_info(&client->dev,
             "AT24C32 ready — 4096 bytes, 16-bit addr, major=%d\n",
             MAJOR(g_dev->devno));
    return 0;

err_dev:   class_destroy(g_dev->class);
err_class: cdev_del(&g_dev->cdev);
err_cdev:  unregister_chrdev_region(g_dev->devno, 1);
    return ret;
}

static int at24c32_remove(struct i2c_client *client)
{
    struct at24c32_dev *dev = i2c_get_clientdata(client);
    device_destroy(dev->class, dev->devno);
    class_destroy(dev->class);
    cdev_del(&dev->cdev);
    unregister_chrdev_region(dev->devno, 1);
    return 0;
}

/* ── I2C match tables ───────────────────────────────────────────────────── */
static const struct i2c_device_id at24c32_id[] = {
    { "at24c32", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, at24c32_id);

static const struct of_device_id at24c32_of_match[] = {
    { .compatible = "atmel,24c32" },  /* standard kernel DT binding */
    { }
};
MODULE_DEVICE_TABLE(of, at24c32_of_match);

static struct i2c_driver at24c32_driver = {
    .driver   = { .name = DRIVER_NAME, .of_match_table = at24c32_of_match },
    .probe    = at24c32_probe,
    .remove   = at24c32_remove,
    .id_table = at24c32_id,
};

module_i2c_driver(at24c32_driver);

MODULE_AUTHOR("Lokesh Donode");
MODULE_DESCRIPTION("AT24C32 EEPROM ioctl demo — ZS-042, 16-bit addressing");
MODULE_LICENSE("GPL");
