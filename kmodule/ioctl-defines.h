#ifndef IOCTL_DEFINES_H
#define IOCTL_DEFINES_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define USB_MAGIC       'u'
#define BREAD           _IOWR(USB_MAGIC, 1, struct kmod_rw_request)
#define BWRITE          _IOW (USB_MAGIC, 2, struct kmod_rw_request)
#define BREADOFFSET     _IOWR(USB_MAGIC, 3, struct kmod_rwoffset_request)
#define BWRITEOFFSET    _IOW (USB_MAGIC, 4, struct kmod_rwoffset_request)

struct kmod_rw_request {
    void __user *data;
    size_t       size;
};

struct kmod_rwoffset_request {
    void __user *data;
    size_t       size;
    loff_t       offset;
};

#endif
