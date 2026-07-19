// gpio_driver.c
// Simple GPIO Character Driver (Legacy GPIO API)
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/types.h>

#define GPIO_18    26  //Base + Offset = 16 + 10 = 26
#define DEVICE_NAME "my_device"
#define CLASS_NAME  "my_class"

static dev_t dev;
static struct cdev my_cdev;
static struct class *dev_class;

/* Function Prototypes */
static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off);

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off);

/* File Operations */
static const struct file_operations fops =
{
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

/*------------------------------------------------------------------*/

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("GPIO Driver: Device Opened\n");
    return 0;
}

/*------------------------------------------------------------------*/

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("GPIO Driver: Device Closed\n");
    return 0;
}

/*------------------------------------------------------------------*/

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    u8 gpio_state;

    if (*off)
        return 0;

    gpio_state = gpio_get_value(GPIO_18);

    if (copy_to_user(buf, &gpio_state, 1))
        return -EFAULT;

    *off += 1;

    pr_info("GPIO18 = %d\n", gpio_state);

    return 1;
}

/*------------------------------------------------------------------*/

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    char rec_buf[10];

    if (len > sizeof(rec_buf) - 1)
        len = sizeof(rec_buf) - 1;

    if (copy_from_user(rec_buf, buf, len))
        return -EFAULT;

    rec_buf[len] = '\0';

    switch (rec_buf[0])
    {
        case '1':
            gpio_set_value(GPIO_18, 1);
            pr_info("GPIO18 HIGH\n");
            break;

        case '0':
            gpio_set_value(GPIO_18, 0);
            pr_info("GPIO18 LOW\n");
            break;

        default:
            pr_err("Invalid Input. Use 0 or 1\n");
            return -EINVAL;
    }

    return len;
}

/*------------------------------------------------------------------*/

static int __init my_driver_init(void)
{
    int ret;

    /* Allocate Major Number */
    ret = alloc_chrdev_region(&dev, 0, 1, "my_dev");
    if (ret < 0)
    {
        pr_err("Cannot allocate major number\n");
        return ret;
    }

    pr_info("Major=%d Minor=%d\n", MAJOR(dev), MINOR(dev));

    /* Initialize CDEV */
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev, 1);
    if (ret)
    {
        pr_err("Cannot add cdev\n");
        goto unregister_chrdev;
    }

    /* Create Class */
    dev_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(dev_class))
    {
        pr_err("Cannot create class\n");
        ret = PTR_ERR(dev_class);
        goto del_cdev;
    }

    /* Create Device */
    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, DEVICE_NAME)))
    {
        pr_err("Cannot create device\n");
        ret = -EINVAL;
        goto destroy_class;
    }

    /* Validate GPIO */
    if (!gpio_is_valid(GPIO_18))
    {
        pr_err("GPIO %d is invalid\n", GPIO_18);
        ret = -ENODEV;
        goto destroy_device;
    }

    /* Request GPIO */
    ret = gpio_request(GPIO_18, "GPIO_18");
    if (ret)
    {
        pr_err("Failed to request GPIO\n");
        goto destroy_device;
    }

    /* Configure GPIO as Output */
    ret = gpio_direction_output(GPIO_18, 0);
    if (ret)
    {
        pr_err("Cannot set GPIO direction\n");
        goto free_gpio;
    }

    gpio_export(GPIO_18, false);

    pr_info("GPIO Driver Loaded Successfully\n");

    return 0;

free_gpio:
    gpio_free(GPIO_18);

destroy_device:
    device_destroy(dev_class, dev);

destroy_class:
    class_destroy(dev_class);

del_cdev:
    cdev_del(&my_cdev);

unregister_chrdev:
    unregister_chrdev_region(dev, 1);

    return ret;
}

/*------------------------------------------------------------------*/

static void __exit my_driver_exit(void)
{
    gpio_unexport(GPIO_18);
    gpio_set_value(GPIO_18, 0);
    gpio_free(GPIO_18);

    device_destroy(dev_class, dev);
    class_destroy(dev_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev, 1);

    pr_info("GPIO Driver Unloaded\n");
}

/*------------------------------------------------------------------*/

module_init(my_driver_init);
module_exit(my_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lokesh Donode");
MODULE_DESCRIPTION("Simple GPIO Character Driver");
MODULE_VERSION("1.0");