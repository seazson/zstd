/*
 * Freescale QUICC Engine UART device driver
 *
 * Author: Timur Tabi <timur@freescale.com>
 *
 * Copyright 2007 Freescale Semiconductor, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * This driver adds support for UART devices via Freescale's QUICC Engine
 * found on some Freescale SOCs.
 *
 * If Soft-UART support is needed but not already present, then this driver
 * will request and upload the "Soft-UART" microcode upon probe.  The
 * filename of the microcode should be fsl_qe_ucode_uart_X_YZ.bin, where "X"
 * is the name of the SOC (e.g. 8323), and YZ is the revision of the SOC,
 * (e.g. "11" for 1.1).
 */

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/circ_buf.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/irq.h>


#include <linux/of_platform.h>

#include "../inc/rctty.h"
#include "../inc/rcuart.h"
#include "../inc/rctty_flip.h"
#include "../inc/serial_ucc.h"
/*
 * The GUMR flag for Soft UART.  This would normally be defined in qe.h,
 * but Soft-UART is a hack and we want to keep everything related to it in
 * this file.
 */
#define UCC_SLOW_GUMR_H_SUART   	0x00004000      /* Soft-UART */

/*
 * soft_uart is 1 if we need to use Soft-UART mode
 */
static int soft_uart;
/* Enable this macro to configure all serial ports in internal loopback
   mode */
//#define LOOPBACK 

extern char *rcuartname[UART_NR];
extern struct rcuart_port *pxr_ports[UART_NR];
static int qe_uart_request_port(struct rcuart_port *port);
static void qe_uart_release_port(struct rcuart_port *port);

//#define DEBUG_UCC_UART
#ifdef DEBUG_UCC_UART
#define DEBUG_UCC(fmt, args...) printk(KERN_ALERT "ucc" fmt, ##args)
#else
#define DEBUG_UCC(fmt, args...)
#endif

static int ucc_uart_probe(struct of_device *ofdev,
	const struct of_device_id *match)
{
    struct rcuart_port *uap;
	struct device_node *np = ofdev->node;
	const unsigned int *iprop;      /* Integer OF properties */
	const char *sprop;      /* String OF properties */
	struct uart_qe_port *qe_port = NULL;
	struct resource res;
	int ret;

	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

	qe_port = kzalloc(sizeof(struct uart_qe_port), GFP_KERNEL);
	if (!qe_port) {
		printk( "can't allocate QE port structure\n");
		return -ENOMEM;
	}

	/* Search for IRQ and mapbase 获取reg字段值*/
	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		printk( "missing 'reg' property in device tree\n");
		kfree(qe_port);
		return ret;
	}
	if (!res.start) {
		printk( "invalid 'reg' property in device tree\n");
		kfree(qe_port);
		return -EINVAL;
	}
	qe_port->port.mapbase = res.start;
	DEBUG_UCC("port.mapbase %x\n",qe_port->port.mapbase);

	/* Get the UCC number (device ID) cell-index指定ucc编号 UCCs are numbered (1~8)-1 */
	iprop = of_get_property(np, "cell-index", NULL);
	if (!iprop) {
		iprop = of_get_property(np, "device-id", NULL);
		if (!iprop) {
			kfree(qe_port);
			printk( "UCC is unspecified in "
				"device tree\n");
			return -EINVAL;
		}
	}

	if ((*iprop < 1) || (*iprop > UCC_MAX_NUM)) {
		printk( "no support for UCC%u\n", *iprop);
		kfree(qe_port);
		return -ENODEV;
	}
	qe_port->ucc_num = *iprop - 1;
	DEBUG_UCC("qe_port->ucc_num %x\n",qe_port->ucc_num);

	/*
	 * In the future, we should not require the BRG to be specified in the
	 * device tree.  If no clock-source is specified, then just pick a BRG
	 * to use.  This requires a new QE library function that manages BRG
	 * assignments.
	 */

	sprop = of_get_property(np, "rx-clock-name", NULL);
	if (!sprop) {
		printk( "missing rx-clock-name in device tree\n");
		kfree(qe_port);
		return -ENODEV;
	}
	qe_port->us_info.rx_clock = qe_clock_source(sprop); //获取clk对应的枚举变量值，UART必须使用BRG
	if ((qe_port->us_info.rx_clock < QE_BRG1) ||
	    (qe_port->us_info.rx_clock > QE_BRG16)) {
		printk( "rx-clock-name must be a BRG for UART\n");
		kfree(qe_port);
		return -ENODEV;
	}

	sprop = of_get_property(np, "tx-clock-name", NULL);
	if (!sprop) {
		printk( "missing tx-clock-name in device tree\n");
		kfree(qe_port);
		return -ENODEV;
	}
	qe_port->us_info.tx_clock = qe_clock_source(sprop);
	if ((qe_port->us_info.tx_clock < QE_BRG1) ||
	    (qe_port->us_info.tx_clock > QE_BRG16)) {
		printk( "tx-clock-name must be a BRG for UART\n");
		kfree(qe_port);
		return -ENODEV;
	}
	DEBUG_UCC("qe_port->us_info.tx_clock %x\n",qe_port->us_info.tx_clock);
	DEBUG_UCC("qe_port->us_info.rx_clock %x\n",qe_port->us_info.rx_clock);

	/* Get the port number给串口编号, numbered 0-3 */
	iprop = of_get_property(np, "port-number", NULL);
	if (!iprop) {
		printk( "missing port-number in device tree\n");
		kfree(qe_port);
		return -EINVAL;
	}
	qe_port->port.line = *iprop;
	DEBUG_UCC("qe_port->port.line %x\n",qe_port->port.line);

	if (qe_port->port.line >= UCC_MAX_UART) {
		printk( "port-number must be 0-%u\n",
			UCC_MAX_UART - 1);
		kfree(qe_port);
		return -EINVAL;
	}

	qe_port->port.irq = irq_of_parse_and_map(np, 0);
	DEBUG_UCC("qe_port->port.irq %x\n",qe_port->port.irq);
	if (qe_port->port.irq == NO_IRQ) {
		printk( "could not map IRQ for UCC%u\n",
		       qe_port->ucc_num + 1);
		kfree(qe_port);
		return -EINVAL;
	}

	/*
	 * Newer device trees have an "fsl,qe" compatible property for the QE
	 * node, but we still need to support older device trees.
	 */
	np = of_find_compatible_node(NULL, NULL, "fsl,qe");
	if (!np) {
		np = of_find_node_by_type(NULL, "qe");
		if (!np) {
			printk( "could not find 'qe' node\n");
			kfree(qe_port);
			return -EINVAL;
		}
	}

	iprop = of_get_property(np, "brg-frequency", NULL);
	if (!iprop) {
		printk(
		       "missing brg-frequency in device tree\n");
		kfree(qe_port);
		return -EINVAL;
	}

	if (*iprop)
		qe_port->port.uartclk = *iprop;
	else {
		/*
		 * Older versions of U-Boot do not initialize the brg-frequency
		 * property, so in this case we assume the BRG frequency is
		 * half the QE bus frequency.
		 */
		iprop = of_get_property(np, "bus-frequency", NULL);
		if (!iprop) {
			printk(
				"missing QE bus-frequency in device tree\n");
			kfree(qe_port);
			return -EINVAL;
		}
		if (*iprop)
			qe_port->port.uartclk = *iprop / 2;  //brg时钟通常是qe的一半
		else {
			printk(
				"invalid QE bus-frequency in device tree\n");
			kfree(qe_port);
			return -EINVAL;
		}
	}
