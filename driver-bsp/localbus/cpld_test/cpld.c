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



#define CMD_EXTMEM_READ                       0x80205206
#define CMD_EXTMEM_WRITE                      0x80205208

//CPLD���Ĵ���ƫ�Ƶ�ַ
#define CPLD_VER_REG_OFFSET                   0x01
#define PCB_VER_REG_OFFSET                    0x02
#define INT_FLAG_REGET                        0x10
#define PONSTAT_REG_OFFSET                    0x11
#define SYSLED_REG_OFFSET                     0x12
#define SLICLED_REG_OFFSET                    0x09
#define WDEN_REG_OFFSET                       0x13
#define PONCON_REG_OFFSET                     0x14
#define INT_EN_REG_OFFSET                     0x15
#define INT_CLR_REG_OFFSET                    0x16
#define MURESET_REG_OFFSET                    0x17
#define CPU_RESET_OK_OFFSET					  0x18	

#define DSPRST_BIT                     		  0x08
#define SWRST_BIT                             0x04
#define SLIC1RST_BIT                          0x02
#define SLIC2RST_BIT                          0x01

#define RES_ERR          1
#define RES_OK           0
static int fd_ext_mem = -1;

struct  write_CPLD_REG
{
	 int index;
	 int offset;
     int value;
};

struct read_CPLD_REG
{
	 int index;
	 int offset;
     int  *p_data;
     int data;
};

