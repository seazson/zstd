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

//CPLD各寄存器偏移地址
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
*CPLD初始化函数
*
*输入 : 无
*
*输出 : 无

* 功能:打开设备，获取设备句柄
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*获取CPLD 版本信息
*
*输入	value指针，指向保存当前CPLD 版本值的变量
*
*输出 value指针，指向保存当前CPLD 版本值的变量:

*当前值为0x10，表示V1.0
  
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*led 灯点亮控制
*
*输入	value: 表示要写入的数据
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*系统灯控制
*
*输入	value: 1表示点亮 0表示熄灭 2表示闪烁
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*看门狗使能
*
*输入	value: 1表示使能 0表示关闭
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*交换复位
*
*输入	value: 1表示复位 0表示不复位
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*dsp复位
*
*输入	value: 1表示复位 0表示不复位
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*slic1复位
*
*输入	value: 1表示复位 0表示不复位
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*slic2复位
*
*输入	value: 1表示复位 0表示不复位
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*查询光模块引脚状态
*
*输入	无
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*光模块硬件切换
*
*输入	1:关闭硬件切换 0:打开硬件切换
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*光模块0电源控制
*
*输入	1:关闭发送电源 0:打开发送电源
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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
*光模块1电源控制
*
*输入	1:关闭发送电源 0:打开发送电源
*
*输出 : 无
*
*返回	RES_OK  成功
*            RES_ERR 失败
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

