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

#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/kmap_types.h>
#include <asm/page.h>
#include <asm/io.h>
#include <linux/kmod.h>
#include <linux/cdev.h>
#include <asm/system.h>
#include <linux/phy.h>
#include <linux/selection.h>
#include <linux/pagemap.h>

/*OPL CS1 CS2*/
#define OPL_LOCALBUS_MAJOR    		160
#define OPL_REG_DEVICE  			"local_bus"
#define CS1_PHY_BASE          		0x17A00000
#define CS2_PHY_BASE          		0x17B00000

#define CMD_LOCALBUS_READ           0x80205206
#define CMD_LOCALBUS_WRITE          0x80205208
#define CMD_LOCALBUS_FIELD          0x80205207
#define CMD_LOCALBUS_CFG            0x80205209
#define CMD_REG_READ                0x80205210
#define CMD_REG_WRITE               0x80205211
#define CMD_LONGLIGHT_CFG			0x80205212

#define CPLD_MIN_OFFSET             0x00
#define CPLD_MAX_OFFSET             0xFF

#define DBGREG printf
#define UINT32 unsigned int
/*OPL REG*/
#define OPL_REG_BASE          		0xBF000000
#define OPL_REG_ID2ADDR(regId)      (OPL_REG_BASE + (regId))

#if 1

#define OPL_RegWrite(regId, val)   *((volatile UINT32 *)OPL_REG_ID2ADDR(regId)) = (UINT32)(val)
#define OPL_RegRead(regId, pval)   *(UINT32*)(pval) = *(volatile UINT32 *)OPL_REG_ID2ADDR(regId)

#define OPL_RegWrite_DIRECT(regId, val)   *((volatile UINT32 *)(regId)) = (UINT32)(val)
#define OPL_RegRead_DIRECT(regId, pval)   *(UINT32*)(pval) = *(volatile UINT32 *)(regId)

#else
//debug
#define DBGREG printk
#define OPL_RegWrite(regId, val) DBGREG("write 0x%08x val=0x%08x\n",OPL_REG_ID2ADDR(regId),val);\
*((volatile UINT32 *)OPL_REG_ID2ADDR(regId)) = (UINT32)(val)
#define OPL_RegRead(regId, pval) DBGREG("read  0x%08x\n",OPL_REG_ID2ADDR(regId));\
*(UINT32*)(pval) = *(volatile UINT32 *)OPL_REG_ID2ADDR(regId)

#endif

#define opl_read8(addr)           (*(volatile unsigned char *)(addr))
#define opl_write8(addr,value)   (*(volatile unsigned char *)(addr) = value)
#define opl_read16(addr)           (*(volatile unsigned short *)(addr))   /*如果是8位bus 会发起四次读*/
#define opl_write16(addr,value)   (*(volatile unsigned short *)(addr) = value)
#define opl_read32(addr)           (*(volatile unsigned int *)(addr))   /*如果是8位bus 会发起四次读*/
#define opl_write32(addr,value)   (*(volatile unsigned int *)(addr) = value)


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
	int field;
	int width;
};

struct  longlight_t
{
	int enable;
	int irq_pin;
	int detect_pin;
	int ctrl_pin;
	int value;
};


static struct cdev ext_localbus_cdev;
static u32         opl_reg_open_count = 0;
static u32         opl_reg_is_open = 0;
static u8          irq_enable = 0;
static u8          longlight_irq_enable = 0;
static int         opened =0;
static int		   localbus16=0;
struct semaphore   sema_cpld;
struct  longlight_t     longlight_cfg;
/*****************************************************************************************/
//
//							get gpio irq,and send signal
//
/*****************************************************************************************/
#define CPLD_IRQ_PIN 		 10
#define CPLD_INT_EN_REG 	 0x15
#define CPLD_INT_CLEAR_REG 	 0x16
#define CPU_RESET_OK_OFFSET	 0x18	

typedef struct irq_signal
{

	pid_t pid;
	pid_t tgid;
	struct task_struct *task;
	struct tasklet_struct	irq2sig_let;
} irq_signal;

// the irq No. will be mapped to
static unsigned int irq_switch_fiber =0;
struct irq_signal irq2sig;

