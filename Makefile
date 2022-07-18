obj-m += rtcN.o
PWD := $(shell pwd)
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

load:
	-sudo rmmod rtcN
	sudo dmesg -C
	sudo insmod rtcN.ko
	sudo dmesg
unload:
	sudo rmmod rtcN.ko
	sudo dmesg

check:
	-lsmod | grep rtcN
	-ls /proc | grep rtc
	-cat /proc/modules | grep rtc
	-ls -la /dev/ | grep rtc

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean