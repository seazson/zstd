#ifndef _SERIAL_XR16
#define _SERIAL_XR16

#define UART_RX 0x00             /*URBR*/
#define UART_TX 0x00			 /*UTHR*/


#define UART_DLL 0x00			 /*UDLB*/
#define UART_DLM 0x01			 /*UDMB*/

#define UART_DLD 0x02			 /*n */
#define UART_DLD_AUTO485    0x40 /*n */

#define UART_EFR 0x02			 /*n */
#define UART_EFR_ENABLESB   0x10 /*n */

#define UART_IER 0x01			 /*UIER 中断控制器，用于使能中断*/	
#define UART_IER_MSI		0x08 /* Enable Modem status interrupt */
#define UART_IER_RLSI		0x04 /* Enable receiver line status interrupt */
#define UART_IER_THRI		0x02 /* Enable Transmitter holding register int. */
#define UART_IER_RDI		0x01 /* Enable receiver data interrupt */




#define UART_IIR 0x02            /* UIIR in:  Interrupt ID Register */
#define UART_IIR_NO_INT		0x01 /* No interrupts pending */
#define UART_IIR_ID		    0x06 /* Mask for the interrupt ID */
#define UART_IIR_MSI		0x00 /* Modem status interrupt */
#define UART_IIR_THRI		0x02 /* Transmitter holding register empty */
#define UART_IIR_RDI		0x04 /* Receiver data interrupt */
#define UART_IIR_RLSI		0x06 /* Receiver line status interrupt */

#define UART_FCR 0x02				/*UFCR*/
#define UART_FCR_ENABLE_FIFO	0x01 /* Enable the FIFO */
#define UART_FCR_CLEAR_RCVR  	0x02 /* Clear the RCVR FIFO */
#define UART_FCR_CLEAR_XMIT	    0x04 /* Clear the XMIT FIFO */
#define UART_FCR_DMA_SELECT	    0x08 /* For DMA applications */
#define UART_FCR_TFIFO_LEVEL_5  0x20 /*n */
#define UART_FCR_TFIFO_LEVEL_4  0x10 /*n */

#define UART_FCR_TFIFO_LEVEL_1B  0x00
#define UART_FCR_TFIFO_LEVEL_4B  0x40
#define UART_FCR_TFIFO_LEVEL_8B  0x80
#define UART_FCR_TFIFO_LEVEL_14B  0xC0


#define UART_LCR 0x03
#define UART_LCR_DLAB		0x80 /* Divisor latch access bit */
#define UART_LCR_SBC		0x40 /* Set break control */
#define UART_LCR_SPAR		0x20 /* Stick parity (?) */
#define UART_LCR_EPAR		0x10 /* Even parity select */
#define UART_LCR_PARITY		0x08 /* Parity Enable */
#define UART_LCR_STOP		0x04 /* Stop bits: 0=1 bit, 1=2 bits */
#define UART_LCR_WLEN5		0x00 /* Wordlength: 5 bits */
#define UART_LCR_WLEN6		0x01 /* Wordlength: 6 bits */
#define UART_LCR_WLEN7		0x02 /* Wordlength: 7 bits */
#define UART_LCR_WLEN8		0x03 /* Wordlength: 8 bits */

#define UART_MCR 0x04
#define UART_MCR_LOOP		0x10 /* Enable loopback test mode */
#define UART_MCR_OUT2		0x08 /* n Out2 complement */
#define UART_MCR_OUT1		0x04 /* n Out1 complement */
#define UART_MCR_RTS		0x02 /* RTS complement */
#define UART_MCR_DTR		0x01 /* n DTR complement */


#define UART_LSR 0x05
#define UART_LSR_RXGERROR	0x80 /* Rx fifo global error */
#define UART_LSR_TEMT		0x40 /* Transmitter empty */
#define UART_LSR_THRE		0x20 /* Transmit-hold-register empty */
#define UART_LSR_BI	    	0x10 /* Break interrupt indicator */
#define UART_LSR_FE	    	0x08 /* Frame error indicator */
#define UART_LSR_PE		    0x04 /* Parity error indicator */
#define UART_LSR_OE	    	0x02 /* Overrun error indicator */
#define UART_LSR_DR		    0x01 /* Receiver data ready */


#define UART_MSR 0x06
#define UART_MSR_DCD		0x80 /* n Data Carrier Detect */
#define UART_MSR_RI		    0x40 /* n Ring Indicator */
#define UART_MSR_DSR		0x20 /* n Data Set Ready */
#define UART_MSR_CTS		0x10 /* Clear to Send */
#define UART_MSR_DDCD		0x08 /* n Delta DCD */
#define UART_MSR_TERI		0x04 /* n Trailing edge ring indicator */
#define UART_MSR_DDSR		0x02 /* n Delta DSR */
#define UART_MSR_DCTS		0x01 /* Delta CTS */
#define UART_MSR_ANY_DELTA	0x0F /* n Any of the delta bits! */


//free register
#define UART_SPR 0x07			/*SCR*/

void xr_stop_tx_duart(struct rcuart_port *uap);
void xr_start_tx_duart(struct rcuart_port *uap);
void xr_break_ctl_duart(struct rcuart_port *uap, int break_state);
int xr_startup_duart(struct rcuart_port *uap);
void xr_shutdown_duart(struct rcuart_port *uap);
void xr_enable_485_duart(struct rcuart_port *port);
void xr_disable_485_duart(struct rcuart_port *port);
void xr_set_termios_duart(struct rcuart_port *port, struct rctty_base_param_s * ptty_base_param);
int xr_probe_duart(unsigned int num);
void xr_unprobe_duart(void);

#endif

