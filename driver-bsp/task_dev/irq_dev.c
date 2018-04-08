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

#include <asm/io.h>
#include <asm/delay.h>

/***********************************************软中断****************************************/
#define SEA_SOFTIRQ 17

/*软中断处理例程*/
static void sea_softirq_action(struct softirq_action *a)
{
	printk(KERN_ALERT "sea softirq run\n");
}

/*注册并触发一个软中断*/
static void init_softirq()
{
/*注册一个软中断*/
	open_softirq(SEA_SOFTIRQ, sea_softirq_action, NULL);

/*触发一个软中断*/
	raise_softirq(SEA_SOFTIRQ);
}

/***********************************************tasklet****************************************/
/*tasklet由软中断处理函数tasklet_action进行调度*/
struct tasklet_struct tasklet_sea;

void sea_print(void)
{
	printk("sea print\n");
	return;
}

/*注册并触发一个tasklet*/
static void init_tasklet()
{
	tasklet_init(&tasklet_sea, sea_print, NULL);
	tasklet_schedule(&tasklet_sea);	
}

/*******************************************************************************************/
static int irq_dev_init(void)
{
	int i=0;
	printk("irq dev init\n");	
	
	init_softirq();
	init_tasklet();
	
	return 0;
}

static void irq_dev_free(void)
{
	printk("irq dev free\n");


	return;
}

MODULE_LICENSE("GPL");
module_init(irq_dev_init);
module_exit(irq_dev_free);
