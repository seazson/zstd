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
#include "test_mdio.h"

#define RES_ERR 1
#define RES_OK 0

#define SWITCH_53284   1               //BCM53604

int main(int argc , char **argv)
{    
    int rv = -1 ;
    int tmp = 0;
    unsigned short phy_id;
    unsigned short reg;
    unsigned short data;
    
#ifdef SWITCH_53284
    unsigned char page_addr;
    unsigned char reg_addr;
    unsigned long long val = 0;
    unsigned short c;
    unsigned short d;
    unsigned short e;
    unsigned short f;
#endif

    if((argc != 4 && argc != 5))
    {
        printf("usage:\r\n");
        printf("./app_mdio read phy_addr reg_addr\r\n");
        printf("./app_mdio write phy_addr reg_addr val\r\n");
#ifdef SWITCH_53284
        printf("./app_mdio sw_read page_addr reg_addr\r\n");
        printf("./app_mdio sw_write page_addr reg_addr val\r\n");
#endif
        exit(1);
    }
	
    rv = drv_mdio_open();
    if(rv == RES_ERR)
    {
    	 printf("\nopen mdio dev fail!\n");

	 return RES_ERR;
    }
	
    if(strncmp(argv[1], "read", 4) == 0)
    {
        phy_id = strtoul(argv[2], NULL, 16);      
        reg = strtoul(argv[3], NULL, 16);   
		
        if((rv = drv_mdio_read(phy_id,reg,&data))!=0)
        {
            printf("\nmdio read failed!!\n") ;
        }
        else
        {
            printf("\naddr : 0x%8.8x = val : 0x%8.8x\n", reg, data) ;
        }
    }
    else if(strncmp(argv[1], "write", 5) == 0)
    {
        phy_id = strtoul(argv[2], NULL, 16);
        reg = strtoul(argv[3], NULL, 16);
        data = strtoul(argv[4], NULL, 16);
		
        if((rv = drv_mdio_write(phy_id,reg,data))!=0)
        {
            printf("\nmdio write failed!!\n") ;
        }
        else
        {
            printf("\nmdio write success!!\n") ;
        }
    }
    else if(strncmp(argv[1], "sw_read", 7) == 0)
    {
        page_addr = strtoul(argv[2], NULL, 16);      
        reg_addr = strtoul(argv[3], NULL, 16);   
		    rv = drv_mdio_write(0x1e, 0x10, ((page_addr<<8)|0x01));
		    rv = drv_mdio_write(0x1e, 0x11, ((reg_addr<<8)|0x02));
		    rv = drv_mdio_read(0x1e, 0x18, &f);
		    rv = drv_mdio_read(0x1e, 0x19, &e);
		    rv = drv_mdio_read(0x1e, 0x1a, &d);
		    rv = drv_mdio_read(0x1e, 0x1b, &c);
		    val =(((unsigned long long)c<<48)|((unsigned long long)d<<32)|((unsigned long long)e<<16)|((unsigned long long)f)); 
        if(rv!=0)
        {
            printf("\nmdio sw_read failed!!\n") ;
        }
        else
        {
            printf("\nval : 0x%64.64x\n", val) ;
        }
    }
    else if(strncmp(argv[1], "sw_write", 8) == 0)
    {
        page_addr = strtoul(argv[2], NULL, 16);      
        reg_addr = strtoul(argv[3], NULL, 16); 
        val = strtoul(argv[4], NULL, 16);
		    c = (unsigned short)(val>>48);
		    d = (unsigned short)(val>>32);
		    e = (unsigned short)(val>>16);
		    f = (unsigned short)(val);
		    rv = drv_mdio_write(0x1e, 0x10, ((page_addr<<8)|0x01)); 
		    rv = drv_mdio_write(0x1e, 0x18, f);
		    rv = drv_mdio_write(0x1e, 0x19, e);
		    rv = drv_mdio_write(0x1e, 0x1a, d);
		    rv = drv_mdio_write(0x1e, 0x1b, c);
		    rv = drv_mdio_write(0x1e, 0x11, ((reg_addr<<8)|0x01)); 
        if(rv!=0)
        {
            printf("\nmdio sw_write failed!!\n") ;
        }
        else
        {
            printf("\nmdio sw_write success!!\n") ;
        }
    }
    else
    {
        printf("\ncommand error!!\n") ;
    } 
	
    drv_mdio_close();
   //  while(1);
    return 0;
    
}