//	qe_port->port.uartclk = 22118400*2*4/2;
	DEBUG_UCC("qe_port->port.uartclk %d\n",qe_port->port.uartclk);
	
	spin_lock_init(&qe_port->port.lock);
	qe_port->np = np;
	qe_port->port.dev = &ofdev->dev;
	qe_port->port.fifosize = 512;
//	qe_port->port.iotype = UPIO_MEM;
//	qe_port->port.flags = UPF_BOOT_AUTOCONF | UPF_IOREMAP;

	qe_port->tx_nrfifos = TX_NUM_FIFO;
	qe_port->tx_fifosize = TX_BUF_SIZE;
	qe_port->rx_nrfifos = RX_NUM_FIFO;
	qe_port->rx_fifosize = RX_BUF_SIZE;

	qe_port->wait_closing = UCC_WAIT_CLOSING;

	qe_port->us_info.ucc_num = qe_port->ucc_num;
	qe_port->us_info.regs = (phys_addr_t) res.start;
	qe_port->us_info.irq = qe_port->port.irq;

	qe_port->us_info.rx_bd_ring_len = qe_port->rx_nrfifos;
	qe_port->us_info.tx_bd_ring_len = qe_port->tx_nrfifos;

	/* Make sure ucc_slow_init() initializes both TX and RX 使用命令初始化cmd*/
	qe_port->us_info.init_tx = 1;
	qe_port->us_info.init_rx = 1;

	//关联全局变量
	uap = &qe_port->port;
	uap->qe_port = qe_port;

	uap->uart_type      = 1;   //for ucc
	uap->xr_break_ctl   = xr_break_ctl_ucc; 
	uap->xr_set_termios = xr_set_termios_ucc;
	uap->xr_shutdown    = xr_shutdown_ucc;
	uap->xr_startup     = xr_startup_ucc;
	uap->xr_start_tx    = xr_start_tx_ucc;
	uap->xr_enable_485  = xr_enable_485_ucc;
	uap->xr_disable_485  = xr_disable_485_ucc;
//	uap->xr_stop_tx     = xr_stop_tx_ucc;

	qe_uart_request_port(uap);

	
	if(match->compatible[8] == '1' )
	{
		DEBUG_UCC("ucc_uart1 probe\n");
		pxr_ports[0] = uap;
	}
	else if(match->compatible[8] == '2' )
	{
		DEBUG_UCC("ucc_uart2 probe\n");
		pxr_ports[1] = uap;
	}
	else if(match->compatible[8] == '3' )
	{
		DEBUG_UCC("ucc_uart3 probe\n");
		pxr_ports[2] = uap;
	}

	return 0;
}

static int ucc_uart_remove(struct of_device *ofdev)
{
	struct uart_qe_port *qe_port = dev_get_drvdata(&ofdev->dev);

	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

	dev_set_drvdata(&ofdev->dev, NULL);
	kfree(qe_port);
	
	return 0;
}

static struct of_device_id ucc_uart_match[3] = {
	{
		.type = "serial",
		.compatible = "ucc_uart1",
	},
	{
		.type = "serial",
		.compatible = "ucc_uart2",
	},
	{
		.type = "serial",
		.compatible = "ucc_uart3",
	},
};

static struct of_platform_driver ucc_uart_of_driver = {
	.owner  	    = THIS_MODULE,
	.name   	    = "ucc_uart",
	.match_table    = &ucc_uart_match[0],
	.probe  	    = ucc_uart_probe,
	.remove 	    = ucc_uart_remove,
};

