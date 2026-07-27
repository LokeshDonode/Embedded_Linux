obj-m := at24c32_driver.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

.PHONY: all clean load unload test

all: module userapp

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

userapp:
	$(CC) -Wall -o at24c32_test at24c32_test.c -I.

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f at24c32_test

load:
	# Instantiate without DT (quick bench test)
	echo at24c32 0x57 > /sys/bus/i2c/devices/i2c-1/new_device
	sudo insmod at24c32_driver.ko
	dmesg | tail -5

unload:
	sudo rmmod at24c32_driver
	echo 0x57 > /sys/bus/i2c/devices/i2c-1/delete_device

test:
	sudo ./at24c32_test
