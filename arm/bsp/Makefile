##################################################################################
#												common makefile                          
#                                                   edit by seazson
#							use : make DEVICE=m82331/z2440
##################################################################################

ifndef DEVICE
DEVICE := m82331
endif

ifeq ($(DEVICE) ,m82331)
CC_PATH := ~/m82331/SiSP-L26.7.8-OpenWrt-SDK-c300v2evm/staging_dir/toolchain-arm_gcc-4.3.4_uClibc-0.9.30.1/usr/bin/:/bin:/sbin:/usr/bin
CROSS   := arm-openwrt-linux-
CHIPDIR := m82331
CFLAGS  += -I$(CURDIR)/$(CHIPDIR) 
OBJCHIP = $(CURDIR)/$(CHIPDIR)/soc.o \
          $(CURDIR)/$(CHIPDIR)/bsp.o \
          $(CURDIR)/$(CHIPDIR)/serial.o \
          $(CURDIR)/$(CHIPDIR)/timer.o
endif


##################################################################################
# common define
##################################################################################
APPNAME := Z
CURDIR   = $(shell pwd)
LDFLAGS := $(CURDIR)/$(CHIPDIR)/link.lds

CC       = $(CROSS)gcc
LD       = $(CROSS)ld
OBJCOPY  = $(CROSS)objcopy
OBJDUMP  = $(CROSS)objdump


##################################################################################
# add obj here
##################################################################################
OBJS = $(CURDIR)/arch/start.o \
		$(CURDIR)/lib/div64.o \
		$(CURDIR)/lib/printf.o \
		$(CURDIR)/common/cpuid.o \
		$(CURDIR)/common/cache.o \
		$(CURDIR)/common/mmu.o \

OBJS += $(OBJCHIP)

CFLAGS += -fno-builtin -Iinclude/
##################################################################################
# define rule here
##################################################################################
all: elf bin dis

elf: $(OBJS)
	@echo "making ${DEVICE}...." 
	@( PATH=$(CC_PATH); \
	$(LD)  -T$(LDFLAGS) -g $(OBJS) -o $(APPNAME).elf; )
	@echo "done!"   

bin: elf
	@echo "making ${DEVICE}.bin...." 
	@( PATH=$(CC_PATH); \
	$(OBJCOPY) -O binary $(APPNAME).elf $(APPNAME).bin; )
	@echo "done!"   

dis: elf
	@echo "making ${DEVICE}.dis...." 
	@( PATH=$(CC_PATH); \
	$(OBJDUMP) -D $(APPNAME).elf > $(APPNAME).dis; )
	@echo "done!"   

%.o: %.c %.S
	@( PATH=$(CC_PATH);\
	echo compiling  $<;\
	$(CC) $(CFLAGS) -c $<  -o $@ )

clean:
	rm -rf $(OBJS)
	rm -rf $(APPNAME).*