struct rctty_base_param_s def_params = {
    .baud = 9600,         /* 波特率 9600 19200…… */
    .datalen = 8,      /* 字长 5 6 7 8*/
    .check = 0,        /* 校验 0- NO 1-odd 2-even 3-mark 4-space*/
    .stopbit = 0,      /* 停止位 1-1,2-2 */           

};

/*
 * Virtual to physical address translation.
 *
 * Given the virtual address for a character buffer, this function returns
 * the physical (DMA) equivalent.
 */
static inline dma_addr_t cpu2qe_addr(void *addr, struct uart_qe_port *qe_port)
{
	if (likely((addr >= qe_port->bd_virt)) &&
	    (addr < (qe_port->bd_virt + qe_port->bd_size)))
		return qe_port->bd_dma_addr + (addr - qe_port->bd_virt);

	/* something nasty happened */
	printk(KERN_ERR "%s: addr=%p\n", __func__, addr);
	BUG();
	return 0;
}

/*
 * Physical to virtual address translation.
 *
 * Given the physical (DMA) address for a character buffer, this function
 * returns the virtual equivalent.
 */
static inline void *qe2cpu_addr(dma_addr_t addr, struct uart_qe_port *qe_port)
{
	/* sanity check */
	if (likely((addr >= qe_port->bd_dma_addr) &&
		   (addr < (qe_port->bd_dma_addr + qe_port->bd_size))))
		return qe_port->bd_virt + (addr - qe_port->bd_dma_addr);

	/* something nasty happened */
	printk(KERN_ERR "%s: addr=%x\n", __func__, addr);
	BUG();
	return NULL;
}

/*
 * Return 1 if the QE is done transmitting all buffers for this port
 *
 * This function scans each BD in sequence.  If we find a BD that is not
 * ready (READY=1), then we return 0 indicating that the QE is still sending
 * data.  If we reach the last BD (WRAP=1), then we know we've scanned
 * the entire list, and all BDs are done.
 */

#define pl32(y) DEBUG_UCC(#y ": %x\n",in_be32(&(y)))
#define pl16(y) DEBUG_UCC(#y ": %x\n",in_be16(&(y)))
#define pl8(y) DEBUG_UCC(#y ": %x\n",in_8(&(y)))

static unsigned int qe_uart_tx_empty(struct rcuart_port *port)
{

	struct uart_qe_port *qe_port = port->qe_port;
	struct qe_bd *bdp = qe_port->tx_bd_base;

	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

	while (1) {
//		DEBUG_UCC("bdp->status %x : [%x]\n",bdp,in_be16(&bdp->status));
//		DEBUG_UCC("bdp->len    %x : [%x]\n",bdp,in_be16(&bdp->length));
//		pl16(qe_port->uccp->ucce);
//		pl16(qe_port->uccp->uccm);
//		pl16(qe_port->uccp->uccs);
//		pl32(qe_port->uccp->gumr_l);
//		pl32(qe_port->uccp->gumr_h);
//		pl16(qe_port->uccp->upsmr);
//		udelay(100000);
		if (in_be16(&bdp->status) & BD_SC_READY)
		{
			/* This BD is not done, so return "not done" */
			return 0;
		}
		
		if (in_be16(&bdp->status) & BD_SC_WRAP)
		{	/*
			 * This BD is done and it's the last one, so return
			 * "done"
			 */
			return 1;
		}
		bdp++;
	};
}

/*
 * Set the modem control lines
 *
 * Although the QE can control the modem control lines (e.g. CTS), we
 * don't need that support. This function must exist, however, otherwise
 * the kernel will panic.
 */
void qe_uart_set_mctrl(struct rcuart_port *port, unsigned int mctrl)
{
}

/*
 * Get the current modem control line status
 *
 * Although the QE can control the modem control lines (e.g. CTS), this
 * driver currently doesn't support that, so we always return Carrier
 * Detect, Data Set Ready, and Clear To Send.
 *
static unsigned int qe_uart_get_mctrl(struct rcuart_port *port)
{
	printk("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
}*/

/*
 * Disable the transmit interrupt.
 *
 * Although this function is called "stop_tx", it does not actually stop
 * transmission of data.  Instead, it tells the QE to not generate an
 * interrupt when the UCC is finished sending characters.
 */
static void qe_uart_stop_tx(struct rcuart_port *port)
{
	struct uart_qe_port *qe_port = port->qe_port;

	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	clrbits16(&qe_port->uccp->uccm, UCC_UART_UCCE_TX);
}

/*
 * Transmit as many characters to the HW as possible.
 *
 * This function will attempt to stuff of all the characters from the
 * kernel's transmit buffer into TX BDs.
 *
 * A return value of non-zero indicates that it successfully stuffed all
 * characters from the kernel buffer.
 *
 * A return value of zero indicates that there are still characters in the
 * kernel's buffer that have not been transmitted, but there are no more BDs
 * available.  This function should be called again after a BD has been made
 * available.
 */
