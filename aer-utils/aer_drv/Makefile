ifneq ($(KERNELRELEASE),)
obj-m :=aer_inject.o
else
KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.ko *.o *.mod.o *.symvers *.order *.mod.c
endif
