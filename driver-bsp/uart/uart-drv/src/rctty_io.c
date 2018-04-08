#include <linux/types.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/file.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/kd.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/smp_lock.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/cdev.h>
#include <linux/circ_buf.h>

#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <asm/io.h>
#include <asm/8xx_immap.h>
#include <asm/cpm1.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <asm/rheap.h>
#include <asm/prom.h>
#include <asm/cpm.h>
#include <asm/fs_pd.h>

#include "../inc/rctty.h"
#include "../inc/rcuart.h"
#include "../inc/rcNtty.h"
#include "../inc/rcserial_core.h"
#include "../inc/serial_xr16m752.h"
#include "../inc/serial_ucc.h"

static int pic_init(unsigned int *pic_irq );
static void pic_free(void);

#define SSR_OPEN   1
#define SSR_CLOSED 0

static struct cdev rctty_cdev;
static char tty_opened[UART_NR];

char *rcuartname[UART_NR]={"rcuart0","rcuart1","rcuart2","rcuart3"};
struct rctty_struct *pGtty_Str_xv[UART_NR];
struct rcuart_port *pxr_ports[UART_NR];


/*******************************************************************************
*描述   readbuf字符统计
*
*输入	rctty描述符，amt是否满足该字符数标准
*
*输出	
*
*返回	字符数是否达到标准
*  
*/
static inline int input_available_p(struct rctty_struct *tty, int amt)
{
	 if (tty->read_cnt >= (amt ? amt : 1))
     { return 1; }

	 return 0;
}
    
/*******************************************************************************
*描述   启动flip_buffer 刷新至readbuf 工作队列
*
*输入	rctty描述符
*
*输出	
*
*返回	
*  
*/
void tty_flip_buffer_push(struct rctty_struct *tty)
{
     schedule_delayed_work(&tty->work, 1);
}


/*******************************************************************************
*描述   切换flip buffer,最后调用刷新n_tty_receive_buf 操作
*
*输入	rctty描述符
*
*输出	
*
*返回	
*  
*/
static void flush_to_ldisc(struct work_struct *work)
{
		
	struct rctty_struct *tty = container_of(work, struct rctty_struct,  work.work);	
	
	unsigned char	*cp;
	unsigned char   *fp;
	int		        count;
	unsigned long 	flags;
		

	if (test_bit(TTY_DONT_FLIP, &tty->flags)) 
    {
		/*
		 * Do it after the next timer tick:
		 */
		schedule_delayed_work(&tty->work, 1);
		return;
	}
	spin_lock_irqsave(&tty->read_lock, flags);
	if (tty->flip.buf_num) {
		cp = tty->flip.char_buf + TTY_FLIPBUF_SIZE;
		fp = tty->flip.flag_buf + TTY_FLIPBUF_SIZE;
		tty->flip.buf_num = 0;
		tty->flip.char_buf_ptr = tty->flip.char_buf;
		tty->flip.flag_buf_ptr = tty->flip.flag_buf;
	} else {
		cp = tty->flip.char_buf;
		fp = tty->flip.flag_buf;
		tty->flip.buf_num = 1;
		tty->flip.char_buf_ptr = tty->flip.char_buf + TTY_FLIPBUF_SIZE;
		tty->flip.flag_buf_ptr = tty->flip.flag_buf + TTY_FLIPBUF_SIZE;
	}
	count = tty->flip.count;
	tty->flip.count = 0;
	spin_unlock_irqrestore(&tty->read_lock, flags);

	n_tty_receive_buf(tty, cp, fp, count);	
}


