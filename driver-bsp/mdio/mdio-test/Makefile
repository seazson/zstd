
ifeq ($(device),ppc_83xx)
CROSS_COMPILE=powerpc-e300c3-linux-gnu-
else 
ifeq ($(device),arm_18xx)
CROSS_COMPILE=arm-none-linux-gnueabi-
else 
ifeq ($(device),mips_bcm)
CROSS_COMPILE=/opt/toolchains/uclibc-crosstools-gcc-4.4.2-1-voyager/usr/bin/mips-linux-uclibc-
else 
ifeq ($(device),opl)
CROSS_COMPILE=/opt/openwrt/kamikaze_7.09/staging_dir_mips/bin/mips-linux-uclibc-
endif
endif
endif
endif

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

DIR_CURRENT =$(shell pwd)
INCLUDEFLAGS :=  -I$(DIR_CURRENT)
CFLAGS = $(INCLUDEFLAGS)

SRC_FILES := test_mdio.c main.c
OBJ_FILES := $(SRC_FILES:%.c=%.o)


all:  clean $(OBJ_FILES)
	@$(CC) $(OBJ_FILES) -o test_mdio
	@rm -rf $(OBJ_FILES)

%.o:%.c
	@$(CC) $(CFLAGS) -c $< -o $@


clean:
	@rm -rf  test_mdio $(OBJ_FILES) *.bak

help:
	@echo 'make device=ppc_83xx'
	@echo 'make device=arm_18xx'
	@echo 'make device=mips_bcm'
	@echo 'make device=opl'
	