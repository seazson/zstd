#Comment/uncomment the following line to disable/enable debugging
#DEBUG = y

AS	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP	= $(CC) -E
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
SSTRIP	= $(CROSS_COMPILE)sstrip
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump


CROSS_COMPILE  = mips-linux-uclibc-
KERNEL  = /home/zengyang/views/oam_142_v3/package_v2/linux/Opl06750/linux-2.6.21.5_release/


#ifeq ($(DEBUG),y)
#  DEBFLAGS = -O -g  # "-O" is needed to expand inlines
#else
  DEBFLAGS = -O2
#endif

EXTRA_CFLAGS += $(DEBFLAGS)

obj-m	:= mixdev.o

PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL) M=$(PWD) ARCH=mips modules
	$(STRIP) --strip-unneeded mixdev.ko


clean:
	rm -rf *.o *.order *.bak *.symvers *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend
	
	
help:
	@echo '************************************************************'
	@echo '*                      make help                           *'
	@echo '************************************************************'
	@echo 'make arch=$(ARCH_MPC83XX_LIST)$(ARCH_MPC8XX_LIST)$(ARCH_ARM_TI_LIST)'