/******************************************************************************
*
*CPLD��ʼ������
*
*���� : ��
*
*��� : ��

* ����:���豸����ȡ�豸���
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/
int cpld_init(void)
{
    static int i = 0;
	char buf[64];
	if(i==0)
    {
        if( (fd_ext_mem = open("/dev/hwver_dev",O_RDWR ))== -1 )     
        {     
              printf("open cpld failed \n");
              return   RES_ERR;     
        } 
	 	i = 1;
    }
	
    return  RES_OK;   
}   

/******************************************************************************
*
*��ȡCPLD �汾��Ϣ
*
*����	valueָ�룬ָ�򱣴浱ǰCPLD �汾ֵ�ı���
*
*��� valueָ�룬ָ�򱣴浱ǰCPLD �汾ֵ�ı���:

*��ǰֵΪ0x10����ʾV1.0
  
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/
int get_cpld_ver(char *value)
{
    int rv = -1;
    struct read_CPLD_REG    read_cpld_reg;
	read_cpld_reg.index = 1;
    read_cpld_reg.offset= CPLD_VER_REG_OFFSET;
    read_cpld_reg.p_data = &read_cpld_reg.data;
	
    if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_READ, &read_cpld_reg)) !=0)
    {
         printf("get_cpld_ver:cpld read failed!!\n") ;
	  	 return RES_ERR;
    }
    else
    {
        *value = (char)read_cpld_reg.data;          
    }
	
    return RES_OK;
}

/******************************************************************************
*
*led �Ƶ�������
*
*����	value: ��ʾҪд�������
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_led_onoff(char value)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= SLICLED_REG_OFFSET;
    wirte_cpld_reg.value = value;
    if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}
/******************************************************************************
*
*ϵͳ�ƿ���
*
*����	value: 1��ʾ���� 0��ʾϨ�� 2��ʾ��˸
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_sysled(char value)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= SYSLED_REG_OFFSET;
	if(value == 1)
	{
		wirte_cpld_reg.value = 0xAA;
	}
	else if(value == 2)
	{
		wirte_cpld_reg.value = 0x55;
	}
	else
	{
		wirte_cpld_reg.value = 0;
	}


	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }

	//read();
	
    return RES_OK;
}

/******************************************************************************
*
*���Ź�ʹ��
*
*����	value: 1��ʾʹ�� 0��ʾ�ر�
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_watchdog(char value)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= WDEN_REG_OFFSET;
	if(value == 1)
	{
		wirte_cpld_reg.value = 0xAA;
	}
	else
	{
		wirte_cpld_reg.value = 0;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}

/******************************************************************************
*
*������λ
*
*����	value: 1��ʾ��λ 0��ʾ����λ
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_switch_reset(char value)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= MURESET_REG_OFFSET;
	if(value == 1)
	{
		wirte_cpld_reg.value = SWRST_BIT;
	}
	else
	{
		wirte_cpld_reg.value = 0;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}
/******************************************************************************
*
*dsp��λ
*
*����	value: 1��ʾ��λ 0��ʾ����λ
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_dsp_reset(char value)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= MURESET_REG_OFFSET;
	if(value == 1)
	{
		wirte_cpld_reg.value = DSPRST_BIT;
	}
	else
	{
		wirte_cpld_reg.value = 0;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}

/******************************************************************************
*
*slic1��λ
*
*����	value: 1��ʾ��λ 0��ʾ����λ
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_slic1_reset(char value)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= MURESET_REG_OFFSET ;
	if(value == 1)
	{
		wirte_cpld_reg.value = SLIC1RST_BIT;
	}
	else
	{
		wirte_cpld_reg.value = 0;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}
/******************************************************************************
*
*slic2��λ
*
*����	value: 1��ʾ��λ 0��ʾ����λ
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_slic2_reset(char value)
{
    int rv = -1;
    struct  write_CPLD_REG  wirte_cpld_reg;
    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= MURESET_REG_OFFSET ;
	if(value == 1)
	{
		wirte_cpld_reg.value = SLIC2RST_BIT;
	}
	else
	{
		wirte_cpld_reg.value = 0;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}
/******************************************************************************
*
*��ѯ��ģ������״̬
*
*����	��
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_pon_stat(char *value)
{
    int rv = -1;
    struct read_CPLD_REG    read_cpld_reg;
	read_cpld_reg.index = 1;
    read_cpld_reg.offset= PONSTAT_REG_OFFSET;
    read_cpld_reg.p_data = &read_cpld_reg.data;
	
    if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_READ, &read_cpld_reg)) !=0)
    {
         printf("get_cpld_ver:cpld read failed!!\n") ;
	  	 return RES_ERR;
    }
    else
    {
        *value = (char)read_cpld_reg.data;  
		printf("cpld stat 0x%x\n",*value);
    }

	read_cpld_reg.index = 1;
    read_cpld_reg.offset= INT_FLAG_REGET;
    read_cpld_reg.p_data = &read_cpld_reg.data;
	
    if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_READ, &read_cpld_reg)) !=0)
    {
         printf("get_cpld_ver:cpld read failed!!\n") ;
	  	 return RES_ERR;
    }
    else
    {
        *value = (char)read_cpld_reg.data;  
		printf("int stat 0x%x\n",*value);
    }
	
    return RES_OK;
}

/******************************************************************************
*
*��ģ��Ӳ���л�
*
*����	1:�ر�Ӳ���л� 0:��Ӳ���л�
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_hd_switch(char value)
{
    int rv = -1;
	unsigned char val;
    struct read_CPLD_REG    read_cpld_reg;
    struct  write_CPLD_REG  wirte_cpld_reg;
	read_cpld_reg.index = 1;
    read_cpld_reg.offset= PONCON_REG_OFFSET;
    read_cpld_reg.p_data = &read_cpld_reg.data;
	
    if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_READ, &read_cpld_reg)) !=0)
    {
         printf("get_cpld_ver:cpld read failed!!\n") ;
	  	 return RES_ERR;
    }
    else
    {
        val = (char)read_cpld_reg.data;          
    }

    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= PONCON_REG_OFFSET;
	if(value == 1)
	{
		wirte_cpld_reg.value = val|0x04;
	}
	else
	{
		wirte_cpld_reg.value = val&0x03;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}

/******************************************************************************
*
*��ģ��0��Դ����
*
*����	1:�رշ��͵�Դ 0:�򿪷��͵�Դ
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_pon1_power(char value)
{
    int rv = -1;
	unsigned char val;
    struct read_CPLD_REG    read_cpld_reg;
    struct  write_CPLD_REG  wirte_cpld_reg;
	read_cpld_reg.index = 1;
    read_cpld_reg.offset= PONCON_REG_OFFSET;
    read_cpld_reg.p_data = &read_cpld_reg.data;
	
    if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_READ, &read_cpld_reg)) !=0)
    {
         printf("get_cpld_ver:cpld read failed!!\n") ;
	  	 return RES_ERR;
    }
    else
    {
        val = (char)read_cpld_reg.data;          
    }

    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= PONCON_REG_OFFSET;
	if(value == 1)
	{
		wirte_cpld_reg.value = val|0x02;
	}
	else
	{
		wirte_cpld_reg.value = val&0x05;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}

/******************************************************************************
*
*��ģ��1��Դ����
*
*����	1:�رշ��͵�Դ 0:�򿪷��͵�Դ
*
*��� : ��
*
*����	RES_OK  �ɹ�
*            RES_ERR ʧ��
*/

