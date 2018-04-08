#ifndef _UART_H
#define _UART_H

//#include "../inc/serial_ucc.h"

#define uart_circ_empty(circ)   ((circ)->head == (circ)->tail)
#define uart_circ_clear(circ)   ((circ)->head = (circ)->tail = 0)

#define uart_circ_chars_pending(circ) \
  (CIRC_CNT((circ)->head, (circ)->tail, PAGE_SIZE)) //返回buf里面的数值head-tail

#define uart_circ_chars_free(circ)  \
  (CIRC_SPACE((circ)->head, (circ)->tail, PAGE_SIZE))



typedef enum ssr_port_status
{
    RECEIVE=0,
    TRANSMIT,
    STOP
}ssr_port_status_e;


typedef enum phy_type
{
    RS232 = 0,
    RS485    
        
}phy_type_e;

struct rcuart_port 
{
    struct uart_qe_port          *qe_port;
    unsigned char       __iomem  *membase;    /* read/write[bwl] */
    unsigned int                  irq;        /* irq number */
    struct rcuart_icount          icount;     /* statistics */
    unsigned int    			  flags;
    unsigned int                  line;       /* port index */
    unsigned long                 mapbase;    /* for ioremap */
    struct rctty_struct          *tty;  
    spinlock_t                    lock; 
	struct tasklet_struct         tlet;
	unsigned char                 im;         /* interrupt mask*/
      
#if defined  DEVICE_PORT_4
       struct tasklet_struct	moderm_tlet;
       unsigned char hw_stopped;
       //
        phy_type_e ssr_phy_type;
       //for 232 only means interrupt is enable for REC?
       //for 485 means current work status
       ssr_port_status_e ssr_port_status;
#endif
	unsigned int		uartclk;		/* base uart clock */
	unsigned int		fifosize;		/* tx fifo size */
	unsigned char		iotype;			/* io access style */
	struct device		*dev;			/* parent device */
	unsigned int        uart_type;
	
	void (*xr_stop_tx)(struct rcuart_port *uap);
	void (*xr_start_tx)(struct rcuart_port *uap);
	void (*xr_break_ctl)(struct rcuart_port *uap, int break_state);
	int (*xr_startup)(struct rcuart_port *uap);
	void (*xr_shutdown)(struct rcuart_port *uap);
	void (*xr_enable_485)(struct rcuart_port *port);
	void (*xr_disable_485)(struct rcuart_port *port);
	void (*xr_set_termios)(struct rcuart_port *port, struct rctty_base_param_s * ptty_base_param);
	int (*xr_probe)(int irq);
	void (*xr_unprobe)(void);

};

#endif