/*******************************************************************************
*描述  分配rctty描述符，初始化内部结构
*
*输入	rctty序号
*
*输出	rctty描述符
*
*返回	
*  
*/
static int init_dev(int idx, struct rctty_struct **ret_tty)
{
    struct rctty_struct *tty;
    int retval = 0;
    
	/* 
	 * Check whether we need to acquire the tty semaphore to avoid
	 * race conditions.  For now, play it safe.
	 */
	tty = kmalloc(sizeof(struct rctty_struct), GFP_KERNEL);
	if (tty)
	{	
		memset(tty, 0, sizeof(struct rctty_struct)); 
	}
	else
	{
        return -ENOMEM;
	}
    
	//initialize tty
	tty->magic = RCTTY_MAGIC;
	//init flip buffer ,other varible should be 0
	tty->flip.char_buf_ptr = tty->flip.char_buf;
	tty->flip.flag_buf_ptr = tty->flip.flag_buf;
    //init read buf
	tty->read_buf = kmalloc(N_TTY_BUF_SIZE,GFP_KERNEL);
	if (!tty->read_buf)
	{			    
    	kfree(tty);        
    	return -ENOMEM;
    }	
	memset(tty->read_buf, 0, N_TTY_BUF_SIZE);			
	   
    //init circle buf
    tty->xmit.buf = (char *) get_zeroed_page(GFP_KERNEL);
    
	if (!(tty->xmit.buf))
	{
		kfree(tty->read_buf);   
		kfree(tty);   
		return -ENOMEM;
	}
    tty->xmit.head = tty->xmit.tail = 0; 

    //
	INIT_DELAYED_WORK(&tty->work, flush_to_ldisc);
    //wait queue
  	init_waitqueue_head(&tty->write_wait);
	init_waitqueue_head(&tty->read_wait);
    //
    tty->minimum_to_wake = 1;  
    
	sema_init(&tty->atomic_read, 1);
	sema_init(&tty->atomic_write, 1);
	sema_init(&tty->tx_over, 1);
	
	spin_lock_init(&tty->read_lock);
             
	*ret_tty = tty;
	
    return retval;	

}




/*******************************************************************************
*描述  设备打开操作底层接口，实现文件与物理实体挂接
*
*输入	inode内核内部文件表示，file打开文件的描述符
*
*输出	
*
*返回	
*  
*/
static int rctty_open(struct inode * inode, struct file * filp)
{
	struct rctty_struct *tty;	
    unsigned index = iminor(inode);     //获取次设备号
    int retval = 0;	 

    if(SSR_OPEN == tty_opened[index])
    {
        printk("Err: ssruart%d is opened \n",index);
        return -EBUSY;
    }
    
    
    tty_opened[index]= SSR_OPEN;
    
    retval = init_dev(index, &tty);
    if (retval)
   {
   	  printk("Err:OOM init_dev ssruart%d \n",index);
   	  tty_opened[index] = SSR_CLOSED;
   	  
   	  return retval;
   }
      
    tty->index = index;
    tty->uartport = pxr_ports[index];
    tty->uartport->tty = tty;    
    tty->pid = task_pid(current);
              
    //printk("open ssruart%d, id=%d \n",index,tty->pid);
     
    pGtty_Str_xv[index] = tty;
    filp->private_data = tty;

    if(tty->index != tty->uartport->line)
    {
        printk("Error Match uart=tty !\n");
        printk("uart line %d \n",tty->uartport->line);
        printk("tty index %d \n",tty->index); 
        kfree(tty->read_buf);
        free_page((unsigned long) tty->xmit.buf);
        kfree(tty);
        tty_opened[index] = SSR_CLOSED;
        
        return -ENXIO;
    }
  
    retval = uart_open(tty, filp);    
    if (retval)
    {
    	printk("Err: uart_open ssruart%d \n",index);    	  
    	kfree(tty->read_buf);
        free_page((unsigned long) tty->xmit.buf);
        kfree(tty);   	     	
    	tty_opened[index] = SSR_CLOSED;
    	  
    	return retval;
    }
           
    return 0;
}


/*******************************************************************************
*描述  设备关闭操作底层接口，释放tty资源
*
*输入	inode内核内部文件表示，file打开文件的描述符
*
*输出	
*
*返回	
*  
*/

