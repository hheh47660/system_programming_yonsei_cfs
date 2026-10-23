obj-m = hw1.o
target = hw1
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	rmmod $(target)
	insmod $(target).ko
clean:
	rm -rf *.ko *.mod *.mod.* .*.cmd *.o *.symvers *.order
