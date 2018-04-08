#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>


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


#define MPC870_MDIO_MAJOR   168
#define MDIO_OPEN   1
#define MDIO_CLOSED 0

static struct cdev mdio_cdev;
static int mdio_opened;


struct mdio_read_s 
{
     int phy_id;         
     int location;     
     int *p_data;               
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


#ifndef OPL_OK
#define OPL_OK (0)
#endif


#ifndef OPL_ERROR
#define OPL_ERROR (-1)
#endif


#ifndef NULL
#define NULL (char*)0
#endif


#ifndef _OPLONU_TYPE_DEF
#define _OPLONU_TYPE_DEF

typedef unsigned long long      UINT64;
typedef unsigned 	int  		UINT32;
typedef unsigned 	short 	    UINT16;
typedef unsigned 	char  	    UINT8;

typedef signed 	 	int  		INT32;
typedef signed 		short 	    INT16;
typedef signed 		char  	    INT8;

typedef UINT8 		      		    BOOL_T;
typedef int					    	OPL_BOOL;
typedef int							OPL_STATUS;
typedef float						FLOAT32;
#endif /*_OPLONU_TYPE_DEF*/

#define OPL_REG_BASE          		0xBF000000
#define MDIO_READ				0x02
#define MDIO_WRITE				0x01

#define MDIO_BUSY				0x1
#define MDIO_TIMEOUT			1000
#define OPCONN_BASE_ADDR		0x00000000

#define MDIO_BA_B				 (OPCONN_BASE_ADDR + 0x0500)
#define MDIO_BA_E				 (OPCONN_BASE_ADDR + 0x0506)

#define REG_MDIO_DIV_FACTOR     		((MDIO_BA_B + 0)*4)
#define REG_MDIO_OP_PULSE				((MDIO_BA_B + 1)*4)
#define REG_MDIO_PHYAD  				((MDIO_BA_B + 2)*4)
#define REG_MDIO_REGAD					((MDIO_BA_B + 3)*4)
#define REG_MDIO_WRITE_DATA   			((MDIO_BA_B + 4)*4)
#define REG_MDIO_READ_DATA     			((MDIO_BA_B + 5)*4)
#define REG_MDIO_BUSY     				((MDIO_BA_B + 6)*4)


OPL_STATUS opl_Reg_Read(UINT32 regAddr, UINT32 *regVal)
{
	if (NULL == regVal)
	{
		return (-1);
	}
	if (regAddr % 4)
	{
		printk("invalid regiser addr.\n");
		return OPL_ERROR;
	}
	*regVal = *(UINT32 *)((UINT32)OPL_REG_BASE + regAddr);
	return OPL_OK;
}

OPL_STATUS opl_Reg_Write(UINT32 regAddr, UINT32 regVal)
{
	OPL_STATUS retVal = OPL_OK;

	if (regAddr % 4)
	{
		printk("invalid regiser addr.\n");
		return OPL_ERROR;
	}
	*(UINT32 *)((UINT32)OPL_REG_BASE + regAddr) = regVal;
	return retVal;
}

#define MDIO_REG_FIELD_READ(regAddr,fieldOffset,fieldWidth,data0) { \
	opl_Reg_Read(regAddr, &(data0)); \
	data0 = ((data0)&((~(0XFFFFFFFF<<(fieldWidth)))<<(fieldOffset)))>>(fieldOffset); \
}

#define MDIO_REG_FIELD_WRITE(regAddr,fieldOffset,fieldWidth,data0) { \
	UINT32 oldVal,fieldMask; \
	fieldMask = (~(0XFFFFFFFF<<(fieldWidth)))<<(fieldOffset); \
	opl_Reg_Read(regAddr,&oldVal); \
	opl_Reg_Write(regAddr, (((data0)<<(fieldOffset))&fieldMask)|(oldVal&(~fieldMask))); \
}