static int qe_uart_tx_pump(struct rcuart_port *port)
{
	struct qe_bd *bdp;
	unsigned char *p;
	unsigned int count;
	struct uart_qe_port *qe_port = port->qe_port;
	struct circ_buf *xmit = &(port->tty->xmit);

	if (uart_circ_empty(xmit)) {
		qe_uart_stop_tx(port);
		return 0;
	}

	/* Pick next descriptor and fill from buffer */
	bdp = qe_port->tx_cur;

	while (!(in_be16(&bdp->status) & BD_SC_READY) &&
	       (xmit->tail != xmit->head)) {
		count = 0;
		p = qe2cpu_addr(bdp->buf, qe_port);
		while (count < qe_port->tx_fifosize) {
			*p++ = xmit->buf[xmit->tail];
			DEBUG_UCC("send %c [%x] \n",*(p-1),*(p-1));
			xmit->tail = (xmit->tail + 1) & (TTY_FLIPBUF_SIZE - 1);
			port->icount.tx++;
			count++;
			if (xmit->head == xmit->tail)
				break;
		}

		out_be16(&bdp->length, count);
		setbits16(&bdp->status, BD_SC_READY);

		/* Get next BD. */
		if (in_be16(&bdp->status) & BD_SC_WRAP)
			bdp = qe_port->tx_bd_base;
		else
			bdp++;
	}
	qe_port->tx_cur = bdp;

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
	{
		tasklet_schedule(&port->tlet);
	}
	
	if (uart_circ_empty(xmit)) {
		/* The kernel buffer is empty, so turn off TX interrupts.  We
		   don't need to be told when the QE is finished transmitting
		   the data. */
		DEBUG_UCC("in pump circ is empty\n");
		qe_uart_stop_tx(port);
		return 0;
	}

	return 1;
}

/*
 * Start transmitting data
 *
 * This function will start transmitting any available data, if the port
 * isn't already transmitting data.
 */
void xr_start_tx_ucc(struct rcuart_port *port)
{

 	struct uart_qe_port *qe_port = port->qe_port;

	/* If we currently are transmitting, then just return */
	if (in_be16(&qe_port->uccp->uccm) & UCC_UART_UCCE_TX)
		return;

	/* Otherwise, pump the port and start transmission */
	if (qe_uart_tx_pump(port))
		setbits16(&qe_port->uccp->uccm, UCC_UART_UCCE_TX);
}

/*
 * Stop transmitting data
 */
static void qe_uart_stop_rx(struct rcuart_port *port)
{
	struct uart_qe_port *qe_port =
		container_of(port, struct uart_qe_port, port);

	clrbits16(&qe_port->uccp->uccm, UCC_UART_UCCE_RX);
}

/* Start or stop sending  break signal
 *
 * This function controls the sending of a break signal.  If break_state=1,
 * then we start sending a break signal.  If break_state=0, then we stop
 * sending the break signal.
 */
void xr_break_ctl_ucc(struct rcuart_port *port, int break_state)
{
	struct uart_qe_port *qe_port = port->qe_port;

	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	if (break_state)
		ucc_slow_stop_tx(qe_port->us_private);
	else
		ucc_slow_restart_tx(qe_port->us_private);
}

/* ISR helper function for receiving character.
 *
 * This function is called by the ISR to handling receiving characters
 */
static void qe_uart_int_rx(struct uart_qe_port *qe_port)
{
	int i;
	unsigned char ch, *cp;
	struct rcuart_port *port = &qe_port->port;
	struct rctty_struct *tty = port->tty;
	struct qe_bd *bdp;
	u16 status;
	unsigned int flg;

	/* Just loop through the closed BDs and copy the characters into
	 * the buffer.
	 */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	bdp = qe_port->rx_cur;
	while (1) {
		status = in_be16(&bdp->status);

		/* If this one is empty, then we assume we've read them all */
		if (status & BD_SC_EMPTY)
			break;

		/* get number of characters, and check space in RX buffer */
		i = in_be16(&bdp->length);

		/* If we don't have enough room in RX buffer for the entire BD,
		 * then we try later, which will be the next RX interrupt.
		 */
		if (unlikely(tty->flip.count >= TTY_FLIPBUF_SIZE - i)) 
		{
			tty_flip_buffer_push(tty);  
			return;
		}

		/* get pointer */
		cp = qe2cpu_addr(bdp->buf, qe_port);

		/* loop through the buffer */
		while (i-- > 0) {
			ch = *cp++;
			DEBUG_UCC("recv %c [%x] \n",*(cp-1),*(cp-1));
			port->icount.rx++;
			flg = TTY_NORMAL;

			if (!i && status &
			    (BD_SC_BR | BD_SC_FR | BD_SC_PR | BD_SC_OV))
				goto handle_error;

error_return:
			tty_insert_flip_char(tty, ch, flg);

		}

		/* This BD is ready to be used again. Clear status. get next */
		clrsetbits_be16(&bdp->status, BD_SC_BR | BD_SC_FR | BD_SC_PR |
			BD_SC_OV | BD_SC_ID, BD_SC_EMPTY);
		if (in_be16(&bdp->status) & BD_SC_WRAP)
			bdp = qe_port->rx_bd_base;
		else
			bdp++;

	}

	/* Write back buffer pointer */
	qe_port->rx_cur = bdp;

	/* Activate BH processing */
	tty_flip_buffer_push(tty);

	return;

	/* Error processing */

handle_error:
	/* Statistics */
	if (status & BD_SC_BR)
		port->icount.brk++;
	if (status & BD_SC_PR)
		port->icount.parity++;
	if (status & BD_SC_FR)
		port->icount.frame++;
	if (status & BD_SC_OV)
		port->icount.overrun++;

	/* Handle the remaining ones */
	if (status & BD_SC_BR)
		flg = TTY_BREAK;
	else if (status & BD_SC_PR)
		flg = TTY_PARITY;
	else if (status & BD_SC_FR)
		flg = TTY_FRAME;

	/* Overrun does not affect the current character ! */
	if (status & BD_SC_OV)
		tty_insert_flip_char(tty, 0, TTY_OVERRUN);

	goto error_return;
}