int cpld_pon2_power(char value)
{
    int rv = -1;
	unsigned char val;
    struct read_CPLD_REG    read_cpld_reg;
    struct  write_CPLD_REG  wirte_cpld_reg;
	read_cpld_reg.index = 1;
    read_cpld_reg.offset= PONCON_REG_OFFSET;
    read_cpld_reg.p_data = &read_cpld_reg.data;
	
    if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_READ, &read_cpld_reg)) !=0)
    {
         printf("get_cpld_ver:cpld read failed!!\n") ;
	  	 return RES_ERR;
    }
    else
    {
        val = (char)read_cpld_reg.data;          
    }

    wirte_cpld_reg.index = 1;
    wirte_cpld_reg.offset= PONCON_REG_OFFSET;
	if(value == 1)
	{
		wirte_cpld_reg.value = val|0x01;
	}
	else
	{
		wirte_cpld_reg.value = val&0x06;
	}
	if((rv = ioctl(fd_ext_mem, CMD_EXTMEM_WRITE, &wirte_cpld_reg)) != 0) 
    {
            printf("console_switch:cpld write failed!!\n") ;
	     return RES_ERR;
    }
	
    return RES_OK;
}
/*
int main()
{
	unsigned char cc;
	printf("this is cpld test\n");
	cpld_init();
	get_cpld_ver(&cc);
	printf("cpld ver = %x\n",cc);
    cpld_led_onoff(0x00);
	cpld_pon_stat(&cc);
	printf("pon stat = %x\n",cc);

	printf("sysy led\n");
	cpld_sysled(1);
	sleep(2);
	cpld_sysled(0);
	
	printf("switch reset\n");
	cpld_switch_reset(1);
	sleep(2);
	cpld_switch_reset(0);

	printf("dps reset\n");
	cpld_dsp_reset(1);
	sleep(2);
	cpld_dsp_reset(0);

	printf("slic1 reset\n");
	cpld_slic1_reset(1);
	sleep(2);
	cpld_slic1_reset(0);

	printf("slic2 reset\n");
	cpld_slic2_reset(1);
	sleep(2);
	cpld_slic2_reset(0);

	lseek(fd_ext_mem, 0, SEEK_SET);
	read(fd_ext_mem,buf,32);
	for(i=0;i<32;i++)
		printf("%d:%x\n",i,buf[i]);

	lseek(fd_ext_mem, 0x17, SEEK_SET);
	buf[0] = 0x04;
	write(fd_ext_mem,buf,1);
	sleep(1);
	lseek(fd_ext_mem, 0x17, SEEK_SET);
	buf[0] = 0x00;
	write(fd_ext_mem,buf,1);

	cpld_watchdog(1);

	close(fd_ext_mem);
	return 0;
}*/