int mpc870_export_mdio_read(int deviceAddr, int regAddr, int *data0)
{
	UINT32 mdioBusy;
	UINT32 timeOut = MDIO_TIMEOUT;
	UINT32 regVal = 0;
	OPL_STATUS retVal = OPL_OK;

	/* first check that it is not busy */
	MDIO_REG_FIELD_READ(REG_MDIO_BUSY, 0, 1, mdioBusy);
	while (mdioBusy&MDIO_BUSY)
	{
		if (!timeOut--)
		{
			retVal = OPL_ERROR;
			goto exit_label;
		}
		MDIO_REG_FIELD_READ(REG_MDIO_BUSY, 0, 1, mdioBusy);
	}

	MDIO_REG_FIELD_WRITE(REG_MDIO_PHYAD, 0, 5, deviceAddr&0x1f);
	MDIO_REG_FIELD_WRITE(REG_MDIO_REGAD, 0, 5, regAddr&0x1f);
	MDIO_REG_FIELD_WRITE(REG_MDIO_OP_PULSE, 0, 2, MDIO_READ);
	MDIO_REG_FIELD_READ(REG_MDIO_BUSY, 0, 1, mdioBusy);
	timeOut = MDIO_TIMEOUT;
	while (mdioBusy&MDIO_BUSY)
	{
		if (!timeOut--)
		{
			retVal = OPL_ERROR;
			goto exit_label;
		}
		MDIO_REG_FIELD_READ(REG_MDIO_BUSY, 0, 1, mdioBusy);
	}

	MDIO_REG_FIELD_READ(REG_MDIO_READ_DATA, 0, 16, regVal);

exit_label:
	*data0 = regVal;
	return retVal;
}

int mpc870_export_mdio_write(int deviceAddr, int regAddr, u16 data0)
{
	UINT32 mdioBusy;
	UINT32 timeOut = MDIO_TIMEOUT;
	OPL_STATUS retVal = OPL_OK;

	MDIO_REG_FIELD_READ(REG_MDIO_BUSY, 0, 1, mdioBusy);
	while (MDIO_BUSY&mdioBusy)
	{
		if (!timeOut--)
		{
			retVal = OPL_ERROR;
			goto exit_label;
		}
		MDIO_REG_FIELD_READ(REG_MDIO_BUSY, 0, 1, mdioBusy);
	}

	MDIO_REG_FIELD_WRITE(REG_MDIO_PHYAD, 0, 5, deviceAddr&0x1f);
	MDIO_REG_FIELD_WRITE(REG_MDIO_REGAD, 0, 5, regAddr&0x1f);
	MDIO_REG_FIELD_WRITE(REG_MDIO_WRITE_DATA, 0, 16, data0);
	MDIO_REG_FIELD_WRITE(REG_MDIO_OP_PULSE, 0, 2, MDIO_WRITE);

exit_label:

	return retVal;
}

int mpc870_export_mdio_switch_read(mdio_switch_read_data_s *p_mdio_switch_rdata )
{
	return 0;	
}
int mpc870_export_mdio_switch_write(mdio_switch_wrire_data_s *p_mdio_switch_wdata)
{
	return 0;	
}






static int mdio_open(struct inode * inode, struct file * filp)
{

  if(MDIO_OPEN == mdio_opened)
    {
        printk("Err: mdio is opened already\n");
        return -EBUSY;
    }
    
    mdio_opened = MDIO_OPEN;
    printk("open mdio\n");
    return 0;
}


static int mdio_release(struct inode * inode, struct file * filp)
{
	
	  mdio_opened = MDIO_CLOSED;
    printk("close mdio\n");
       
	  return 0;
	
}





static ssize_t mdio_read(struct file * file, char __user * buf, size_t count, loff_t *ppos)
{	    
	return 0;
}


static ssize_t mdio_write(struct file * file, const char __user * buf, size_t count, loff_t *ppos)
{
	return 0;
}





