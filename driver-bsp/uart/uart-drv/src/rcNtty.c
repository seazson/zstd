//#include <linux/config.h>
#include <linux/types.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/bitops.h>


#include <linux/interrupt.h>
#include <linux/circ_buf.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/system.h>

#include "../inc/rctty.h"
#include "../inc/rcuart.h"
#include "../inc/rcNtty.h"
#include "../inc/rcserial_core.h"
#include "../inc/serial_xr16m752.h"
#include "../inc/serial_ucc.h"






/*******************************************************************************
*描述   readbuf拷贝到用户buffer
*
*输入	rctty描述符，用户buffer,期望拷贝字节数
*
*输出	
*
*返回	内核标准错误
*  
*/
static inline int copy_from_read_buf(struct rctty_struct *tty,
				      unsigned char __user **b,
				      size_t *nr)
{
	int retval;
	size_t n;
	unsigned long flags;

	retval = 0;
	spin_lock_irqsave(&tty->read_lock, flags);
	n = min(tty->read_cnt, N_TTY_BUF_SIZE - tty->read_tail);
	n = min(*nr, n);
	spin_unlock_irqrestore(&tty->read_lock, flags);
	if (n) {
		mb();
		retval = copy_to_user(*b, &tty->read_buf[tty->read_tail], n);
 		n -= retval;
		spin_lock_irqsave(&tty->read_lock, flags);
		tty->read_tail = (tty->read_tail + n) & (N_TTY_BUF_SIZE-1);
		tty->read_cnt -= n;
		spin_unlock_irqrestore(&tty->read_lock, flags);
		*b += n;
		*nr -= n;
	}
	return retval;
}


/*******************************************************************************
*描述   读数据，可能睡眠
*
*输入	rctty描述符，内核文件描述符，用户buffer,期望拷贝字节数
*
*输出	
*
*返回	内核标准错误
*  
*/
ssize_t read_chan(struct rctty_struct *tty, struct file *file,
			 unsigned char __user *buf, size_t nr)
{
	unsigned char __user *b = buf;
	DECLARE_WAITQUEUE(wait, current);
	int minimum, time;
	ssize_t retval = 0;
	ssize_t size;
	long timeout;	
    int uncopied;

	if (!tty->read_buf) 
    {
		printk("ERROR read_buf == NULL!\n");
		return -EIO;
	}
	
	//minimum = time = 0;
	timeout = ((long)(~0UL>>1)); //MAX_SCHEDULE_TIMEOUT;	
	time = (HZ / 10) * (tty->time_char);
	minimum = tty->min_char;

    //最小字符读取限制
    if (minimum)
    {
        //有规定时间
		if (time)
		{	tty->minimum_to_wake = 1; }
		else if (!waitqueue_active(&tty->read_wait) || (tty->minimum_to_wake > minimum))
		{	tty->minimum_to_wake = minimum; }
	} 
    else
	{
		timeout = 0;
		if (time) 
        {
				timeout = time;
				time = 0;
		}
		tty->minimum_to_wake = minimum = 1;//单字符返回
	}	

	/*
	 *	Internal serialization of reads.
	 */
	if (file->f_flags & O_NONBLOCK)
    {
		if (down_trylock(&tty->atomic_read))
			return -EAGAIN;
	}
	else 
    {
		if (down_interruptible(&tty->atomic_read))
			return -ERESTARTSYS;
	}

	add_wait_queue(&tty->read_wait, &wait);
	set_bit(TTY_DONT_FLIP, &tty->flags);
    
	while (nr)
    {
		
		/* This statement must be first before checking for input
		   so that any interrupt will set the state back to
		   TASK_RUNNING. */
		set_current_state(TASK_INTERRUPTIBLE);
		
		if (((minimum - (b - buf)) < tty->minimum_to_wake) &&
		    ((minimum - (b - buf)) >= 1))
			tty->minimum_to_wake = (minimum - (b - buf));
		
		//if (!input_available_p(tty, 0)) 
		 if( !(tty->read_cnt >= 1) )
        {
			if (!timeout)//No data but do not want wait
				break;
			if (file->f_flags & O_NONBLOCK) 
            {
				retval = -EAGAIN;
				break;
			}
			if (signal_pending(current)) 
            {
				retval = -ERESTARTSYS;
				break;
			}
			clear_bit(TTY_DONT_FLIP, &tty->flags);
			timeout = schedule_timeout(timeout);
			set_bit(TTY_DONT_FLIP, &tty->flags);
			continue;
		}
        
		__set_current_state(TASK_RUNNING);

	          
		   uncopied = copy_from_read_buf(tty, &b, &nr);
		   uncopied += copy_from_read_buf(tty, &b, &nr);
           if (uncopied)
           {
				retval = -EFAULT;
				break;
		   }
		
       //check minim 满足
		if (b - buf >= minimum)
			break;
		if (time)
			timeout = time;
	}
    
	clear_bit(TTY_DONT_FLIP, &tty->flags);
	up(&tty->atomic_read);
	remove_wait_queue(&tty->read_wait, &wait);

	if (!waitqueue_active(&tty->read_wait))
		tty->minimum_to_wake = minimum;

	__set_current_state(TASK_RUNNING);
    
	size = b - buf;
	if (size) 
    {
		retval = size;	   
	} 
 
	return retval;
}



