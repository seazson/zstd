#ifndef _TTY_H
#define _TTY_H
#include "../inc/ssr_device.h"

/*
 * This is the flip buffer used for the tty driver.  The buffer is
 * located in the tty structure, and is used as a high speed interface
 * between the tty driver and the tty line discipline.
 */

#define  TTY_FLIPBUF_SIZE  4096
#define  N_TTY_BUF_SIZE    16384
#define  WAKEUP_CHARS      256
#define  UART_NR   		   4
#define  SERIAL_XR16_MAJOR 158

/* Defer buffer flip */
#define TTY_DONT_FLIP 		8	
/* rctty magic number */
#define RCTTY_MAGIC		    0x5808

struct tty_flip_buffer 
{
	unsigned char		   *char_buf_ptr;
	unsigned char	       *flag_buf_ptr;
	int	                 	count;
	int	                 	buf_num;//which buf 0 or 1
	unsigned char	        char_buf[2*TTY_FLIPBUF_SIZE];
	unsigned char	        flag_buf[2*TTY_FLIPBUF_SIZE];
	unsigned char	        slop[4]; /* N.B. bug overwrites buffer by 1 */
};

struct rctty_struct 
{
	int	magic;
	int index;

    unsigned char time_char;
	unsigned char min_char;
       
#if defined  DEVICE_PORT_4
    unsigned char enable_hw_flow_ctl;    
    unsigned char enable_hw_flow_sig;
#endif
	unsigned long flags;
    //who opened current ssrtty
	struct pid *pid;
    
  struct delayed_work		work;//flush_to_ldisc
	struct tty_flip_buffer flip;

	wait_queue_head_t write_wait;
	wait_queue_head_t read_wait;

    //current wake up level
  unsigned short minimum_to_wake;
	
	
	char *read_buf;
	int read_head;
	int read_tail;
	int read_cnt;

	struct semaphore atomic_read;
	struct semaphore atomic_write;
    struct semaphore tx_over;
	
	unsigned char *write_buf;
	int write_cnt;

	spinlock_t read_lock; 

    struct rcuart_port *uartport; 
    struct circ_buf	xmit;

};

void tty_flip_buffer_push(struct rctty_struct *tty);

#endif

