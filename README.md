k# USB Block Device Access Kernel Module

## Overview
This Linux kernel module provides an interface for reading from and writing to a USB block device via custom IOCTL commands. It supports both full-block operations and byte-offset operations.  
Developed as part of **CSE 330 – Operating Systems** coursework at Arizona State University.

---

## Features
- **BREAD / BWRITE** — Read or write entire blocks from a USB storage device.
- **BREADOFFSET / BWRITEOFFSET** — Read or write from specific byte offsets.
- Uses the Linux BIO layer (`bio_alloc`, `submit_bio_wait`) for efficient block device I/O.
- Compatible with **Linux kernel 6.10** API changes.
- Character device interface created at `/dev/kmod`.

---

## Technologies Used
- **Language:** C  
- **Platform:** Linux Kernel 6.10  
- **APIs:** BIO layer, IOCTL interface, block device APIs

---

## How It Works
1. Module registers a character device `/dev/kmod`.
2. Users can send IOCTL commands for reading/writing blocks or byte ranges.
3. Internally uses the Linux block device APIs and BIO structures for efficient I/O.
4. Proper error handling for memory allocation, device access, and invalid parameters.

---

## Build & Run
```bash
# 1. Build the Module
make

# 2. Insert the Module
sudo insmod kmod.ko

# 3. Create Device Node (replace <major> with number from dmesg)
sudo mknod /dev/kmod c <major> 0

# 4. Remove the Module
sudo rmmod kmod