static void irq2sig_action(unsigned long data)
{
	 //kill_pid(irq2sig.pid, SIGUSR2, 1);
	 struct task_struct *p;
	 
	 rcu_read_lock();
	 p = find_task_by_pid(irq2sig.pid);
	 rcu_read_unlock();
	 
	 if( p !=  irq2sig.task)
	 {
	 	 printk("info Task Lost %d \n",irq2sig.pid);
	 	 return;
	 }
	 send_sig(SIGUSR2, irq2sig.task, 1);      
//	 send_sig(SIGWINCH, irq2sig.task, 1);      

}

static irqreturn_t irq2sig_interrupt(int irq, void *dev_id)
{
	void *immap;
	immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
	//清零中断标志
	opl_write8((unsigned char*)immap+CPLD_INT_CLEAR_REG,0x01);
	opl_write8((unsigned char*)immap+CPLD_INT_CLEAR_REG,0x00);
	iounmap(immap);

	//printk("irq for pon switch\n");
	if(irq_enable)
   		tasklet_schedule(&irq2sig.irq2sig_let);
   	return IRQ_HANDLED;
}


static int irq_init(void)
{
	int res;
	int val;
	void *immap;

	//set deriction	
	OPL_RegRead(0x2c84,&val);	
	OPL_RegWrite(0x2c84, val&(~(1<<CPLD_IRQ_PIN)));
	//set terger mode 	
	OPL_RegRead(0x2c98,&val);	
	OPL_RegWrite(0x2c98, val&(~(1<<CPLD_IRQ_PIN)));
	//set nege edge 	
	OPL_RegRead(0x2ca0,&val);
	val &= ~(0x3<<(CPLD_IRQ_PIN*2));	
	OPL_RegWrite(0x2ca0, val|(0x02<<(CPLD_IRQ_PIN*2)));
		
	irq_switch_fiber = CPLD_IRQ_PIN;
	res = request_irq(irq_switch_fiber, irq2sig_interrupt, IRQF_SHARED, "irq1", irq2sig_interrupt);
	if(res < 0 )
	{
		printk("irq2sig res = 0x%x\n",res);
		return res;
	}

	immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
	//使能总中断和pon切换中断
	opl_write8((unsigned char*)immap+0x14,0x00); //关闭软件切换
	opl_write8((unsigned char*)immap+CPLD_INT_EN_REG,0x81);
	//清零中断标志
	opl_write8((unsigned char*)immap+CPLD_INT_CLEAR_REG,0x01);
	opl_write8((unsigned char*)immap+CPLD_INT_CLEAR_REG,0x00);
	iounmap(immap);
	
	return 0;			     
}	


static void irq_free(void)
{
   free_irq(irq_switch_fiber,irq2sig_interrupt);
}	 

static irqreturn_t longlight_interrupt(int irq, void *dev_id)
{
	int val;
	
	OPL_RegRead(0x2c80,&val);	
	printk("irq for longlight %x\n",val);
	
	if(val & (1<<longlight_cfg.detect_pin))
	{
		printk("long light\n");
//disable light out	
		OPL_RegRead(0x2c88,&val);	
		val = val|(1<<longlight_cfg.ctrl_pin);              
		OPL_RegWrite(0x2c88, val);
		OPL_RegRead(0x2c8c,&val);	
		val = val&(~(1<<longlight_cfg.ctrl_pin));              
		OPL_RegWrite(0x2c8c, val);
	}
	
   	return IRQ_HANDLED;
}

/*****************************************
* ext_mem file operation encap
*****************************************/


static int ext_localbus_open(struct inode * inode, struct file * filp)
{
	int res;

	if (test_and_set_bit(0, &opl_reg_is_open))
        printk(KERN_DEBUG "the /dev/opl_reg is already opened\n");
    opl_reg_open_count++;

	if(0 == opened)
	{
	//	printk("__first open__\n");
	    OPL_RegWrite(0x3008, 0x80000000); //CS1 Configure Register 0
	    OPL_RegWrite(0x3044, 0x17A00);    //cs1 base addr c11
	    OPL_RegWrite(0x3064, 0xFFFF);     //cs1 mask c19
	//	OPL_RegWrite(0x3094, 0x1);

		opened = 1;
		return 0;
	 }
	
	 return 0;
}