/* Interrupt handler
 *
 * This interrupt handler is called after a BD is processed.
 */
static irqreturn_t qe_uart_int(int irq, void *data)
{
	struct uart_qe_port *qe_port = (struct uart_qe_port *) data;
	struct ucc_slow __iomem *uccp = qe_port->uccp;
	u16 events;

	/* Clear the interrupts */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	events = in_be16(&uccp->ucce);
//	pl16(qe_port->uccp->ucce);
	out_be16(&uccp->ucce, events);
//	pl16(qe_port->uccp->ucce);

/*	if (events & UCC_UART_UCCE_BRKE)
	{
		DEBUG_UCC("int break\n");
		uart_handle_break(&qe_port->port);
	}*/
	if (events & UCC_UART_UCCE_RX)
	{
		DEBUG_UCC("rx interapt\n");
		qe_uart_int_rx(qe_port);
	}
	if (events & UCC_UART_UCCE_TX)
	{
		DEBUG_UCC("tx interpart\n");
		qe_uart_tx_pump(&(qe_port->port));
	}
	return events ? IRQ_HANDLED : IRQ_NONE;
}

/* Initialize buffer descriptors
 *
 * This function initializes all of the RX and TX buffer descriptors.
 */
static void qe_uart_initbd(struct uart_qe_port *qe_port)
{
	int i;
	void *bd_virt;
	struct qe_bd *bdp;

	/* Set the physical address of the host memory buffers in the buffer
	 * descriptors, and the virtual address for us to work with.
	 */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	bd_virt = qe_port->bd_virt;
	bdp = qe_port->rx_bd_base;
	qe_port->rx_cur = qe_port->rx_bd_base;
	for (i = 0; i < (qe_port->rx_nrfifos - 1); i++) {
		out_be16(&bdp->status, BD_SC_EMPTY | BD_SC_INTRPT);
		out_be32(&bdp->buf, cpu2qe_addr(bd_virt, qe_port));
		out_be16(&bdp->length, 0);
		bd_virt += qe_port->rx_fifosize;
		bdp++;
	}

	/* */
	out_be16(&bdp->status, BD_SC_WRAP | BD_SC_EMPTY | BD_SC_INTRPT);
	out_be32(&bdp->buf, cpu2qe_addr(bd_virt, qe_port));
	out_be16(&bdp->length, 0);

	/* Set the physical address of the host memory
	 * buffers in the buffer descriptors, and the
	 * virtual address for us to work with.
	 */
	bd_virt = qe_port->bd_virt +
		L1_CACHE_ALIGN(qe_port->rx_nrfifos * qe_port->rx_fifosize);
	qe_port->tx_cur = qe_port->tx_bd_base;
	bdp = qe_port->tx_bd_base;
	for (i = 0; i < (qe_port->tx_nrfifos - 1); i++) {
		out_be16(&bdp->status, BD_SC_INTRPT);
		out_be32(&bdp->buf, cpu2qe_addr(bd_virt, qe_port));
		out_be16(&bdp->length, 0);
		bd_virt += qe_port->tx_fifosize;
		bdp++;
	}

	/* Loopback requires the preamble bit to be set on the first TX BD */
#ifdef LOOPBACK
	setbits16(&qe_port->tx_cur->status, BD_SC_P);
#endif

	out_be16(&bdp->status, BD_SC_WRAP | BD_SC_INTRPT);
	out_be32(&bdp->buf, cpu2qe_addr(bd_virt, qe_port));
	out_be16(&bdp->length, 0);
}

/*
 * Initialize a UCC for UART.
 *
 * This function configures a given UCC to be used as a UART device. Basic
 * UCC initialization is handled in qe_uart_request_port().  This function
 * does all the UART-specific stuff.
 */
