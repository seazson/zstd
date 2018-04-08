# make PORT_NUM=DEVICE_PORT_2

### software versions definitions ###
DRIVER_VERSION := 1.0.1

EXTRA_CFLAGS += -I. -Wall -O2 
#CFLAGS  += -O2

CROSS_COMPILE=powerpc-e300c3-linux-gnu-
AS	:= $(CROSS_COMPILE)as
LD	:= $(CROSS_COMPILE)ld
CC	:= $(CROSS_COMPILE)gcc
CPP	:= $(CC) -E
AR	:= $(CROSS_COMPILE)ar
NM	:= $(CROSS_COMPILE)nm
STRIP	:= $(CROSS_COMPILE)strip
SSTRIP	:= $(CROSS_COMPILE)sstrip
OBJCOPY	:= $(CROSS_COMPILE)objcopy
OBJDUMP	:= $(CROSS_COMPILE)objdump

### list of files to compile ###

obj-m	:= ppc_ssruart.o
ppc_ssruart-objs := rctty_io.o rcNtty.o rcserial_core.o ucc_uart.o ucc_slow.o ucc.o serial_xr16m752.o

.PHONY: all clean
KERNEL_SOURCE :=/ppc/kernel/linux-2.6.34
PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules
	$(STRIP) --strip-unneeded ppc_ssruart.ko

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