static int ext_localbus_release(struct inode * inode, struct file * filp)
{
	if((irq2sig.pid  == current->pid) && irq_enable == 1)
	{
		//printk("release the one %d\n",irq2sig.pid);
		irq_enable = 0;
		irq_free();
		tasklet_kill(&irq2sig.irq2sig_let);
	}
	
    opl_reg_open_count--;
    if(opl_reg_open_count==0)
    {
		//printk("release last one\n");
		clear_bit(0, &opl_reg_is_open);
		opened = 0;
    }
	return 0;
}

static loff_t ext_localbus_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret = 0;
	switch(orig)
	{
		case 0 :
			if(offset < 0)
			{
				ret = -EINVAL;
			}
			filp->f_pos = (unsigned int)offset;
			ret = filp->f_pos;
			break;
		default :
			ret = -EINVAL;
			break;
	}
	return ret;
}

static ssize_t ext_localbus_read(struct file * file, char __user * buf, size_t count, loff_t *ppos)
{	    
	int __iomem *immap;
	char kbuf[64];
	int i;
	int last;
	int data16;
	unsigned long p = *ppos;

	if(down_interruptible(&sema_cpld) !=0)
	{
		return -EINVAL;
	}
	//printk("read at %llx\n",*ppos);
	if(count>=64)
	{
		printk("error: too manay data File: %s; line: %d\n",__FILE__,__LINE__); 
		goto err;      
	}
	
	immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
	if(localbus16 == 1)
	{
		last = count%2;
		for(i=0; (i+2)<=count; i+=2)
		{
			data16 = opl_read16((unsigned char*)immap+p+i);
			kbuf[i]   = data16;
			kbuf[i+1] = (data16>>8);
	    	//printk("value = %x \r\n",data16);	
		}
		if(last!=0)
		{
			data16 = opl_read16((unsigned char*)immap+p+i);
			kbuf[i]   = data16;
			i++;
		}
	}
	else
	{
		for(i=0; i<count; i++)
		{
			kbuf[i] = opl_read8((unsigned char*)immap+p+i);
	    	//printk("value = %x %x %x %x\r\n",kbuf[i],immap, p, i);	
		}
	}
	iounmap(immap);    	
	
	if (copy_to_user(buf, kbuf, count)) 
    {
		printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		goto err;      
    }
	up(&sema_cpld);	
	return i;

err:
	up(&sema_cpld);
	return -EINVAL;
}


static ssize_t ext_localbus_write(struct file * file, const char __user * buf, size_t count, loff_t *ppos)
{
	int __iomem *immap;
	char kbuf[64];
	int i;
	int last;
	unsigned long p = *ppos;
	int data16=0;

	if(down_interruptible(&sema_cpld) !=0)
	{
		return -EINVAL;
	}
//	printk("write at %llx\n",*ppos);
	if(count>=64)
	{
		printk("error: too manay data File: %s; line: %d\n",__FILE__,__LINE__); 
		goto err;      
	}
	if (copy_from_user(kbuf, buf, count))
	{
		printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
		goto err;      
	}
	
	immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
	if(localbus16 == 1)
	{
		last = count%2;
		for(i=0; (i+2)<=count; i+=2)
		{
			data16 = kbuf[i] | (kbuf[i+1]<<8);
			opl_write16((unsigned char*)immap+p+i,data16);
//	    	printk("value = %x \r\n",data16);	
		}
		if(last!=0)
		{
			data16 = kbuf[i] & 0x00ff;
			opl_write8((unsigned char*)immap+p+i,data16);
//	    	printk("value = %x \r\n",data16);	
			i++;
		}
	}
	else
	{
		for(i=0; i<count; i++)
		{
			opl_write8((unsigned char*)immap+p+i,kbuf[i]);
//	    	printk("value = %x \r\n",kbuf[i]);	
		}
	}
	iounmap(immap); 
	
	up(&sema_cpld);
	return i;
err:
	up(&sema_cpld);
	return -EINVAL;
}


