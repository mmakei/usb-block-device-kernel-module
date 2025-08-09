#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/blkdev.h>
#include <linux/namei.h>
#include "ioctl-defines.h"

static char *device = "/dev/sda1";
module_param(device, charp, 0444);

static struct block_device *bdevice;

bool kmod_ioctl_init(void);
void kmod_ioctl_teardown(void);

static bool open_usb(void)
{
    bdevice = blkdev_get_by_path(device, FMODE_READ|FMODE_WRITE, NULL);
    if (IS_ERR(bdevice)) {
        bdevice = NULL;
        return false;
    }
    return true;
}

static void close_usb(void)
{
    if (bdevice) {
        blkdev_put(bdevice, FMODE_READ|FMODE_WRITE);
        bdevice = NULL;
    }
}

struct block_device *get_usb_bdev(void)
{
    return bdevice;
}

static int __init kmod_init(void)
{
    if (!open_usb())
        return -ENODEV;
    if (!kmod_ioctl_init()) {
        close_usb();
        return -ENOMEM;
    }
    pr_info("kmod loaded\n");
    return 0;
}

static void __exit kmod_exit(void)
{
    kmod_ioctl_teardown();
    close_usb();
    pr_info("kmod unloaded\n");
}

module_init(kmod_init);
module_exit(kmod_exit);

MODULE_LICENSE("GPL");