/*******************************************************************************
*描述   写数据，可能睡眠
*
*输入	rctty描述符，内核文件描述符，用户buffer,期望写入字节数
*
*输出	
*
*返回	内核标准错误
*  
*/
ssize_t write_chan(struct rctty_struct * tty, struct file * file,
			  const unsigned char * buf, size_t nr)
{
	const unsigned char *b = buf;
	DECLARE_WAITQUEUE(wait, current);
	int c;
	ssize_t retval = 0;

	add_wait_queue(&tty->write_wait, &wait);
	while (1) 
    {
		set_current_state(TASK_INTERRUPTIBLE);
		if (signal_pending(current)) 
        {
			retval = -ERESTARTSYS;
			break;
		}
		//down(&tty->tx_over);

		c = uart_write(tty, b, nr);
		//printk("write chan c == %d\n",c);

		if (c < 0) 
        {
			retval = c;
			goto break_out;
		}
		b += c;
		nr -= c;
		
		if (!nr)
		{
            break;
        }
		if (file->f_flags & O_NONBLOCK) 
        {
			retval = -EAGAIN;
			break;
		}
		schedule();
	}

break_out:
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&tty->write_wait, &wait);
	return (b - buf) ? b - buf : retval;
}





/*******************************************************************************
 *	n_tty_receive_buf	-	data receive
 *	@tty: terminal device
 *	@cp: buffer
 *	@fp: flag buffer
 *	@count: characters
 *
 *	Called by the terminal driver when a block of characters has
 *	been received. This function must be called from soft contexts
 *	not from interrupt context. The driver is responsible for making
 *	calls one at a time and in order (or using flush_to_ldisc)
 */
 //异步更新_readBuf
void n_tty_receive_buf(struct rctty_struct *tty, const unsigned char *cp,
			      unsigned char *fp, int count)
{
	
	int	i;
	unsigned long cpuflags;

	if (!tty->read_buf)
	{
        return;   
    }

	    //copy form real raw
		spin_lock_irqsave(&tty->read_lock, cpuflags);
		i = min(N_TTY_BUF_SIZE - tty->read_cnt,
			N_TTY_BUF_SIZE - tty->read_head);
		i = min(count, i);
		memcpy(tty->read_buf + tty->read_head, cp, i);
		tty->read_head = (tty->read_head + i) & (N_TTY_BUF_SIZE-1);
		tty->read_cnt += i;
		cp += i;
		count -= i;

		i = min(N_TTY_BUF_SIZE - tty->read_cnt,
			N_TTY_BUF_SIZE - tty->read_head);
		i = min(count, i);
		memcpy(tty->read_buf + tty->read_head, cp, i);
		tty->read_head = (tty->read_head + i) & (N_TTY_BUF_SIZE-1);
		tty->read_cnt += i;
		spin_unlock_irqrestore(&tty->read_lock, cpuflags);
	 
    //读取字符数满足上限
	if  (tty->read_cnt >= tty->minimum_to_wake)
    {
        //异步信号读写通知
		//kill_fasync(&tty->fasync, SIGIO, POLL_IN);
		if (waitqueue_active(&tty->read_wait))
		{
            wake_up_interruptible(&tty->read_wait);
		}
	}

	     
}






