
#include "reg.h"
typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned short u16;

struct uart_regs {
	volatile u32 data;	/* Receive/Transmit data register */
	volatile u32 ier;	/* Interrupt Enable register */
	volatile u32 iir_fcr;	/* Interrupt Identity register / FIFO Control register */
	volatile u32 lcr;	/* Line Control register */
	volatile u32 mcr;	/* Modem Control register */
	volatile u32 lsr;	/* Line Status register */
	volatile u32 msr;	/* Modem Status register */
	volatile u32 sr;	/* Scratch register */
	volatile u32 reserved[23];
	volatile u32 usr;	/* UART Status register */
};

struct uart_dl_regs {
	volatile u32 dll;	/* Divisor Latch (Low) */
	volatile u32 dlh;	/* Divisor Latch (High) */
};


/* Interrupt Enable Register */
/* UART 16550 */
#define IER_RDI			(1 << 0)	/* Enable Received Data Available Interrupt */
#define IER_THRI		(1 << 1)	/* Enable Transmitter holding register Interrupt */
#define IER_RLSI		(1 << 2)	/* Enable receiver line status Interrupt */
#define IER_MSI			(1 << 3)	/* Enable Modem status Interrupt */

#define IIR_FIFOES1		(1 << 7)	/* FIFO Mode Enable Status */
#define IIR_FIFOES0		(1 << 6)	/* FIFO Mode Enable Status */
#define IIR_TOD			(1 << 3)	/* Time Out Detected */
#define IIR_IID2		(1 << 2)	/* Interrupt Source Encoded */
#define IIR_IID1		(1 << 1)	/* Interrupt Source Encoded */
#define IIR_IP			(1 << 0)	/* Interrupt Pending (active low) */

/* UART 16550 FIFO Control Register */
#define FCR_FIFOEN		(1 << 0)
#define FCR_RCVRRES		(1 << 1)
#define FCR_XMITRES		(1 << 2)

#define LCR_CHAR_LEN_5		0
#define LCR_CHAR_LEN_6		1
#define LCR_CHAR_LEN_7		2
#define LCR_CHAR_LEN_8		3
#define LCR_TWO_STOP		(1 << 2)	/* Two stop bit! */
#define LCR_PEN			(1 << 3)	/* Parity Enable */
#define LCR_EPS			(1 << 4)	/* Even Parity Select */
#define LCR_PS			(1 << 5)	/* Enable Parity  Stuff */
#define LCR_SBRK		(1 << 6)	/* Start Break */
#define LCR_PSB			(1 << 7)	/* Parity Stuff Bit */
#define LCR_DLAB		(1 << 7)	/* UART 16550 Divisor Latch Assess */

#define LSR_RXFIFO_ERROR	(1 << 7)	/* FIFO Error Status */
#define LSR_TEMT		(1 << 6)	/* Transmitter Empty */
#define LSR_THRE		(1 << 5)	/* Transmit Data Request */
#define LSR_BI			(1 << 4)	/* Break Interrupt */
#define LSR_FE			(1 << 3)	/* Framing Error */
#define LSR_PE			(1 << 2)	/* Parity Error */
#define LSR_OE			(1 << 1)	/* Overrun Error */
#define LSR_DR			(1 << 0)	/* Data Ready */

#define USR_BUSY		(1 << 0)	/* UART Busy - LCR can't be written if this bit is 1 */

struct uart_regs *uart = (struct uart_regs *) COMCERTO_UART0_BASE;
struct uart_dl_regs *uart_dl = (struct uart_dl_regs *) COMCERTO_UART0_BASE;

/*
    * Comcerto 300 UART has 'busy' functionality, LCR can't be written if
	 * UART receiving/transmitting data or holds received data
	  */
static void serial_set_lcr(u8 lcr)
{
	while (1) {
		/* waiting for transmitter to become not busy, flush rx to avoid infinite loops */
		while (uart->usr & USR_BUSY)
		uart->iir_fcr = FCR_RCVRRES | FCR_FIFOEN;

		/* write LCR, read it back and compare - this is needed becase we have a chance
		* that incoming character will make UART busy again - it's possible to get serious
		* problems in this case, especially when we in the middle of setting baud rate (DLAB
		* is on)
		*/
		uart->lcr = lcr;
		if (uart->lcr == lcr)
		break;
	}
}

#define CFG_HZ_CLOCK			137500000		/* 137.5MHz */
#define CONFIG_BAUDRATE         115200			
void serial_setbrg(void)
{
	int baudrate = 115200;
	u32 divisor;

	if (baudrate <= 0)
		baudrate = CONFIG_BAUDRATE;

	divisor = CFG_HZ_CLOCK / (115200 * 16);

	serial_set_lcr(uart->lcr | LCR_DLAB);

	uart_dl->dll = divisor & 0xFF;
	uart_dl->dlh = divisor >> 8;

	serial_set_lcr(uart->lcr & ~LCR_DLAB);
}

int serial_init(void)
{
	uart->ier = 0;
	uart->iir_fcr = 0;
	serial_set_lcr(LCR_CHAR_LEN_8);

	serial_setbrg();

	uart->data = 0;
	uart->iir_fcr = FCR_RCVRRES | FCR_XMITRES | FCR_FIFOEN;

	return 0;
}

void serial_putc(const char c)
{
	/* wait for room in the tx FIFO */
	while ((uart->lsr & LSR_THRE) == 0)
	;

	uart->data = c;
	if (c == '\n')
	serial_putc ('\r');
}

void serial_puts(const char *s)
{
	while (*s)
		serial_putc(*s++);
}

int serial_getc(void)
{
	while ((uart->lsr & LSR_DR) == 0);
	return (char)uart->data & 0xFF;
}

int serial_tstc(void)
{
	return uart->lsr & LSR_DR;
}

void puts(const char *s)
{
	serial_puts(s);
}
