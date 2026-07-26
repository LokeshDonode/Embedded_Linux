# Usage:
#
#   Build default module (hello.c)
#       make
#
#   Build a specific module
#       make MODULE=gpio_driver
#       make MODULE=uart_driver
#       make MODULE=led_driver
#
#   Clean build files
#       make clean

MODULE ?= hello

obj-m += $(MODULE).o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
        make -C $(KDIR) M=$(PWD) modules

clean:
        make -C $(KDIR) M=$(PWD) clean