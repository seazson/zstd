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
#include <linux/proc_fs.h>

#define DBGREG printf
#define UINT32 unsigned int
/*OPL REG*/
#define OPL_REG_BASE          		0xBF000000
#define OPL_REG_ID2ADDR(regId)      (OPL_REG_BASE + (regId))

#define OPL_RegWrite(regId, val)   *((volatile UINT32 *)OPL_REG_ID2ADDR(regId)) = (UINT32)(val)
#define OPL_RegRead(regId, pval)   *(UINT32*)(pval) = *(volatile UINT32 *)OPL_REG_ID2ADDR(regId)

#define OPL_RegWrite_DIRECT(regId, val)   *((volatile UINT32 *)(regId)) = (UINT32)(val)
#define OPL_RegRead_DIRECT(regId, pval)   *(UINT32*)(pval) = *(volatile UINT32 *)(regId)

#define opl_read8(addr)           (*(volatile unsigned char *)(addr))
#define opl_write8(addr,value)   (*(volatile unsigned char *)(addr) = value)
#define opl_read16(addr)           (*(volatile unsigned short *)(addr))   /*如果是8位bus 会发起四次读*/
#define opl_write16(addr,value)   (*(volatile unsigned short *)(addr) = value)
#define opl_read32(addr)           (*(volatile unsigned int *)(addr))   /*如果是8位bus 会发起四次读*/
#define opl_write32(addr,value)   (*(volatile unsigned int *)(addr) = value)

static unsigned int longlight_ctrl_pin = 11;
static unsigned int longlight_detect_pin = 5;
static unsigned int longlight_en = 1;
static unsigned int longlight_time = 5;
static unsigned int longlight_time2 = 5;
static unsigned int longlight_confirm=0;
struct timer_list  timer1;
struct timer_list  timer2;

void longlight_irq_enable(unsigned int irq)
{
	unsigned int val;
	OPL_RegRead(0x2e08,&val);	
	val = val|(1<<irq);              
	OPL_RegWrite(0x2e08, val);
}

void longlight_irq_disable(unsigned int irq)
{
	unsigned int val;
	OPL_RegRead(0x2e08,&val);	
	val = val&(~(1<<irq));              
	OPL_RegWrite(0x2e08, val);
}

static irqreturn_t longlight_interrupt(int irq, void *dev_id)
{
	longlight_irq_disable(longlight_detect_pin);
//	printk("irq detect\n");
	longlight_confirm = 0;
   	return IRQ_HANDLED;
}

void timer1_tick(unsigned long data)
{
	int val,ret,val2;

	if(longlight_en == 0)
		return;
		
	OPL_RegRead(0x2c80,&val);	
	OPL_RegRead(0x2c84,&val2);	
//	printk("timer for longlight input=%x direct=%x\n",val,val2);
	
	if(val & (1<<longlight_detect_pin))
	{
//		printk("detect high pin\n");
		longlight_confirm = 1;
		longlight_irq_disable(longlight_detect_pin);

		timer2.expires = jiffies + (HZ * longlight_time2);
		add_timer(&timer2);
		longlight_irq_enable(longlight_detect_pin);
	}
	else
	{
		timer1.expires = jiffies + (HZ * longlight_time);
		add_timer(&timer1);
	}
	
}

void timer2_tick(unsigned long data)
{
	unsigned int val;
//	printk("timer2 tick\n");
	if(longlight_confirm == 0)
	{
		timer1.expires = jiffies + (HZ * longlight_time);
		add_timer(&timer1);
	}
	else if(longlight_confirm < 3)
	{
		printk("detect longlight\n");
		longlight_confirm++;
		timer2.expires = jiffies + (HZ * longlight_time2);
		add_timer(&timer2);
	}
	else
	{
		printk("detect longlight, trun off light!!!\n");
//disable light out	
		OPL_RegRead(0x2c88,&val);	
		val = val|(1<<longlight_ctrl_pin);              
		OPL_RegWrite(0x2c88, val);
		OPL_RegRead(0x2c8c,&val);	
		val = val&(~(1<<longlight_ctrl_pin));              
		OPL_RegWrite(0x2c8c, val);
		timer1.expires = jiffies + (HZ * longlight_time);
		add_timer(&timer1);
	}
}

/************************************proc********************************************/
static int longlighten_read_fun(char *page, char **start, off_t off, int count, int *eof, void *data)
{  
   int ret = 0;
  
    ret = sprintf(page + ret, "enable = %d\n", longlight_en);

   *eof=1;  
   return ret;
}

