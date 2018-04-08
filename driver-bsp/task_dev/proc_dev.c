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

struct proc_dir_entry *entry_dir = NULL;
int val=10;
extern struct proc_dir_entry proc_root;

/*首先实现基本的读写函数*/
static int sea_read_fun(char *page, char **start, off_t off, int count, int *eof, void *data)
{  
   int ret = 0;
  
    ret = sprintf(page + ret, "val = %d\n", val);

   *eof=1;  
   return ret;
}

static int sea_write_fun(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;
	char p[100];
	data = p;
	
	if(copy_from_user(data,buffer,count))
		return -EFAULT;
	
	val = *p-'0'+1;
	
	return count;	
}

/*创建目录*/
static void sea_create_proc_dir()
{
	entry_dir = create_proc_entry("sea", S_IFDIR | S_IRUGO | S_IXUGO, &proc_root);
	
	if(!entry_dir)
	{
		printk("err create proc/dir\n");		
	}
	else
	{
		
	}
}

/*创建文件*/
static void sea_create_proc_entry()
{
	struct proc_dir_entry *entry = NULL;
	entry = create_proc_entry("seafile", S_IFREG | S_IRUGO | S_IWUSR, entry_dir);
	
	if(!entry)
	{
		printk("err create proc/dir\n");		
	}
	else
	{
		entry->read_proc  = sea_read_fun;
		entry->write_proc = sea_write_fun;
	}
}










/*******************************************************************************************/
static int proc_dev_init(void)
{
	int i=0;
	printk("proc dev init\n");	

/*核心函数*/
	sea_create_proc_dir();
	sea_create_proc_entry();

/*包装函数*/
	proc_mkdir("sea1",NULL);
	proc_symlink("sea2",NULL,"proc/sea");
	create_proc_read_entry("seafile2", 0, NULL, sea_read_fun, NULL);
	
	return 0;
}

static void proc_dev_free(void)
{
	printk("proc dev free\n");

  remove_proc_entry("seafile", entry_dir);
  remove_proc_entry("sea", &proc_root);

  remove_proc_entry("sea1", NULL);
  remove_proc_entry("sea2", NULL);
  remove_proc_entry("seafile2", NULL);

	return;
}

MODULE_LICENSE("GPL");
module_init(proc_dev_init);
module_exit(proc_dev_free);
