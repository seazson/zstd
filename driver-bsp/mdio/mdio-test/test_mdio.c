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

typedef struct mdio_switch_wrire_data
{
	unsigned int phy_id[3];
	unsigned int location[3];
	unsigned int value[3];
}mdio_switch_wrire_data_s;

typedef struct mdio_switch_read_data
{
	unsigned int phy_id[3];
	unsigned int location[3];
	unsigned int value;
	unsigned int *p_value[2];
}mdio_switch_read_data_s;

struct mdio_read_s 
{
     int phy_id;         
     int location;     
     int *p_data;    
     int data;        
};

struct mdio_write_s 
{
     int phy_id;         
     int location;     
     int data;               
};

#define CMD_MDIO_READ              0x80185206
#define CMD_MDIO_WRITE             0x80185207
#define CMD_MDIO_SWITCH_READ       0x80185208
#define CMD_MDIO_SWITCH_WRITE      0x80185209

#define	RES_OK                   0      
#define	RES_ERR                 1    

static int fdmdio=-1;

int drv_mdio_open(void)
{
    if( (fdmdio=open("/dev/mdio0",O_RDWR ))== -1 )     
    {     
         printf("open mdio failed \n");  
          
         return RES_ERR;     
    }   
   
    return RES_OK;
}

int drv_mdio_close(void)
{
    if(fdmdio != -1)
    {
	 close(fdmdio);
    }
	
    return RES_OK;
}

int drv_mdio_read(unsigned short	 phyAddr, unsigned short regAddr, unsigned short* value)
{
    int ret_value = RES_ERR;
    struct mdio_read_s mdio_read;
    
    mdio_read.phy_id = (int)phyAddr;
    mdio_read.location = (int)regAddr; 
    mdio_read.p_data =&(mdio_read.data);
    ret_value = ioctl(fdmdio, CMD_MDIO_READ, &mdio_read); 
    *value  =  (unsigned short)mdio_read.data;

    return  ret_value==RES_OK? RES_OK : RES_ERR;
}

int drv_mdio_write(unsigned short phyAddr, unsigned short regAddr, unsigned short value)
{
    int ret_value = RES_ERR;
    struct mdio_write_s mdio_write;
    
    mdio_write.phy_id = (int)phyAddr;
    mdio_write.location =(int) regAddr; 
    mdio_write.data =(int) value;
    ret_value = ioctl(fdmdio, CMD_MDIO_WRITE, &mdio_write);      

    return  ret_value==RES_OK? RES_OK : RES_ERR;
}
