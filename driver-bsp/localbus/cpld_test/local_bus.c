
#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>

//#include "../h/local_bus.h"
#include "local_bus.h"

#define CMD_LOCALBUS_READ           0x80205206
#define CMD_LOCALBUS_WRITE          0x80205208
#define CMD_LOCALBUS_FIELD          0x80205207
#define CMD_LOCALBUS_CFG            0x80205209

static int  fd_bus_dev = -1;

struct  write_CPLD_REG
{
	int index;
	int offset;
	int field;
	int width;
	int value;
};

struct read_CPLD_REG
{
	int index;
	int offset;
	int  *p_data;
	int data;
};

static void sig_handler(int sig)
{
	/*dushaobo:SIGUSR2 ��OAM�������ü���ʱ�Ѿ�ʹ�ã�
	���Դ˴���Ϊʹ��SIGUSR1*/
	//if (kill(protect_pid, SIGUSR1) < 0) 
	{   
		printf("pon switch\r\n");
	}
}

/*******************************************************************************
*����:��ʼ��local_bus_dev
*
*����:��
*
*���:��	
*
*�� �� ֵ����������
*  
*/
int oplLocalBusDevInit(void)
{	

    struct sigaction action;

    memset(&action,0,sizeof(struct sigaction));
	action.sa_handler = &sig_handler;
	sigemptyset(&action.sa_mask);	
    action.sa_flags = 0;
	sigaction(SIGUSR2, &action, NULL);
    
    
    if( (fd_bus_dev=open("/dev/hwver_dev", O_RDWR ))== -1 ) 
    {   
        printf(("open /dev/local_bus_dev failed.\n"));
        return -1;
    }

    return 0;	
}

/*******************************************************************************
*����: ��local bus
*
*����: offset ƫ���ֽ���
*      num_of_bytes
*      buf
*���:�汾��	
*
*�� �� ֵ����������
*  
*/

int oplLocalBusRead(UINT32 offset, UINT32 nbytes, UINT8 *buf )
{
	int ret;
	
	if(-1 == fd_bus_dev)
	{
		 return -1;
	}
  lseek(fd_bus_dev,offset,SEEK_SET);
	ret = read(fd_bus_dev,buf,nbytes);
	if(ret != nbytes)
	{
		return -1;
	}
	return 0;
}

/*****************************************************************************
 �� �� ��  : local_bus_write
 ��������  : дlocal bus
 �������  : ��
 �������  : ��
 �� �� ֵ  : int
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2011��12��1��
    ��    ��   : ����
    �޸�����   : �����ɺ���

*****************************************************************************/
int oplLocalBusWrite(UINT32 offset, UINT32 nbytes, UINT8 *buf)
{
	int ret;
	if(-1 == fd_bus_dev)
	{
		 return -1;
	}
  lseek(fd_bus_dev,offset,SEEK_SET);
 	ret = write(fd_bus_dev,buf,nbytes);

	if(ret != nbytes)
	{
		printf("write less than %d\n",nbytes);
		return -1;
	}
	return 0;
}

/*****************************************************************************
 �� �� ��  : local_bus_write_field
 ��������  : ���ֽ�дlocal bus
 �������  : ��
 �������  : ��
 �� �� ֵ  : int
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2011��12��1��
    ��    ��   : ����
    �޸�����   : �����ɺ���

*****************************************************************************/
int oplLocalBusWriteField(UINT32 offset, UINT8 field, UINT8 width, UINT8 *data)
{
    struct  write_CPLD_REG  wirte_cpld_reg;
		int rv;
	if(-1 == fd_bus_dev)
	{
		 return -1;
	}

	wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= offset;
	wirte_cpld_reg.field = field;
	wirte_cpld_reg.width = width;
    wirte_cpld_reg.value = *data;
    if((rv = ioctl(fd_bus_dev, CMD_LOCALBUS_FIELD, &wirte_cpld_reg)) != 0) 
    {
         printf("set bit failed!!\n") ;
	     return -1;
    }

	return 0;
}

/*******************************************************************************
*����:�ͷ�local_bus_dev
*
*����:��
*
*���:��	
*
*�� �� ֵ����������
*  
*/

int oplLocalBusDevRelease(void)
{
	 close(fd_bus_dev);
	 fd_bus_dev = -1;

	 return 0;
}

/*******************************************************************************
*����:�����ж��ϱ�����
*
*����:��
*
*���:��	
*
*�� �� ֵ����������
*  
*/

int oplLocalBusIrqEnable(void)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
	if(-1 == fd_bus_dev)
	{
		 return -1;
	}
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= 0;
    wirte_cpld_reg.value = 1;
    if((rv = ioctl(fd_bus_dev, CMD_LOCALBUS_CFG, &wirte_cpld_reg)) != 0) 
    {
         printf("irq set enable failed!!\n") ;
	     return -1;
    }
	
    return 0;
}

/*******************************************************************************
*����:�ر��ж��ϱ�����
*
*����:��
*
*���:��	
*
*�� �� ֵ����������
*  
*/

int oplLocalBusIrqDisable(void)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
	if(-1 == fd_bus_dev)
	{
		 return -1;
	}
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= 0;
    wirte_cpld_reg.value = 0;
    if((rv = ioctl(fd_bus_dev, CMD_LOCALBUS_CFG, &wirte_cpld_reg)) != 0) 
    {
         printf("irq set disable failed!!\n") ;
	     return -1;
    }
	
    return 0;
}

unsigned char buf[100];
int main()
{
	int i;
	printf("test cpld");
	oplLocalBusDevInit();
	oplLocalBusRead(0, 0x20, buf );
	for(i=0;i<0x20;i++)
		printf("%x : %x\n",i,buf[i]);
/*
	buf[0] = 0xff;
	oplLocalBusWrite(0x17, 1, buf);
	sleep(1);
  buf[0] = 0x00;
	oplLocalBusWrite(0x17, 1, buf);
*/
buf[0] = 1;	
oplLocalBusWriteField(0x17, 2, 1, buf);
oplLocalBusIrqEnable();
while(1);
sleep(3);
oplLocalBusIrqDisable();

buf[0] = 0;	
oplLocalBusWriteField(0x17, 2, 1, buf);


	return 0;
}