static int rctty_release(struct inode * inode, struct file * filp)
{
		
    struct rctty_struct *tty = (struct rctty_struct *)filp->private_data;
    int do_sleep;
    unsigned index = iminor(inode);

    uart_close(tty, filp);
    
    while (1) 
    {
		do_sleep = 0;
        
		if (waitqueue_active(&tty->read_wait))
        {
				wake_up(&tty->read_wait);
				do_sleep++;
		}
		if (waitqueue_active(&tty->write_wait)) 
        {
				wake_up(&tty->write_wait);
				do_sleep++;
		}
		
		
		if (!do_sleep)
			break;

		printk("release_dev: read/write wait queue active!\n");
		schedule();
	}	

    clear_bit(TTY_DONT_FLIP, &tty->flags);
    cancel_delayed_work(&tty->work);
	/*
	 * Wait for ->hangup_work and ->flip.work handlers to terminate
	 */	 
	flush_scheduled_work();

    //remove_wait_queue(&tty->write_wait, &wait);
    //remove_wait_queue(&tty->read_wait, &wait);

    //free read write xmit buffer
    kfree(tty->read_buf);
    kfree(tty->write_buf);
    free_page((unsigned long) tty->xmit.buf);

    //free tty
    kfree(tty);

    filp->private_data = NULL ;

    tty_opened[index]= SSR_CLOSED;
//    printk("close ssruart%d\n",index);
       
	return 0;
}


/*******************************************************************************
*描述   设备读操作底层接口
*
*输入	
*
*输出	
*
*返回	
*  
*/
static ssize_t rctty_read(struct file * file, char __user * buf, size_t count, loff_t *ppos)
{
	int i;
	struct rctty_struct * tty;
	
	tty = (struct rctty_struct *)file->private_data;    
	i = read_chan(tty,file,buf,count);
	    
	return i;
}


/*******************************************************************************
*描述   设备写操作底层接口
*
*输入	
*
*输出	
*
*返回	
*  
*/
static ssize_t rctty_write(struct file * file, const char __user * buf, size_t count,
			 loff_t *ppos)
{

    ssize_t ret = 0, written = 0;
    unsigned int  chunk = 2048;
    struct rctty_struct * tty = (struct rctty_struct *)file->private_data;

	if (down_interruptible(&tty->atomic_write)) {
		return -ERESTARTSYS;
	}
	
	if (count < chunk)
	{	chunk = count; }

	//当新的数据大于以前的数据
    if (tty->write_cnt < chunk) 
    {
		unsigned char *mbuf;

		if (chunk < 1024)
			chunk = 1024;  //至少1024，最多2048 4096会发送一个数据

		mbuf = kmalloc(chunk, GFP_KERNEL);
		if (!mbuf) 
        {
			up(&tty->atomic_write);
			return -ENOMEM;
		}
        //old buff
		kfree(tty->write_buf);
		tty->write_cnt = chunk; //代表写缓冲区的大小1024~4096
		tty->write_buf = mbuf;
	}

	/* Do the write .. 每次至多拷贝chunk个数据到write buf*/
	for (;;) 
    {
		size_t size = count;
		if (size > chunk)
			size = chunk;
		ret = -EFAULT;
		if (copy_from_user(tty->write_buf, buf, size))
			break;

		//printk("now write %d\n",size);
		ret = write_chan(tty, file, tty->write_buf, size);
		//printk("tty write ret == %d\n",ret);
		
		if (ret <= 0)
			break;
		written += ret;
		buf += ret;
		count -= ret;
		if (!count)
			break;
		ret = -ERESTARTSYS;
		if (signal_pending(current)) //查询是否有信号要处理
			break;
		cond_resched();
	}
	
/*	while(uart_circ_empty(&tty->xmit)!=1) //必须等待最后的数据传输完成，如果不关闭不需要等待
	{
		schedule();
		//udelay(100);
	}*/
	
	//printk("finish write %d\n",written);
	if (written) 
    {
		ret = written;
	}

    up(&tty->atomic_write);
	return ret;

}


