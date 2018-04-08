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
//#include <linux/of_platform.h>
//#include <linux/of_gpio.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/cdev.h>
//#include <asm/fs_pd.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/pagemap.h>


#define MIX_MAJOR    173
#define DEVICE_NAME  "ext_mix"
static struct cdev drvmix_cdev;

/*i2c lock*/
#define CMD_I2C_LOCK           0x10405206
#define CMD_I2C_UNLOCK         0x10405207

struct mutex i2c_mutex;

/*****************************************
* ext_mem file operation encap
*****************************************/
static int drvmix_open(struct inode * inode, struct file * filp)
{
			printk("open mix\n");
       return 0;
}

static int drvmix_release(struct inode * inode, struct file * filp)
{
	       
       return 0;
}

static ssize_t drvmix_read(struct file * file, char __user * buf, size_t count, loff_t *ppos)
{	    
	return -EINVAL;
}

static ssize_t drvmix_write(struct file * file, const char __user * buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

static int drvmix_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg)
{
    switch(cmd) 
    {	 
    	 case CMD_I2C_LOCK:  
    	 {  	  	
          mutex_lock(&i2c_mutex);
    	    break;    	        	      
    	 }  
    	 case CMD_I2C_UNLOCK:  
    	 {        
	        mutex_unlock(&i2c_mutex);
    	    break;    	         	      
    	 }   
   	   default:
    	    break;  	  	
    }
    
    return 0;
}

static struct file_operations drvmix_fops = {
	.llseek		= NULL,
	.read		  = drvmix_read,
	.write		= drvmix_write,
	.poll		  = NULL,
	.ioctl		= drvmix_ioctl,
	.open		  = drvmix_open,
	.release	= drvmix_release,
	.fasync		= NULL,
};

static int drvmix_init(void)
{
    int ret;
    
    int devno = MKDEV(MIX_MAJOR,1);
    
    printk("mix drv ver: %s %s \n",__DATE__,__TIME__);
            
    cdev_init(&drvmix_cdev, &drvmix_fops);
	  drvmix_cdev.owner = THIS_MODULE;
	  
    ret = cdev_add(&drvmix_cdev, devno, 1);
    if(ret)
    {      
    	  printk("cannot register mutex dev\n"); 
        return ret;  
    }   

    mutex_init(&i2c_mutex);
	
    return 0;	
}

static void drvmix_free(void)
{
    
    cdev_del(&drvmix_cdev);
    return;
}

MODULE_LICENSE("GPL");
module_init(drvmix_init);
module_exit(drvmix_free);