static int ext_localbus_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	unsigned int   value;
	unsigned int   fieldmask;
	unsigned int   regVal;
	unsigned int   val;
	int __iomem *immap;
	struct  write_CPLD_REG  write_cpld_reg;
	struct  read_CPLD_REG   read_cpld_reg;

	if(cmd != CMD_REG_READ && cmd != CMD_REG_WRITE)
	{
		if(down_interruptible(&sema_cpld) !=0)
		{
			return -EINVAL;
		}
	}
	
    switch(cmd) 
    {
		case CMD_LOCALBUS_READ:  
			if (copy_from_user(&read_cpld_reg, (void *)arg, sizeof(struct read_CPLD_REG)))
			{
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
			}

			if((read_cpld_reg.offset <CPLD_MIN_OFFSET) ||(read_cpld_reg.offset > CPLD_MAX_OFFSET))
			{
				printk("addr offset beyond , not allow !\n");
				goto cpld_err;      
			}
//			printk("index = %x offset = %x \r\n",read_cpld_reg.index,read_cpld_reg.offset);
			if(read_cpld_reg.index == 2)
			{
				immap = ioremap((CS2_PHY_BASE), CPLD_MAX_OFFSET);   
			}
			else
			{
				immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
			}
			if(localbus16)
				value = opl_read16((unsigned char*)immap+read_cpld_reg.offset);
			else
				value = opl_read8((unsigned char*)immap+read_cpld_reg.offset);
		    iounmap(immap);    	
//	        printk("value = %x \r\n",value);	
	
			if (copy_to_user(read_cpld_reg.p_data, (void *)(&value), sizeof(int)) )
	        {
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
	        }
     	    ret = 0;
    	break;
    	  
    	case CMD_LOCALBUS_WRITE:  
			if (copy_from_user(&write_cpld_reg, (void *)arg, sizeof(struct write_CPLD_REG)))
			{
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
			}
			if((write_cpld_reg.offset <CPLD_MIN_OFFSET) ||(write_cpld_reg.offset > CPLD_MAX_OFFSET))
			{
				printk("addr offset beyond , not allow !\n");
				goto cpld_err;      
			}
//			printk("write: index = %x offset = %x \r\n",write_cpld_reg.index,write_cpld_reg.offset);
//			printk("write: data = %x \r\n",write_cpld_reg.value);
			if(write_cpld_reg.index == 2)
			{
				immap = ioremap((CS2_PHY_BASE), CPLD_MAX_OFFSET);   
			}
			else
			{
				immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
			}
 			if(localbus16)
				opl_write16((unsigned char *)immap + write_cpld_reg.offset,write_cpld_reg.value);			 
			else
				opl_write8((unsigned char *)immap + write_cpld_reg.offset,write_cpld_reg.value);			 
    	  	iounmap(immap);    	  	 
			ret = 0;
   	    break;    	      

    	case CMD_LOCALBUS_FIELD:  
			if (copy_from_user(&write_cpld_reg, (void *)arg, sizeof(struct write_CPLD_REG)))
			{
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
			}
			if((write_cpld_reg.offset <CPLD_MIN_OFFSET) ||(write_cpld_reg.offset > CPLD_MAX_OFFSET))
			{
				printk("addr offset beyond , not allow !\n");
				goto cpld_err;      
			}
			if((write_cpld_reg.field < 0) ||(write_cpld_reg.field > 7))
			{
				printk("field beyond 0~7, not allow !\n");
				goto cpld_err;      
			}
			if((write_cpld_reg.width < 1) ||(write_cpld_reg.width > 8) || ((write_cpld_reg.width + write_cpld_reg.field) > 8))
			{
				printk("width beyond 1~8, not allow !\n");
				goto cpld_err;      
			}

//			printk("write: index = %x offset = %x \r\n",write_cpld_reg.index,write_cpld_reg.offset);
//			printk("write: data = %x \r\n",write_cpld_reg.value);
			if(write_cpld_reg.index == 2)
			{
				immap = ioremap((CS2_PHY_BASE), CPLD_MAX_OFFSET);   
			}
			else
			{
				immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
			}
			
			if(write_cpld_reg.width == 32)
			{
					fieldmask = 0xFFFFFFFF;
			}
			else
			{
					fieldmask = (~(0XFFFFFFFF<<write_cpld_reg.width))<<write_cpld_reg.field;
			}
			value = opl_read8((unsigned char*)immap+write_cpld_reg.offset);
			value = value&(~fieldmask);
			regVal = (write_cpld_reg.value<<write_cpld_reg.field)&fieldmask;
			regVal = regVal|value;
 	  		opl_write8((unsigned char *)immap + write_cpld_reg.offset,regVal);			 
 	  		iounmap(immap);    	  	 
			ret = 0;
   	  break;    	      
    	      
		case CMD_LOCALBUS_CFG:  
			if (copy_from_user(&write_cpld_reg, (void *)arg, sizeof(struct write_CPLD_REG)))
			{
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
			}
			switch(write_cpld_reg.value)
			{
				case 0:  /*disable irq*/
					if(irq_enable != 0)
					{
						irq_enable = 0;
						irq_free();
						tasklet_kill(&irq2sig.irq2sig_let);
					}
					ret = 0;
					break;
				case 1:  /*enable irq*/
					if(irq_enable == 1)
					{
						printk("irq has already enable\n");
						goto cpld_err;
					}
					printk("init pon switch irq!!!\n");
					irq2sig.task  = current;
					irq2sig.tgid  = current->tgid;
					irq2sig.pid   = current->pid;	 	 	

			 	 	tasklet_init(&irq2sig.irq2sig_let, irq2sig_action, 0);
				 	 	
					ret = irq_init();
					if(ret <0 )
					{
						printk("init irq error\n");
						goto cpld_err;
					}
					irq_enable = 1;
					break;
				case 8:  /*localbus 8bits width mode*/
					localbus16 = 0;
				    OPL_RegWrite(0x3008, 0x80000000); //CS1 Configure Register 0
				    OPL_RegWrite(0x3044, 0x17A00);    //cs1 base addr c11
				    OPL_RegWrite(0x3064, 0xFFFF);     //cs1 mask c19
					OPL_RegWrite(0x3094, 0x1);					

				    OPL_RegWrite(0x3010, 0x80000000); //CS2 Configure Register 0
				    OPL_RegWrite(0x3048, 0x17B00);    //cs2 base addr c11
				    OPL_RegWrite(0x3068, 0xFFFF);     //cs2 mask c19
					ret = 0;
					break;
				case 16:  /*localbus 16bits width mode*/
					localbus16 = 1;
				    OPL_RegWrite(0x3008, 0x80000004); //CS1 Configure Register 0
				    OPL_RegWrite(0x3044, 0x17A00);    //cs1 base addr c11
				    OPL_RegWrite(0x3064, 0xFFFF);     //cs1 mask c19
					OPL_RegWrite(0x3094, 0x0);		  //必须设置此位和16bits位			
				    OPL_RegWrite(0x3000, 0xC0000004); //CS0 cs0也必须改为16位

				    OPL_RegWrite(0x3010, 0x80000004); //CS2 Configure Register 0
				    OPL_RegWrite(0x3048, 0x17B00);    //cs2 base addr c11
				    OPL_RegWrite(0x3068, 0xFFFF);     //cs2 mask c19
					ret = 0;
					break;
					
				default :
					break;
			}
		break;  

    	case CMD_REG_READ:  
			if (copy_from_user(&read_cpld_reg, (void *)arg, sizeof(struct read_CPLD_REG)))
			{
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
			}

			if((read_cpld_reg.field < 0) ||(read_cpld_reg.field > 31))
			{
				printk("field beyond 0~31, not allow !\n");
				goto cpld_err;      
			}
			if((read_cpld_reg.width < 1) ||(read_cpld_reg.width > 32) || ((read_cpld_reg.width + read_cpld_reg.field) > 32))
			{
				printk("width beyond 1~32, not allow !\n");
				goto cpld_err;      
			}

//			printk("write: index = %x offset = %x \r\n",read_cpld_reg.index,read_cpld_reg.offset);
			if(read_cpld_reg.width == 32)
			{
					fieldmask = 0xFFFFFFFF;
			}
			else
			{
					fieldmask = (~(0XFFFFFFFF<<read_cpld_reg.width))<<read_cpld_reg.field;
			}

			OPL_RegRead(read_cpld_reg.offset,&value);
			regVal = value&fieldmask;
			value = regVal>>read_cpld_reg.field;
//	        printk("value = %x \r\n",value);	
	
			if (copy_to_user(read_cpld_reg.p_data, (void *)(&value), sizeof(int)) )
	        {
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
	        }
     	    ret = 0;
   	    break;    	      

    	case CMD_REG_WRITE:  
			if (copy_from_user(&write_cpld_reg, (void *)arg, sizeof(struct write_CPLD_REG)))
			{
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
			}
			if((write_cpld_reg.field < 0) ||(write_cpld_reg.field > 31))
			{
				printk("field beyond 0~31, not allow !\n");
				goto cpld_err;      
			}
			if((write_cpld_reg.width < 1) ||(write_cpld_reg.width > 32) || ((write_cpld_reg.width + write_cpld_reg.field) > 32))
			{
				printk("width beyond 1~32, not allow !\n");
				goto cpld_err;      
			}

//			printk("write: index = %x offset = %x \r\n",write_cpld_reg.index,write_cpld_reg.offset);
//			printk("write: data = %x \r\n",write_cpld_reg.value);
			if(write_cpld_reg.width == 32)
			{
					fieldmask = 0xFFFFFFFF;
			}
			else
			{
					fieldmask = (~(0XFFFFFFFF<<write_cpld_reg.width))<<write_cpld_reg.field;
			}
			OPL_RegRead(write_cpld_reg.offset,&value);
			value = value&(~fieldmask);
			regVal = (write_cpld_reg.value<<write_cpld_reg.field)&fieldmask;
			regVal = regVal|value;
//			printk("write: actual data = %x \r\n",regVal);
 	  		OPL_RegWrite(write_cpld_reg.offset,regVal);			 
   	  	 
			ret = 0;
   	  break;    	      

		case CMD_LONGLIGHT_CFG:  
			if (copy_from_user(&longlight_cfg, (void *)arg, sizeof(struct longlight_t)))
			{
				printk("error: File: %s; line: %d\n",__FILE__,__LINE__); 
				goto cpld_err;      
			}
			switch(longlight_cfg.enable)
			{
				case 0:  /*disable irq*/
					if(longlight_irq_enable != 0)
					{
						longlight_irq_enable = 0;
						free_irq(longlight_cfg.irq_pin,longlight_interrupt);    
					}
					ret = 0;
					break;
				case 1:  /*enable irq*/
					if(longlight_irq_enable == 1)
					{
						printk("longlight irq has already enable\n");
						goto cpld_err;
					}
					printk("init longlight irq. irq=%d ctrl=%d detect=%d\n",longlight_cfg.irq_pin, 
												longlight_cfg.ctrl_pin, longlight_cfg.detect_pin);
					
					//config in/out
					OPL_RegRead(0x2c84,&val);	
					val = val&(~(1<<longlight_cfg.irq_pin));             //irq input
					val = val&(~(1<<longlight_cfg.detect_pin));          //detect input
					val = val|(1<<longlight_cfg.ctrl_pin);               //ctrl output
					OPL_RegWrite(0x2c84, val);
					
					//config irq mode
					//set terger mode 	
					OPL_RegRead(0x2c98,&val);	
					OPL_RegWrite(0x2c98, val&(~(1<<longlight_cfg.irq_pin)));
					//set nege edge 	
					OPL_RegRead(0x2ca0,&val);
					val &= ~(0x3<<(longlight_cfg.irq_pin*2));	
					OPL_RegWrite(0x2ca0, val|(0x02<<(longlight_cfg.irq_pin*2)));

					//enable light out
					OPL_RegRead(0x2c88,&val);	
					val = val&(~(1<<longlight_cfg.ctrl_pin));              
					OPL_RegWrite(0x2c88, val);
					OPL_RegRead(0x2c8c,&val);	
					val = val|((1<<longlight_cfg.ctrl_pin));              
					OPL_RegWrite(0x2c8c, val);	
					
					ret = request_irq(longlight_cfg.irq_pin, longlight_interrupt, IRQF_SHARED, "longlight_irq", longlight_interrupt);
					if(ret < 0 )
					{
						printk("register irq err ret = 0x%x\n",ret);
						return ret;
					}
					irq_enable = 1;
					break;
					
				default :
					break;
			}
		break;  
	  
	  
		default:
    	    break;
    }
    
    
	if(cmd != CMD_REG_READ && cmd != CMD_REG_WRITE)
	{
		up(&sema_cpld);
	} 
   return ret;