int mdio_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg)
{
	int ret=-EINVAL;
	int value =0;
	int value2=0;
	int *p_value1;
	int *p_value2;
	struct mdio_read_s  m_mdio_read;
	struct mdio_write_s m_mdio_write;
	struct mdio_switch_wrire_data  m_mdio_sw_write;
	struct mdio_switch_read_data   m_mdio_sw_read;
	
	  switch(cmd) 
    {
    	//read
    	case CMD_MDIO_READ:
    		
    		if (copy_from_user(&m_mdio_read, (void *)arg, sizeof(struct mdio_read_s)))
        {
          #ifdef DBG_MDIO_DEV_ERR
		            printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		      #endif
         	   return -EFAULT;      
        }
 	
    		ret = mpc870_export_mdio_read(m_mdio_read.phy_id,m_mdio_read.location,&value);
  
   		 #ifdef DBG_MDIO_DEV_INFO
               printk("MDIOR:phy= 0x%x; reg= 0x%x val= 0x%x\n",m_mdio_data.phy_id,m_mdio_data.location,value); 
       #endif
       
       	if (copy_to_user(m_mdio_read.p_data, (void *)(&value), sizeof(int)) )
        {
        #ifdef DBG_MDIO_DEV_ERR
		            printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		    #endif
         	   return -EFAULT;      
        }
    		
    		break;
    	//write	
    	case CMD_MDIO_WRITE:
    		if (copy_from_user(&m_mdio_write, (void *)arg, sizeof(struct mdio_write_s)))
        {
          #ifdef DBG_MDIO_DEV_ERR
		            printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		      #endif
         	   return -EFAULT;      
        }
    		
    		ret = mpc870_export_mdio_write(m_mdio_write.phy_id, m_mdio_write.location,m_mdio_write.data);
    		
    		break;
    		
    	case CMD_MDIO_SWITCH_WRITE:	
      
    	  	if (copy_from_user(&m_mdio_sw_write, (void *)arg, sizeof(struct mdio_switch_wrire_data)))
          {
    		     #ifdef DBG_MDIO_DEV_ERR
		              printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		         #endif
         	   return -EFAULT;
    	    }
    	  
    	    ret = mpc870_export_mdio_switch_write(&m_mdio_sw_write);
    	  	
    	  	break;
    	
    	case CMD_MDIO_SWITCH_READ:	
    	
        		  	if (copy_from_user(&m_mdio_sw_read, (void *)arg, sizeof(struct mdio_switch_read_data)))
                {
        		      #ifdef DBG_MDIO_DEV_ERR
		                  printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		              #endif
             	    return -EFAULT;
        	      }
        	  
        	    p_value1 = m_mdio_sw_read.p_value[0];
        	    p_value2 = m_mdio_sw_read.p_value[1];
        	    
        	    m_mdio_sw_read.p_value[0] = &value;
        	    m_mdio_sw_read.p_value[1] = &value2;
        	    
        	    
        	    ret = mpc870_export_mdio_switch_read(&m_mdio_sw_read);
        	    
           	  if (copy_to_user(p_value1, (void *)(&value), sizeof(int)) )
              {
                  #ifdef DBG_MDIO_DEV_ERR
		                   printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		               #endif
                	   return -EFAULT;      
               }
               if (copy_to_user(p_value2, (void *)(&value2), sizeof(int)) )
               {
                   #ifdef DBG_MDIO_DEV_ERR
		                   printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		                #endif
                	   return -EFAULT;      
               }
        	    
        	  	
        	  	break;
        		
    	default:
    		
    		break;	
    		
    }
    
    return ret;
	
}
/************************
* 内核文件操作结构
*************************/

static struct file_operations mdio_fops = {
	.llseek		= NULL,
	.read		  = mdio_read,
	.write		= mdio_write,
	.poll		  = NULL,
	.ioctl		= mdio_ioctl,
	.open		  = mdio_open,
	.release	= mdio_release,
	.fasync		= NULL,
};



static int mdio_dev_init(void)
{
	  int ret;
    int devno = MKDEV(MPC870_MDIO_MAJOR,0);
    

    printk("mdio drv ver: %s %s \n",__DATE__,__TIME__);
    
        
    cdev_init(&mdio_cdev, &mdio_fops);
	  mdio_cdev.owner = THIS_MODULE;
    ret = cdev_add(&mdio_cdev, devno, 1);
    if(ret)
    {      
    	  printk("cannot register mdio dev\n"); 
        return ret;  
    }
   
    mdio_opened = MDIO_CLOSED;
    
   
  return 0;	
}

static void mdio_dev_free(void)
{
	  cdev_del(&mdio_cdev);
    return;
}


MODULE_LICENSE("GPL");
module_init(mdio_dev_init);
module_exit(mdio_dev_free);
