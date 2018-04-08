//#include <linux/config.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/circ_buf.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>

#include "../inc/rctty.h"
#include "../inc/rcuart.h"
#include "../inc/rcserial_core.h"
#include "../inc/serial_xr16m752.h"
#include "../inc/serial_ucc.h"


/*******************************************************************************
*����   uart ������� tasklet
*
*����	rctty������
*
*���	
*
*����	
*  
*/

static void uart_tasklet_action(unsigned long data)
{
	 struct rctty_struct *tty = (struct rctty_struct *)data;

      if (waitqueue_active(&tty->write_wait))
      {
           wake_up_interruptible(&tty->write_wait);
       }
}



#if defined  DEVICE_PORT_4
/*******************************************************************************
*����   uart moderm tasklet,send moderm signal
*
*����	rctty������
*
*���	
*
*����	
*  
*/

static void uart_moderm_tasklet_action(unsigned long data)
{
	 struct rctty_struct *tty = (struct rctty_struct *)data;

      kill_pid( tty->pid, SIGUSR2, 1);
      
}

#endif

/*******************************************************************************
*����   ����UART����
*
*����	rctty������
*
*���	
*
*����	
*  
*/
void uart_start_tx(struct rctty_struct *tty)
{
    tty->uartport->xr_start_tx(tty->uartport);
}


/*******************************************************************************
*����   UART�ײ�򿪲�������ʼ����ؽṹ
*
*����	rctty���������ں��ļ�������
*
*���	
*
*����	
*  
*/
int uart_open(struct rctty_struct *tty, struct file *filp)
{
    int retval = 0;

    tasklet_init(&tty->uartport->tlet, uart_tasklet_action,
				     (unsigned long)tty);

#if defined  DEVICE_PORT_4

tasklet_init(&tty->uartport->moderm_tlet, uart_moderm_tasklet_action,
				     (unsigned long)tty);

#endif
    spin_lock_init(&tty->uartport->lock);    
    retval = tty->uartport->xr_startup(tty->uartport);   

    return retval;
}

/*******************************************************************************
*����   UART�ײ�رղ������ͷ���Դ
*
*����	rctty���������ں��ļ�������
*
*���	
*
*����	
*  
*/
void uart_close(struct rctty_struct *tty, struct file *filp)
{
    tty->uartport->xr_shutdown(tty->uartport);
    tasklet_kill(&tty->uartport->tlet);
#if defined  DEVICE_PORT_4
  	tasklet_kill(&tty->uartport->moderm_tlet);
#endif

}

/*******************************************************************************
*����   UARTѭ��bufferд�롣
*
*����	rctty����������д������buffer,����д��������
*
*���	
*
*����	ʵ��д��������
*  
*/

int uart_write(struct rctty_struct *tty, const unsigned char * buf, int count)
{
	
	struct circ_buf *circ = &tty->xmit;
    unsigned long flags;
	int c, ret = 0;

	if (!circ->buf)
		return 0;

	spin_lock_irqsave(&tty->uartport->lock, flags);
	while (1) 
    {
		c = CIRC_SPACE_TO_END(circ->head, circ->tail, PAGE_SIZE); //���ص�bufĩβ���õ��ֽ���
		//printk("c = %d count= %d head=%d tail=%d\n",c,count,circ->head, circ->tail);
		if (count < c)
			c = count;
		if (c <= 0)
			break;
		memcpy(circ->buf + circ->head, buf, c);
		circ->head = (circ->head + c) & (PAGE_SIZE - 1);
		buf += c;
		count -= c;
		ret += c;
	}
	//printk("start tx\n");
    uart_start_tx(tty);
    spin_unlock_irqrestore(&tty->uartport->lock, flags);
	//printk("uart write  ret == %d\n",ret);

    return ret;
}