cpld_err:
	if(cmd != CMD_REG_READ && cmd != CMD_REG_WRITE)
	{
		up(&sema_cpld);
	} 
	return -EFAULT;
}

static int ext_localbus_mmap(struct file * file, struct vm_area_struct * vma)
{
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    /*
    * Accessing memory above the top the kernel knows about or
    * through a file pointer that was marked O_SYNC will be
    * done non-cached.
    */
    offset += CS1_PHY_BASE;
    if ((offset>__pa(high_memory)) || (file->f_flags & O_SYNC)) 
    {
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    }

    /* Don't try to swap out physical pages.. */
    vma->vm_flags |= VM_RESERVED;

    /* Don't dump addresses that are not real memory to a core file.*/
    if (offset >= __pa(high_memory) || (file->f_flags & O_SYNC))
    vma->vm_flags |= VM_IO;


    offset = offset >> PAGE_SHIFT;
    /*debug(">>>>memmap : physical(offset) 0x%x to userspace(vma->vm_start) 0x%x ~ 0x%x\n",
    offset, vma->vm_start, vma->vm_end);*/

    if (remap_pfn_range(vma, vma->vm_start, offset, vma->vm_end-vma->vm_start,
           vma->vm_page_prot))

    {
        return -EAGAIN;
    }
    
    return 0;
}

