obj-m += kmod.o
kmod-objs := kmod-main.o kmod-ioctl.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo rmmod kmod || true
	sudo insmod kmod.ko device=/dev/sda1

remove:
	sudo rmmod kmod || true
