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
//for reg
#define UINT32 unsigned int
#define OPL_REG_BASE          		0xBF000000
#define OPL_REG_ID2ADDR(regId)      (OPL_REG_BASE + (regId))
#define OPL_RegWrite(regId, val)   *((volatile UINT32 *)OPL_REG_ID2ADDR(regId)) = (UINT32)(val)
#define OPL_RegRead(regId, pval)   *(UINT32*)(pval) = *(volatile UINT32 *)OPL_REG_ID2ADDR(regId)



static irqreturn_t int_timer3(int irq, void *dev_id)
{        
	int val;
//	printk("timer1 irq\n");

//  OPL_RegWrite(0xb41*4, 0xff00);   //清零计数
	OPL_RegRead(0xb41*4,&val);       //中断之后需要读取，会清零timer本身状态寄存器(0xb41[6])，进而硬件自动清零timer stutas(全局0xb80)中断寄存器,不然中断返回后立即再中断
//	OPL_RegWrite(0xb41*4, 0xff85);   //使能计数

	return IRQ_HANDLED; 
}

int init_timer3()
{
	int val;

	//for timer3
  val = request_irq(27, int_timer3, IRQF_SHARED, "timer3",int_timer3);
	if (val)
	{
	    printk("Error request irq \n");       
	    return val;
	}
	OPL_RegWrite(0xb41*4, 0xff00);   //256div
	OPL_RegWrite(0xb41*4, 0xff85);   //256div
	OPL_RegWrite(0xb45*4, 244*8000);      //4000hz

	return 1;
}

static int sea_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{  
   int ret = 0;
  
    ret = sprintf(page + ret, "sea is nb");

   *eof=1;  
   return ret;
}

void sea(void)
{
	printk("sea run\n");
	while(1)
	msleep(60*1000*1000);
	return;
}

void sea_print(void)
{
	printk("sea print\n");
	return;
}

void sea_work(void)
{
	printk("sea work\n");
	return;
}

void sea_timer(void)
{
	printk("sea timer\n");
	return;
}



extern struct task_struct init_task;
struct tasklet_struct tasklet_sea;
struct workqueue_struct *wq;
DECLARE_WORK(work, sea_work);
struct timer_list timerl;

/************************************************sysfs****************************************/
static struct kobject kobj;
static struct kset ksett;
static decl_subsys(sea, NULL, NULL);

void sea_release(struct kobject *kobj)
{
	printk("sea release\n");	
}

static ssize_t sea_attr_show(struct kobject * kobj, struct attribute * attr, char * buf)
{
	return sprintf(buf, "sea is me\n");
}

static ssize_t sea_attr_store(struct kobject * kobj, struct attribute * attr,
		 const char *buf, size_t nbytes)
{
	return nbytes;
}

static struct sysfs_ops sea_sysfs_ops = {
	.show = sea_attr_show,
	.store = sea_attr_store,
};

struct attribute attr1 ={
	.name = "attr1",
	.mode =0644,
};

static struct attribute *default_attrs[] = {
	&attr1,
	NULL,
};

static struct kobj_type sea_ktype= {
	.release = sea_release,
	.sysfs_ops = &sea_sysfs_ops,
	.default_attrs = default_attrs,
};
/*******************************************************************************************/
static int task_dev_init(void)
{
	int i=0;
	printk("task dev init\n");	
#if 0
/*从头遍历task链表*/
	struct task_struct *p;
	printk("**************************************\n");
	printk("init_task address = %x\n",&init_task);
	for_each_process(p)
	{
		printk("%d : \t%d \t%s\n",i,p->pid,p->comm);
		i++;
	}

/*当前进程的相关信息*/	
	printk("current %s\n",current->comm);

	kthread_create(sea, NULL, "sea_create");   //会修改进程名，比较全面
//kernel_thread(sea, NULL, CLONE_FS|CLONE_SIGHAND);

/*中断相关*/
	init_timer3();
	
	/*使用软中断*/
	raise_softirq_irqoff(17);

	/*使用tasklet*/
	tasklet_init(&tasklet_sea, sea_print, NULL);
	tasklet_schedule(&tasklet_sea);
	
	/*使用工作队列*/
	wq =  __create_workqueue("sea_work",1,0);
	queue_work(wq, &work);
	
	/*定时器*/
	init_timer(&timerl);
	timerl.function = sea_timer;
	timerl.expires = jiffies + 5*HZ;
	add_timer(&timerl);
	
	create_proc_read_entry("sese", 0, NULL, sea_read_proc, NULL);
#endif
/**********************************************************
*设备模型
***********************************************************
*/
	/*创建sys顶层*/
	subsystem_register(&sea_subsys);

	/*创建一个kset*/
	ksett.subsys = &sea_subsys;  /*这样的话就会在sea下创建目录*/
	kobject_set_name(&ksett.kobj, "%s", "sea_set");
	kset_register(&ksett);

	/*创建kobject:初始化一个kobject对象，并加入sysfs中*/
	memset(&kobj, 0, sizeof(kobj)); 
	kobject_init(&kobj);
	kobj.kset = &ksett;       /*这样的话就会在ksett下创建目录*/
	kobj.ktype = &sea_ktype;  /*这样的话就会在kobject目录下创建文件*/
	kobject_set_name(&kobj, "%s", "sea_obj");
	kobject_add(&kobj);
	
	return 0;	
}

static void task_dev_free(void)
{
	printk("task dev free\n");
	free_irq(27,int_timer3);

  remove_proc_entry("sese", NULL);
	
	kobject_del(&kobj);
	kset_unregister(&ksett);
	subsystem_unregister(&sea_subsys);

	return;
}

MODULE_LICENSE("GPL");
module_init(task_dev_init);
module_exit(task_dev_free);