static struct file_operations ext_localbus_fops = {
	.llseek		= ext_localbus_llseek,
	.read		  = ext_localbus_read,
	.write		= ext_localbus_write,
	.poll		  = NULL,
	.ioctl		= ext_localbus_ioctl,
	.open		  = ext_localbus_open,
	.release	= ext_localbus_release,
	.mmap       = ext_localbus_mmap,
	.fasync		= NULL,
};

static int ext_localbus_init(void)
{
	int ret;
    int devno = MKDEV(OPL_LOCALBUS_MAJOR,1);
    
    printk("cpld drv ver: %s %s \n",__DATE__,__TIME__);

	init_MUTEX(&sema_cpld);
        
    cdev_init(&ext_localbus_cdev, &ext_localbus_fops);
	ext_localbus_cdev.owner = THIS_MODULE;
	  
    ret = cdev_add(&ext_localbus_cdev, devno, 1);
    if(ret)
    {      
    	  printk("cannot register ext_mem dev\n"); 
          return ret;  
    }   
    opened =0;

/****************************test for cs1**************************************************/

 /*   OPL_RegWrite(0x3008, 0x80000000); //CS1 Configure Register 0
    OPL_RegWrite(0x3044, 0x17A00);    //cs1 base addr c11
    OPL_RegWrite(0x3064, 0xFFFF);     //cs1 mask c19
//	OPL_RegWrite(0x3094, 0x1);

	void *immap;
	int i;
	immap = ioremap((CS1_PHY_BASE), CPLD_MAX_OFFSET);   
//	opl_write8((unsigned char*)immap+CPU_RESET_OK_OFFSET,0xAA);
	opl_write8((unsigned char*)immap+0x14,0x00);
printk("addr = %x : %x\n",(unsigned char*)immap+0x14,opl_read8((unsigned char*)immap+0x14));
printk("addr = %x : %x\n",(unsigned char*)immap+0x15,opl_read8((unsigned char*)immap+0x15));
	opl_write8((unsigned char*)immap+0x15,0x81);
printk("addr = %x : %x\n",(unsigned char*)immap+0x14,opl_read8((unsigned char*)immap+0x14));
printk("addr = %x : %x\n",(unsigned char*)immap+0x15,opl_read8((unsigned char*)immap+0x15));
	opl_write8((unsigned char*)immap+CPLD_INT_CLEAR_REG,0x01);
printk("addr = %x : %x\n",(unsigned char*)immap+0x14,opl_read8((unsigned char*)immap+0x14));
printk("addr = %x : %x\n",(unsigned char*)immap+0x15,opl_read8((unsigned char*)immap+0x15));
	opl_write8((unsigned char*)immap+CPLD_INT_CLEAR_REG,0x00);
//	opl_write8((unsigned char*)immap,0xaa);
//	opl_write8((unsigned char*)immap+0x08,0xAA);
//	opl_write8((unsigned char*)immap+0x09,0x0f);
//	opl_write8((unsigned char*)immap+0x0f,0xaa);
//	udelay(500000);
//	opl_write8((unsigned char*)immap+0x0f,0x00);

	for(i=0;i<0x20;i++)
	{
		printk("addr = %x : %x\n",(unsigned char*)immap+i,opl_read8((unsigned char*)immap+i));
		udelay(50000);
	}

	iounmap(immap);*/

    return 0;	
}

static void ext_localbus_free(void)
{
	cdev_del(&ext_localbus_cdev);
    return;
}

MODULE_LICENSE("GPL");
module_init(ext_localbus_init);
module_exit(ext_localbus_free);
