###########################################################################################
#
#
#
###########################################################################################
VERSION = 1.0.0
APPNAME = test_cpld

AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
CC      = $(CROSS_COMPILE)gcc
CPP     = $(CC) -E
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
STRIP   = $(CROSS_COMPILE)strip
SSTRIP  = $(CROSS_COMPILE)sstrip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
ECHO    = echo

CC_PATH:=/opt/openwrt/kamikaze_7.09/staging_dir_mips/bin/:/usr/kerberos/bin:/usr/local/bin:/bin:/usr/bin:/usr/X11R6/bin:/usr/atria/bin:/usr/atria/etc:/usr/bin/X11:/opt/gcc-4.1.78-eglibc-2.5.78-1/powerpc-e300c3-linux-gnu/bin:./
CROSS_COMPILE=mips-linux-uclibc-

	LDFLAGS = -lpthread -lm -lrt  
	CDEFS :=  -DARCH=mips -DDUAL_PON_D -pipe 
	CFLAGS_REL = -Os 
	OSFLAG =   $(CDEFS)

	
#    SRCS_COMMON = ./cpld.c ./z_test.c
     SRCS_COMMON = ./local_bus.c 
    


#################################This is rule define##########################################
#
#
#
##############################################################################################
SRCS = $(SRCS_COMMON) 
WARNING = -Wall -W -Wno-unused-parameter
CFLAGS = $(OSFLAG) $(WARNING) $(INCLUDEFLAGS) $(CFLAGS_REL) 
OBJS = $(SRCS:%.c=%.o)


.PHONY: all install clean

all: clean $(OBJS)	
	@( PATH=$(CC_PATH);\
	$(CC) $(OBJS) $(LDFLAGS) -o $(APPNAME);\
	$(STRIP) $(APPNAME) )
	@echo Finish!!!!!

%.o: %.c
	@( PATH=$(CC_PATH);\
	echo compiling  $<;\
	$(CC) $(CFLAGS) -c $<  -o $@ )


clean:
	rm -rf $(OBJS)
	rm -rf $(APPNAME)


