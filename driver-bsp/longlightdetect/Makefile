IMAGE_MODULE_DIR := ../../image

############
CROSS_COMPILE  = mips-linux-uclibc-

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
RANLIB          = $(CROSS_COMPILE)ranlib
CPU_TYPE	= mips
export AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP CROSS_COMPILE

############
PWD := $(shell /bin/pwd)

##module
obj-m= longlightdetect.o
longlightdetect-objs = longlightdetect_dev.o
MODULE_NAME = longlightdetect.ko

############
KERNEL := zcode/opl/kernel/linux-2.6.21.5_release/

default:
	$(MAKE) -C $(KERNEL) M=$(PWD) ARCH=mips modules
#	$(STRIP)   $(MODULE_NAME)

install:
	-cp $(MODULE_NAME) $(IMAGE_MODULE_DIR)/

clean:
	rm .*.cmd *.o *.ko *.mod.c Module.symvers -rf .tmp*