/*******************************************************************************
*描述   设备ioctl操作底层接口
*
*输入	
*
*输出	
*
*返回	
*  
*/
int rctty_ioctl(struct inode * inode, struct file * file,
	      unsigned int cmd, unsigned long arg)
{
    struct rctty_struct *tty = (struct rctty_struct *)file->private_data;
    struct rctty_base_param_s     tty_base_param;
    struct rctty_min_param_s      tty_min_param;

    switch(cmd) 
    {
        case CMD_CONFIG_BASE_PARA:
            if (copy_from_user(&tty_base_param, (void *)arg, sizeof(struct rctty_base_param_s)))
            {
               return -EFAULT;
            }           

		
			
#ifdef DEBUG_PARA
            printk(" baud %d",tty_base_param.baud);     
            printk(" datalen %d",tty_base_param.datalen);   
            printk(" stopbit %d",tty_base_param.stopbit);  
            printk(" check %d",tty_base_param.check);    
#endif           
			tty->uartport->xr_set_termios(tty->uartport, &tty_base_param);
            break;
            
        case CMD_SET_MIN:
            if (copy_from_user(&tty_min_param, (void *)arg, sizeof(struct rctty_min_param_s)))
            {
               return -EFAULT;
            }
            
            tty->time_char = tty_min_param.min_time;
            tty->min_char = tty_min_param.min_char;
            break;
        

        case CMD_SET_BREAK:
            
            tty->uartport->xr_break_ctl(tty->uartport,arg);            
            break;
            
        case CMD_GET_STATIC:
            
           if (copy_to_user((void *)arg, &(tty->uartport->icount), sizeof(struct rcuart_icount)))
            {
               return -EFAULT;
            }            
            break;   
			  //set 485 or 232 mode    
        case CMD_SET_INTERFACE:
            if(arg)
            {
                 tty->uartport->xr_enable_485(tty->uartport); 
            }    
            else
            { 
                 tty->uartport->xr_disable_485(tty->uartport); 
            }
            break;
           
         default:
	     	   return -EINVAL;
 
    }

    return 0;
}


/*******************************************************************************
*描述   设备poll操作底层接口
*
*输入	
*
*输出	
*
*返回	
*  
*/
static unsigned int rctty_poll(struct file * filp, poll_table * wait)
{
	struct rctty_struct * tty;
	unsigned int mask = 0;
    
	tty = (struct rctty_struct *)filp->private_data;	
	
	
    //not block here just add queue
	poll_wait(filp, &tty->read_wait, wait);
	poll_wait(filp, &tty->write_wait, wait);
	if (input_available_p(tty, tty->time_char ?  0 : tty->min_char))
	{
        mask |= POLLIN | POLLRDNORM;  
    }

	if (!(mask & (POLLHUP | POLLIN | POLLRDNORM))) 
    {
		if (tty->min_char && !(tty->time_char))
			tty->minimum_to_wake = tty->min_char;
		else
			tty->minimum_to_wake = 1;
	}
          
	if (uart_circ_chars_pending(&tty->xmit) < WAKEUP_CHARS &&
			uart_circ_chars_free(&tty->xmit) > 0)
	{	mask |= POLLOUT | POLLWRNORM;      }

    return mask;
		
}

///////////////////////////////////////////////////////////////////////////////
#define ENABLE_PROC 1

#ifdef ENABLE_PROC   


const char *ssr_proc_name[UART_NR]= {"ssruart0","ssruart1","ssruart2","ssruart3"};

/*******************************************************************************
*描述   proc uart参数显示
*
*输入 标准proc内核接口
*
*输出 
*
*返回 填充字节数
*  
*/
static int uart_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{  
   struct rcuart_port *port = data;
   int ret = 0;
  
    ret += sprintf(page + ret, " tx:%d rx:%d",port->icount.tx, port->icount.rx);
   //if (port->icount.frame)
    ret += sprintf(page + ret, " fe:%d",port->icount.frame);    
   //if (port->icount.parity)
    ret += sprintf(page + ret, " pe:%d",port->icount.parity);   
   //if (port->icount.brk)
    ret += sprintf(page + ret, " brk:%d",port->icount.brk);   
   //if (port->icount.overrun)
    ret += sprintf(page + ret, " oe:%d", port->icount.overrun);  
   //
    ret += sprintf(page + ret, " rng:%d", port->icount.rng); 
   //
    ret += sprintf(page + ret, " dsr:%d", port->icount.dsr); 
   //
    ret += sprintf(page + ret, " dcd:%d", port->icount.dcd); 
   //
    ret += sprintf(page + ret, " cts:%d", port->icount.cts); 
   //
    ret += sprintf(page + ret, "\n"); 
   *eof=1;  
   return ret;
}
#endif


