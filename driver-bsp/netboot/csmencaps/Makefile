

### software versions definitions ###
DRIVER_VERSION := 1.1.0


ifndef DEVICE
DEVICE :=ISCOM5216
endif

module := csmencaps
obj-m += csmencaps.o
$(module)-objs := af_csmencaps.o


CC_PATH :=/usr/local/ARM/armv5-marvell-linux-uclibcgnueabi-soft_i686/bin/:/bin:/sbin:/usr/bin
KERNEL_SOURCE :=/kernel/linux-2.6.32.11
CROSS_COMPILE :=arm-marvell-linux-uclibcgnueabi-

#EXTRA_CFLAGS += -I.
EXTRA_CFLAGS := -I$(src)/

CC :=$(CROSS_COMPILE)gcc
LD :=$(CROSS_COMPILE)ld

TOP_DIR := $(shell cd .; pwd)

obj-m += $(module).o

.PHONY: all clean

all:
	@( PATH=$(CC_PATH);\
	$(CC) --version;\
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(TOP_DIR) modules )

clean:
	@( PATH=$(CC_PATH);\
	$(MAKE) KBUILD_VERBOSE=1 -C $(KERNEL_SOURCE) SUBDIRS=$(TOP_DIR) clean)