static void qe_uart_init_ucc(struct uart_qe_port *qe_port)
{
	u32 cecr_subblock;
	struct ucc_slow __iomem *uccp = qe_port->uccp;
	struct ucc_uart_pram *uccup = qe_port->uccup;

	unsigned int i;

	/* First, disable TX and RX in the UCC */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	ucc_slow_disable(qe_port->us_private, COMM_DIR_RX_AND_TX);

	/* Program the UCC UART parameter RAM */
	out_8(&uccup->common.rbmr, UCC_BMR_GBL | UCC_BMR_BO_BE);
	out_8(&uccup->common.tbmr, UCC_BMR_GBL | UCC_BMR_BO_BE);
	out_be16(&uccup->common.mrblr, qe_port->rx_fifosize);
	out_be16(&uccup->maxidl, 0x10);
	out_be16(&uccup->brkcr, 1);
	out_be16(&uccup->parec, 0);
	out_be16(&uccup->frmec, 0);
	out_be16(&uccup->nosec, 0);
	out_be16(&uccup->brkec, 0);
	out_be16(&uccup->uaddr[0], 0);
	out_be16(&uccup->uaddr[1], 0);
	out_be16(&uccup->toseq, 0);
	for (i = 0; i < 8; i++)
		out_be16(&uccup->cchars[i], 0xC000);
	out_be16(&uccup->rccm, 0xc0ff);

	/* Configure the GUMR registers for UART */
	if (soft_uart) {
		/* Soft-UART requires a 1X multiplier for TX */
		clrsetbits_be32(&uccp->gumr_l,
			UCC_SLOW_GUMR_L_MODE_MASK | UCC_SLOW_GUMR_L_TDCR_MASK |
			UCC_SLOW_GUMR_L_RDCR_MASK,
			UCC_SLOW_GUMR_L_MODE_UART | UCC_SLOW_GUMR_L_TDCR_1 |
			UCC_SLOW_GUMR_L_RDCR_16);

		clrsetbits_be32(&uccp->gumr_h, UCC_SLOW_GUMR_H_RFW,
			UCC_SLOW_GUMR_H_TRX | UCC_SLOW_GUMR_H_TTX);
	} else {
		clrsetbits_be32(&uccp->gumr_l,
			UCC_SLOW_GUMR_L_MODE_MASK | UCC_SLOW_GUMR_L_TDCR_MASK |
			UCC_SLOW_GUMR_L_RDCR_MASK,
			UCC_SLOW_GUMR_L_MODE_UART | UCC_SLOW_GUMR_L_TDCR_16 |
			UCC_SLOW_GUMR_L_RDCR_16);

		clrsetbits_be32(&uccp->gumr_h,
			UCC_SLOW_GUMR_H_TRX | UCC_SLOW_GUMR_H_TTX,
			UCC_SLOW_GUMR_H_RFW);
	}

#ifdef LOOPBACK
	DEBUG_UCC("gumr_l is %x\n",in_be32(&uccp->gumr_l));
	clrsetbits_be32(&uccp->gumr_l, UCC_SLOW_GUMR_L_DIAG_MASK,
		UCC_SLOW_GUMR_L_DIAG_LOOP);
	clrsetbits_be32(&uccp->gumr_h,
		UCC_SLOW_GUMR_H_CTSP | UCC_SLOW_GUMR_H_RSYN,
		UCC_SLOW_GUMR_H_CDS);
	DEBUG_UCC("gumr_l is %x\n",in_be32(&uccp->gumr_l));
#endif

	/* Disable rx interrupts  and clear all pending events.  */
	out_be16(&uccp->uccm, 0);
	out_be16(&uccp->ucce, 0xffff);
	out_be16(&uccp->udsr, 0x7e7e);

	/* Initialize UPSMR */
	out_be16(&uccp->upsmr, 0);

	if (soft_uart) {
		out_be16(&uccup->supsmr, 0x30);
		out_be16(&uccup->res92, 0);
		out_be32(&uccup->rx_state, 0);
		out_be32(&uccup->rx_cnt, 0);
		out_8(&uccup->rx_bitmark, 0);
		out_8(&uccup->rx_length, 10);
		out_be32(&uccup->dump_ptr, 0x4000);
		out_8(&uccup->rx_temp_dlst_qe, 0);
		out_be32(&uccup->rx_frame_rem, 0);
		out_8(&uccup->rx_frame_rem_size, 0);
		/* Soft-UART requires TX to be 1X */
		out_8(&uccup->tx_mode,
			UCC_UART_TX_STATE_UART | UCC_UART_TX_STATE_X1);
		out_be16(&uccup->tx_state, 0);
		out_8(&uccup->resD4, 0);
		out_be16(&uccup->resD5, 0);

		/* Set UART mode.
		 * Enable receive and transmit.
		 */

		/* From the microcode errata:
		 * 1.GUMR_L register, set mode=0010 (QMC).
		 * 2.Set GUMR_H[17] bit. (UART/AHDLC mode).
		 * 3.Set GUMR_H[19:20] (Transparent mode)
		 * 4.Clear GUMR_H[26] (RFW)
		 * ...
		 * 6.Receiver must use 16x over sampling
		 */
		clrsetbits_be32(&uccp->gumr_l,
			UCC_SLOW_GUMR_L_MODE_MASK | UCC_SLOW_GUMR_L_TDCR_MASK |
			UCC_SLOW_GUMR_L_RDCR_MASK,
			UCC_SLOW_GUMR_L_MODE_QMC | UCC_SLOW_GUMR_L_TDCR_16 |
			UCC_SLOW_GUMR_L_RDCR_16);

		clrsetbits_be32(&uccp->gumr_h,
			UCC_SLOW_GUMR_H_RFW | UCC_SLOW_GUMR_H_RSYN,
			UCC_SLOW_GUMR_H_SUART | UCC_SLOW_GUMR_H_TRX |
			UCC_SLOW_GUMR_H_TTX | UCC_SLOW_GUMR_H_TFL);

#ifdef LOOPBACK
		clrsetbits_be32(&uccp->gumr_l, UCC_SLOW_GUMR_L_DIAG_MASK,
				UCC_SLOW_GUMR_L_DIAG_LOOP);
		clrbits32(&uccp->gumr_h, UCC_SLOW_GUMR_H_CTSP |
			  UCC_SLOW_GUMR_H_CDS);
#endif

		cecr_subblock = ucc_slow_get_qe_cr_subblock(qe_port->ucc_num);
		qe_issue_cmd(QE_INIT_TX_RX, cecr_subblock,
			QE_CR_PROTOCOL_UNSPECIFIED, 0);
	} else {
		cecr_subblock = ucc_slow_get_qe_cr_subblock(qe_port->ucc_num);
		qe_issue_cmd(QE_INIT_TX_RX, cecr_subblock,
			QE_CR_PROTOCOL_UART, 0);
	}
}
/*
 * Allocate any memory and I/O resources required by the port.
 */