/************************
* 内核文件操作结构
*************************/

static struct file_operations rctty_fops = {
	.llseek   = NULL,
	.read     = rctty_read,
	.write		= rctty_write,
	.poll	    = rctty_poll,
	.ioctl		= rctty_ioctl,
	.open     = rctty_open,
	.release	= rctty_release,
	.fasync		= NULL,
};



/*******************************************************************************
*描述   内核模块初始化函数，挂接设备节点
*
*输入	
*
*输出	
*
*返回	
*  
*/
#if 0
 static struct device_node *pic_np = NULL;
#endif

static unsigned int channel = 0x0;
static unsigned int cfg_done = 0x2;

static int __init rc_tty_init(void)
{
    int ret;
    int devno = MKDEV(SERIAL_XR16_MAJOR,0);
    int i;
    unsigned int ssr_irq1=0; 
         
    printk("ssruart version:%s %s\n",__DATE__,__TIME__); 
    
    /**********************************/
/*    ret = pic_init(&ssr_irq1 );
    if(ret !=0)
    {
   	  printk("PIT Init err = 0x%x\n",ret);
   	  return -1;
    }*/

	ret = xr_probe_duart(ssr_irq1);
    if(ret)
    {
    	 return ret;
    }
	
    ret = xr_probe_ucc(ssr_irq1);
    if(ret)
    {
    	 return ret;
    }        

	register_chrdev_region(devno,UART_NR,"ssruart");
	
    cdev_init(&rctty_cdev, &rctty_fops);
    rctty_cdev.owner = THIS_MODULE;
    ret = cdev_add(&rctty_cdev, devno, UART_NR);    
    
    if(ret)
    {
        xr_unprobe_duart();
        xr_unprobe_ucc();
        return ret;  
    }

	void * itmp;
    itmp = ioremap((uint)0xe0000000, 0x1000);

	//使能CFG_DONE
	if(cfg_done == 1)
	{
		printk("open cfg_done \n");
		setbits32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc00), 0x40000000);
		setbits32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc08), 0x40000000);
	   	printk("IO1_DAT :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc00)));
    	printk("IO1_DAT :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc08)));
	}
	else if(cfg_done == 0)
	{
		printk("close cfg_done \n");
		setbits32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc00), 0x40000000);
		clrbits32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc08), 0x40000000);
	   	printk("IO1_DAT :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc00)));
    	printk("IO1_DAT :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc08)));
	}
	//使能电源
	setbits32((unsigned int __iomem *)((unsigned char*)itmp  + 0xd00), 0xf0000000);
	setbits32((unsigned int __iomem *)((unsigned char*)itmp  + 0xd08), channel<<28);
	printk("channel = %x\n",channel<<28);
	//配置管脚复用
		out_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x114), 0x00000180);  