static int longlighten_write_fun(struct file *file, const char *buffer, unsigned long count, void *data)
{
	static char rcmem_buf[64];
	char *p = rcmem_buf;
	
	if(count > 64)
	{
		printk("arg too long %ld > 64\n",count);
		return -EFAULT;
	}
	
	if(copy_from_user(rcmem_buf,buffer,count))
		return -EFAULT;

	longlight_en = simple_strtol(p,NULL,0);

	if(longlight_en == 1)
	{
		timer1.expires = jiffies + (HZ * longlight_time);
		add_timer(&timer1);	
	}
	
	return count;	
}

static int longlighttime_read_fun(char *page, char **start, off_t off, int count, int *eof, void *data)
{  
   int ret = 0;
  
    ret = sprintf(page + ret, "timer = %d\n", longlight_time);

   *eof=1;  
   return ret;
}

static int longlighttime_write_fun(struct file *file, const char *buffer, unsigned long count, void *data)
{
	static char rcmem_buf[64];
	char *p = rcmem_buf;
	
	if(count > 64)
	{
		printk("arg too long %ld > 64\n",count);
		return -EFAULT;
	}
	
	if(copy_from_user(rcmem_buf,buffer,count))
		return -EFAULT;

	longlight_time = simple_strtol(p,NULL,0);

 	return count;	
}


static void longlight_create_proc_entry(void)
{
	struct proc_dir_entry *entry_en = NULL;
	struct proc_dir_entry *entry_time = NULL;

	entry_en = create_proc_entry("longlighten", S_IFREG | S_IRUGO | S_IWUSR, NULL);
	if(!entry_en)
	{
		printk("err create proc/dir\n");		
	}
	else
	{
		entry_en->read_proc  = longlighten_read_fun;
		entry_en->write_proc = longlighten_write_fun;
	}

	entry_time = create_proc_entry("longlighttime", S_IFREG | S_IRUGO | S_IWUSR, NULL);
	if(!entry_time)
	{
		printk("err create proc/dir\n");		
	}
	else
	{
		entry_time->read_proc  = longlighttime_read_fun;
		entry_time->write_proc = longlighttime_write_fun;
	}
	
}

static int ext_longlight_init(void)
{
	unsigned int   val,ret;
    printk("long light detect drv ver: %s %s \n",__DATE__,__TIME__);
	longlight_create_proc_entry();

	//config in/out
	OPL_RegRead(0x2c84,&val);	
	val = val&(~(1<<longlight_detect_pin));          //detect input
	val = val|(1<<longlight_ctrl_pin);               //ctrl output
	OPL_RegWrite(0x2c84, val);

	//config irq mode
	//set terger mode 	
	OPL_RegRead(0x2c98,&val);	
	OPL_RegWrite(0x2c98, val&(~(1<<longlight_detect_pin)));
	//set nege edge 	
	OPL_RegRead(0x2ca0,&val);
	val &= ~(0x3<<(longlight_detect_pin*2));	
	OPL_RegWrite(0x2ca0, val|(0x02<<(longlight_detect_pin*2)));

	//enable light out,low enable
	OPL_RegRead(0x2c88,&val);	
	val = val&(~(1<<longlight_ctrl_pin));              
	OPL_RegWrite(0x2c88, val);
	OPL_RegRead(0x2c8c,&val);	
	val = val|((1<<longlight_ctrl_pin));              
	OPL_RegWrite(0x2c8c, val);		

	ret = request_irq(longlight_detect_pin, longlight_interrupt, IRQF_SHARED, "longlight_irq", longlight_interrupt);
	if(ret < 0 )
	{
		printk("register irq err ret = 0x%x\n",ret);
		return ret;
	}
	longlight_irq_disable(longlight_detect_pin);
		
	init_timer (&timer1);
    timer1.function = timer1_tick;
    timer1.data= (unsigned long)1;
    timer1.expires = jiffies + (HZ * longlight_time);
    add_timer(&timer1);

	init_timer (&timer2);
    timer2.function = timer2_tick;
    timer2.data= (unsigned long)2;
	
    return 0;	
}

static void ext_longlight_free(void)
{
	del_timer(&timer1);
	del_timer(&timer2);
	free_irq(longlight_detect_pin,longlight_interrupt); 
	remove_proc_entry("longlighten",NULL);
	remove_proc_entry("longlighttime",NULL);
    return;
}

MODULE_LICENSE("GPL");
module_init(ext_longlight_init);
module_exit(ext_longlight_free);
