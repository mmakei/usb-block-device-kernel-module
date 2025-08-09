#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/dcache.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/limits.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/cdev.h>
#include <linux/nospec.h>
#include <linux/vmalloc.h>
#include "../ioctl-defines.h"

static dev_t dev = 0;
static struct class* kmod_class;
static struct cdev kmod_cdev;

struct block_rw_ops rw_request;
struct block_rwoffset_ops rwoffset_request;

bool kmod_ioctl_init(void);
void kmod_ioctl_teardown(void);

static long kmod_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    char *kernbuf;
    struct file *usb_file;
    struct block_device *bdev;
    struct bio *bio;

    switch (cmd)
    {
        case BREAD:
            if (copy_from_user(&rw_request, (void __user *)arg, sizeof(rw_request))) return -EFAULT;
            kernbuf = vmalloc(rw_request.size);
            if (!kernbuf) return -ENOMEM;
            usb_file = bdev_file_open_by_path("/dev/sda1", BLK_OPEN_READ, NULL, NULL);
            if (!usb_file) {
                vfree(kernbuf);
                return -EIO;
            }
            bdev = file_bdev(usb_file);
            bio = bio_alloc(bdev, 1, REQ_OP_READ, GFP_NOIO);
            bio_set_dev(bio, bdev);
            bio->bi_iter.bi_sector = 0;
            bio->bi_opf = REQ_OP_READ;
            bio_add_page(bio, vmalloc_to_page(kernbuf), rw_request.size, offset_in_page(kernbuf));
            submit_bio_wait(bio);
            copy_to_user(rw_request.data, kernbuf, rw_request.size);
            bio_put(bio);
            fput(usb_file);
            vfree(kernbuf);
            return 0;

        case BWRITE:
            if (copy_from_user(&rw_request, (void __user *)arg, sizeof(rw_request))) return -EFAULT;
            kernbuf = vmalloc(rw_request.size);
            if (!kernbuf) return -ENOMEM;
            if (copy_from_user(kernbuf, rw_request.data, rw_request.size)) {
                vfree(kernbuf);
                return -EFAULT;
            }
            usb_file = bdev_file_open_by_path("/dev/sda1", BLK_OPEN_WRITE, NULL, NULL);
            if (!usb_file) {
                vfree(kernbuf);
                return -EIO;
            }
            bdev = file_bdev(usb_file);
            bio = bio_alloc(bdev, 1, REQ_OP_WRITE, GFP_NOIO);
            bio_set_dev(bio, bdev);
            bio->bi_iter.bi_sector = 0;
            bio->bi_opf = REQ_OP_WRITE;
            bio_add_page(bio, vmalloc_to_page(kernbuf), rw_request.size, offset_in_page(kernbuf));
            submit_bio_wait(bio);
            bio_put(bio);
            fput(usb_file);
            vfree(kernbuf);
            return 0;

        case BREADOFFSET:
            if (copy_from_user(&rwoffset_request, (void __user *)arg, sizeof(rwoffset_request))) return -EFAULT;
            kernbuf = vmalloc(rwoffset_request.size);
            if (!kernbuf) return -ENOMEM;
            usb_file = bdev_file_open_by_path("/dev/sda1", BLK_OPEN_READ, NULL, NULL);
            if (!usb_file) {
                vfree(kernbuf);
                return -EIO;
            }
            bdev = file_bdev(usb_file);
            bio = bio_alloc(bdev, 1, REQ_OP_READ, GFP_NOIO);
            bio_set_dev(bio, bdev);
            bio->bi_iter.bi_sector = rwoffset_request.offset / 512;
            bio->bi_opf = REQ_OP_READ;
            bio_add_page(bio, vmalloc_to_page(kernbuf), rwoffset_request.size, offset_in_page(kernbuf));
            submit_bio_wait(bio);
            copy_to_user(rwoffset_request.data, kernbuf, rwoffset_request.size);
            bio_put(bio);
            fput(usb_file);
            vfree(kernbuf);
            return 0;

        case BWRITEOFFSET:
            if (copy_from_user(&rwoffset_request, (void __user *)arg, sizeof(rwoffset_request))) return -EFAULT;
            kernbuf = vmalloc(rwoffset_request.size);
            if (!kernbuf) return -ENOMEM;
            if (copy_from_user(kernbuf, rwoffset_request.data, rwoffset_request.size)) {
                vfree(kernbuf);
                return -EFAULT;
            }
            usb_file = bdev_file_open_by_path("/dev/sda1", BLK_OPEN_WRITE, NULL, NULL);
            if (!usb_file) {
                vfree(kernbuf);
                return -EIO;
            }
            bdev = file_bdev(usb_file);
            bio = bio_alloc(bdev, 1, REQ_OP_WRITE, GFP_NOIO);
            bio_set_dev(bio, bdev);
            bio->bi_iter.bi_sector = rwoffset_request.offset / 512;
            bio->bi_opf = REQ_OP_WRITE;
            bio_add_page(bio, vmalloc_to_page(kernbuf), rwoffset_request.size, offset_in_page(kernbuf));
            submit_bio_wait(bio);
            bio_put(bio);
            fput(usb_file);
            vfree(kernbuf);
            return 0;

        default:
            return -1;
    }

    return 0;
}

static int kmod_open(struct inode* inode, struct file* file) {
    return 0;
}

static int kmod_release(struct inode* inode, struct file* file) {
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = kmod_open,
    .release = kmod_release,
    .unlocked_ioctl = kmod_ioctl,
};

bool kmod_ioctl_init(void) {
    if (alloc_chrdev_region(&dev, 0, 1, "usbaccess") < 0) return false;
    cdev_init(&kmod_cdev, &fops);
    if (cdev_add(&kmod_cdev, dev, 1) < 0) goto cdevfailed;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6,2,16)
    if ((kmod_class = class_create(THIS_MODULE, "kmod_class")) == NULL) goto cdevfailed;
#else
    if ((kmod_class = class_create("kmod_class")) == NULL) goto cdevfailed;
#endif
    if ((device_create(kmod_class, NULL, dev, NULL, "kmod")) == NULL) goto classfailed;
    return true;
classfailed:
    class_destroy(kmod_class);
cdevfailed:
    unregister_chrdev_region(dev, 1);
    return false;
}

void kmod_ioctl_teardown(void) {
    if (kmod_class) {
        device_destroy(kmod_class, dev);
        class_destroy(kmod_class);
    }
    cdev_del(&kmod_cdev);
    unregister_chrdev_region(dev, 1);
}