//		out_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc08), 0xf0000000);  
   printk("sicr1 :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x114)));
   printk("sicr2 :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x118)));
	

    for(i=0;i<UART_NR;i++)
    {
        tty_opened[i] = SSR_CLOSED;
    }

	
#ifdef ENABLE_PROC    
    for(i=0;i<UART_NR;i++)
    {
       
         if (create_proc_read_entry(ssr_proc_name[i], 0, NULL, uart_read_proc, pxr_ports[i])== NULL) 
         {
            printk(KERN_WARNING"creat proc error\n");
         }
    }    
#endif

	
    return 0;
}

/*******************************************************************************
*描述   内核模块注销函数，注销设备节点
*
*输入	
*
*输出	
*
*返回	
*  
*/
static void __exit rc_tty_exit(void)
{
	int i;
//	pic_free();
    xr_unprobe_duart();
    xr_unprobe_ucc();
    cdev_del(&rctty_cdev);

  	for (i = 0; i < UART_NR; i++)
    {
         remove_proc_entry(ssr_proc_name[i], NULL);
    }

#if 0
    of_node_put(pic_np);
#endif
}



/******************
* Interrupt
* for
* PIT
******************/

#define PIT_DIV		0x2               //计数器分频率
#define PIT_COUNT	0x6000            //计数器减计数值
#define CNR_PIM     ((unsigned int)0x00000001)
#define CNR_CLIN	((unsigned int)0x00000040)
#define CNR_CLEN	((unsigned int)0x00000080)
#define EVR_PIF		((unsigned int)0x00000001)


typedef struct	sys_pit_timers 
{	
	uint cnr;
	uint ldr;
	uint psr;
	uint ctr;
	uint evr;
} pit_t;

static struct device_node *np = NULL;
static unsigned int irq1 =0;
static pit_t __iomem *sys_tmr;
static int hirq;
void * itmp;

static irqreturn_t pic_interrupt(int irq, void *dev_id)
{
  
   out_be32(&sys_tmr->ldr, PIT_COUNT);
   out_be32(&sys_tmr->evr, 0x1);  //写1清零

//   printk("start int timer\n");

   return IRQ_HANDLED;
}

#define IMAP_ADDR ((uint)0xe0000000) 

static int pic_init(unsigned int *pic_irq )
{  	
   struct of_irq oirq;
   unsigned int intspec[3];
   int res;   
      
   hirq = 11;
   
   itmp = ioremap(IMAP_ADDR, 0x1000);
   sys_tmr = (pit_t __iomem *) ( (unsigned char*)itmp  + 0x400);
   //
   out_be32(&sys_tmr->psr, PIT_DIV);  
   out_be32(&sys_tmr->ldr, PIT_COUNT);
   out_be32(&sys_tmr->cnr, CNR_CLEN | CNR_PIM );  
   out_be32(&sys_tmr->evr, 0x0);  

/*   printk("psr :%08x\n",in_be32(&sys_tmr->psr));
   printk("ldr :%08x\n",in_be32(&sys_tmr->ldr));
   printk("cnr :%08x\n",in_be32(&sys_tmr->cnr));
   printk("evr :%08x\n",in_be32(&sys_tmr->evr));
   printk("sicr1 :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x114)));
   printk("sicr2 :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x118)));
   printk("IO1_DIR :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc00)));
   printk("IO1_DRN :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc04)));
   printk("IO1_DAT :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc08)));
   
   printk("timer : %08x\n",in_be32(&sys_tmr->ctr));
   udelay(100);
   printk("timer : %08x\n",in_be32(&sys_tmr->ctr));*/

//   np = of_find_compatible_node(NULL, NULL, "fsl,pq1-pic");
   np = of_find_compatible_node(NULL, NULL, "fsl,ipic");
   intspec[0]=65;
   intspec[1]=8; //1-rise-edge 2-fall-edge 3-edge 4-high 8-low
   intspec[2]=0;
    
   res = of_irq_map_raw(np, intspec , 2, 0, &oirq);   
   if(res !=0)
   {
   	 printk("irq map res = 0x%x\n",res);
   	 return -1;
   }
   
   
	 irq1 = irq_create_of_mapping(oirq.controller, oirq.specifier, oirq.size);
	 //printk("irq pit = 0x%x\n",irq1);
	 res = request_irq(irq1, pic_interrupt, IRQF_SHARED, "pic", pic_interrupt);
	 if(res != 0)
	 {
	 	 printk("req pit irq = 0x%x\n",res);
	 }
	 

   *pic_irq = irq1;
   
   return 0; 
}

static void pic_free(void)
{
   free_irq(irq1,pic_interrupt);
   of_node_put(np);
   iounmap(itmp);
}	     
	
module_init(rc_tty_init);
module_exit(rc_tty_exit);

module_param(channel,uint,S_IRUGO);
module_param(cfg_done,uint,S_IRUGO);


MODULE_AUTHOR("www.com");
MODULE_DESCRIPTION("uart port driver for xr16m752");
MODULE_LICENSE("GPL");

	