extern int xr_startup_ucc_per(struct rcuart_port *port);
static int qe_uart_request_port(struct rcuart_port *port)
{
	int ret;
	struct uart_qe_port *qe_port = port->qe_port;
	struct ucc_slow_info *us_info = &qe_port->us_info;
	struct ucc_slow_private *uccs;
	unsigned int rx_size, tx_size;
	void *bd_virt;
	dma_addr_t bd_dma_addr = 0;

	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

	ret = ucc_slow_init(us_info, &uccs);
	if (ret) {
		printk( "could not initialize UCC%u\n",
		       qe_port->ucc_num);
		return ret;
	}

	qe_port->us_private = uccs;
	qe_port->uccp = uccs->us_regs;
	qe_port->uccup = (struct ucc_uart_pram *) uccs->us_pram;
	qe_port->rx_bd_base = uccs->rx_bd;
	qe_port->tx_bd_base = uccs->tx_bd;

	/*
	 * Allocate the transmit and receive data buffers.
	 */

	rx_size = L1_CACHE_ALIGN(qe_port->rx_nrfifos * qe_port->rx_fifosize);
	tx_size = L1_CACHE_ALIGN(qe_port->tx_nrfifos * qe_port->tx_fifosize);

	bd_virt = dma_alloc_coherent(qe_port->port.dev, rx_size + tx_size, &bd_dma_addr,
		GFP_KERNEL);
	if (!bd_virt) {
		printk( "could not allocate buffer descriptors\n");
		return -ENOMEM;
	}

	qe_port->bd_virt = bd_virt;
	qe_port->bd_dma_addr = bd_dma_addr;
	qe_port->bd_size = rx_size + tx_size;

	qe_port->rx_buf = bd_virt;
	qe_port->tx_buf = qe_port->rx_buf + rx_size;

	xr_startup_ucc_per(port);

	return 0;
}

/*
 * Initialize the port.
 */
int xr_startup_ucc_per(struct rcuart_port *port)
{
	struct uart_qe_port *qe_port = port->qe_port;
	int ret;

	/*
	 * If we're using Soft-UART mode, then we need to make sure the
	 * firmware has been uploaded first.
	 */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

	qe_uart_initbd(qe_port);
	qe_uart_init_ucc(qe_port);

	/* Install interrupt handler. */
/*	ret = request_irq(qe_port->port.irq, qe_uart_int, IRQF_SHARED, "ucc-uart",
		qe_port);
	if (ret) {
		printk( "could not claim IRQ %u\n", qe_port->port.irq);
		return ret;
	}*/

	/* Startup rx-int */
//	setbits16(&qe_port->uccp->uccm, UCC_UART_UCCE_RX);
	ucc_slow_enable(qe_port->us_private, COMM_DIR_RX_AND_TX);//在485模式下必须先初始化guml，不然每次打开时都会产生干扰电平

//	xr_set_termios_ucc(port,&def_params);

	return 0;
}

int xr_startup_ucc(struct rcuart_port *port)
{
	struct uart_qe_port *qe_port = port->qe_port;
	int ret;

	/*
	 * If we're using Soft-UART mode, then we need to make sure the
	 * firmware has been uploaded first.
	 */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	/* Install interrupt handler. */
	ret = request_irq(qe_port->port.irq, qe_uart_int, IRQF_SHARED, "ucc-uart",
		qe_port);
	if (ret) {
		printk( "could not claim IRQ %u\n", qe_port->port.irq);
		return ret;
	}

	/* Startup rx-int */
	setbits16(&qe_port->uccp->uccm, UCC_UART_UCCE_RX);
	xr_set_termios_ucc(port,&def_params);

	return 0;
}

/*
 * Release any memory and I/O resources that were allocated in
 * qe_uart_request_port().
 **/
static void qe_uart_release_port(struct rcuart_port *port)
{
	struct uart_qe_port *qe_port = port->qe_port;
	struct ucc_slow_private *uccs = qe_port->us_private;

	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

	ucc_slow_disable(qe_port->us_private, COMM_DIR_RX_AND_TX);
	ucc_slow_graceful_stop_tx(qe_port->us_private);

	dma_free_coherent(port->dev, qe_port->bd_size, qe_port->bd_virt,
			  qe_port->bd_dma_addr);

	ucc_slow_free(uccs);
}

/*
 * Shutdown the port.
 */
