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
static int sysfs_dev_init(void)
{
	int i=0;
	printk("sysfs dev init\n");	
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
	
	dump_stack();
	
	return 0;	
}

static void sysfs_dev_free(void)
{
	printk("sysfs dev free\n");
	
	kobject_del(&kobj);
	kset_unregister(&ksett);
	subsystem_unregister(&sea_subsys);

	return;
}

MODULE_LICENSE("GPL");
module_init(sysfs_dev_init);
module_exit(sysfs_dev_free);