void  xr_shutdown_ucc(struct rcuart_port *port)
{
	struct uart_qe_port *qe_port = port->qe_port;
	struct ucc_slow __iomem *uccp = qe_port->uccp;
	unsigned int timeout = 20;

	/* Disable RX and TX */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

	/* Wait for all the BDs marked sent */
	while (!qe_uart_tx_empty(port)) {
		if (!--timeout) {
			printk( "shutdown timeout\n");
			break;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(2);
	}
	udelay(100000); //这里必须加入延时，bd虽然表示发送完成，但是发送fifo还没有
	/* Stop uarts */
	//ucc_slow_disable(qe_port->us_private, COMM_DIR_RX_AND_TX);
	clrbits16(&uccp->uccm, UCC_UART_UCCE_TX | UCC_UART_UCCE_RX);

	/* Shut them really down and reinit buffer descriptors */
	//ucc_slow_graceful_stop_tx(qe_port->us_private);
	//qe_uart_initbd(qe_port);

	free_irq(qe_port->port.irq, qe_port);

}

/*
 * Set the serial port parameters.
 */

void xr_set_termios_ucc(struct rcuart_port *port, struct rctty_base_param_s * ptty_base_param)
{
	struct uart_qe_port *qe_port = port->qe_port;
	struct ucc_slow __iomem *uccp = qe_port->uccp;
	unsigned int baud = ptty_base_param->baud;
	u16 upsmr = 0;
	u8 char_length = 2; /* 1 + CL + PEN + 1 + SL */

	/* Character length programmed into the mode register is the
	 * sum of: 1 start bit, number of data bits, 0 or 1 parity bit,
	 * 1 or 2 stop bits, minus 1.
	 * The value 'bits' counts this for us.
	 */

	/* byte size */
	DEBUG_UCC("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);

     //length
     switch(ptty_base_param->datalen)
     {
        case 5: 
			upsmr |= UCC_UART_UPSMR_CL_5;
			char_length += 5;
            break;
        case 6: 
			upsmr |= UCC_UART_UPSMR_CL_6;
			char_length += 6;
            break;
        case 7: 
			upsmr |= UCC_UART_UPSMR_CL_7;
			char_length += 7;
            break;
        case 8: 
        default:
			upsmr |= UCC_UART_UPSMR_CL_8;
			char_length += 8;
            break;
      }
	 //stop bits
     switch(ptty_base_param->stopbit)
     {
        case 1:
          break;
          
        case 2:
			upsmr |= UCC_UART_UPSMR_SL;
			char_length++;  /* + SL */
            break;

        default:
            break;
      }   
     //check
     switch(ptty_base_param->check)
     {
        case 0: //NO parity
            
            break;
        case 1: //odd check 
	  			upsmr |= UCC_UART_UPSMR_PEN;
	  			char_length++;  /* + PEN */
             break;
        case 2: //even
				upsmr |= UCC_UART_UPSMR_PEN;
				char_length++;  /* + PEN */
	 			upsmr &= ~(UCC_UART_UPSMR_RPM_MASK |
					   UCC_UART_UPSMR_TPM_MASK);
				upsmr |= UCC_UART_UPSMR_RPM_EVEN |
					UCC_UART_UPSMR_TPM_EVEN;
            break;
        case 3: //mark
				upsmr |= UCC_UART_UPSMR_PEN;
				char_length++;  /* + PEN */
	 			upsmr &= ~(UCC_UART_UPSMR_RPM_MASK |
					   UCC_UART_UPSMR_TPM_MASK);
				upsmr |= UCC_UART_UPSMR_RPM_HIGH|
					UCC_UART_UPSMR_TPM_HIGH;
             break;
        case 4: //space
				upsmr |= UCC_UART_UPSMR_PEN;
				char_length++;  /* + PEN */
	 			upsmr &= ~(UCC_UART_UPSMR_RPM_MASK |
					   UCC_UART_UPSMR_TPM_MASK);
				upsmr |= UCC_UART_UPSMR_RPM_LOW|
					UCC_UART_UPSMR_TPM_LOW;
             break;
             
        default:
            break;        
      }
	out_be16(&uccp->upsmr, upsmr);
	qe_setbrg(qe_port->us_info.rx_clock, baud, 16);  //这里设置brg的分频率，并且使能brg
	qe_setbrg(qe_port->us_info.tx_clock, baud, 16);
}
 


/*
 * Verify that the data in serial_struct is suitable for this device.
 *
static int qe_uart_verify_port(struct rcuart_port *port,
			       struct serial_struct *ser)
{
	printk("%s [%s] : %d\n",__FILE__,__FUNCTION__,__LINE__);
	if (ser->type != PORT_UNKNOWN && ser->type != PORT_CPM)
		return -EINVAL;

	if (ser->irq < 0 || ser->irq >= nr_irqs)
		return -EINVAL;

	if (ser->baud_base < 9600)
		return -EINVAL;

	return 0;
}*/
int xr_probe_ucc(int irq)
{
//	struct rcuart_port *uap;
	int ret = 0;
/*
	for (i = 0; i < 3; i++)
	{

		uap = kmalloc(sizeof(struct rcuart_port), GFP_KERNEL);
		if (uap == NULL)
		{
			ret = -ENOMEM;
			printk(KERN_WARNING"error uap Init \n");
			goto out;
		}  

		uap->qe_port.port = uap;
    	pxr_ports[i] = uap;
		

#ifdef ENABLE_PROC_1    

		if (create_proc_read_entry(ssr_proc_name[i], 0, NULL, uart_read_proc, uap)== NULL) 
		{
			printk(KERN_WARNING"creat proc error\n");
		}

#endif

}*/
     
	ret = of_register_platform_driver(&ucc_uart_of_driver);
	if (ret)
		printk(KERN_ERR "ucc-uart: could not register platform driver\n");

/*
    printk("sicr1 :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x114)));
    printk("sicr2 :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x118)));
    printk("rcwlr :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x900)));
    printk("rcwhr :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0x904)));
	printk("IO1_DIR :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc00)));
	printk("IO1_DRN :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc04)));
	printk("IO1_DAT :%08x\n",in_be32((unsigned int __iomem *)((unsigned char*)itmp  + 0xc08)));
*/

  return ret;    
}

void xr_unprobe_ucc(void)
{
    int i = 0;
    struct rcuart_port *uap;

    for (i = 0; i < 3; i++)
    {
		qe_uart_release_port(pxr_ports[i]);
    }
 
 	of_unregister_platform_driver(&ucc_uart_of_driver);

    for (i = 0; i < 3; i++)
    {
         uap = pxr_ports[i];
         pxr_ports[i] = NULL;
         //remove_proc_entry(ssr_proc_name[i], NULL);
         kfree(uap);
    }

}

void xr_enable_485_ucc(struct rcuart_port *uap)
{    
    return;
}



/*******************************************************************************
*描述 关闭auto485
*
*输入 uart端口描述
*
*输出 无
*
*返回   
*/

void xr_disable_485_ucc(struct rcuart_port *uap)
{
 
    return;
}

