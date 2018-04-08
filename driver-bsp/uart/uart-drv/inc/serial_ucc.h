#ifndef __SERIAL_UCC_H__
#define __SERIAL_UCC_H__

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/fs_uart_pd.h>
#include <asm/ucc_slow.h>
#include <linux/circ_buf.h>

#include <linux/firmware.h>
#include <asm/reg.h>

/* The major and minor device numbers are defined in
 * http://www.lanana.org/docs/device-list/devices-2.6+.txt.  For the QE
 * UART, we have major number 204 and minor numbers 46 - 49, which are the
 * same as for the CPM2.  This decision was made because no Freescale part
 * has both a CPM and a QE.
 */

#define TTY_NORMAL  0
#define TTY_BREAK   1
#define TTY_FRAME   2
#define TTY_PARITY  3
#define TTY_OVERRUN 4

#define SERIAL_QE_MAJOR 204
#define SERIAL_QE_MINOR 46

/* Since we only have minor numbers 46 - 49, there is a hard limit of 4 ports */
#define UCC_MAX_UART    4

/* The number of buffer descriptors for receiving characters. */
#define RX_NUM_FIFO     4

/* The number of buffer descriptors for transmitting characters. */
#define TX_NUM_FIFO     4

/* The maximum size of the character buffer for a single RX BD. buffer太小可能丢包*/
#define RX_BUF_SIZE     64

/* The maximum size of the character buffer for a single TX BD. */
#define TX_BUF_SIZE     64

/*
 * The number of jiffies to wait after receiving a close command before the
 * device is actually closed.  This allows the last few characters to be
 * sent over the wire.
 */
#define UCC_WAIT_CLOSING 100

struct ucc_uart_pram {
	struct ucc_slow_pram common;
	u8 res1[8];     	/* reserved */
	__be16 maxidl;  	/* Maximum idle chars */
	__be16 idlc;    	/* temp idle counter */
	__be16 brkcr;   	/* Break count register */
	__be16 parec;   	/* receive parity error counter */
	__be16 frmec;   	/* receive framing error counter */
	__be16 nosec;   	/* receive noise counter */
	__be16 brkec;   	/* receive break condition counter */
	__be16 brkln;   	/* last received break length */
	__be16 uaddr[2];	/* UART address character 1 & 2 */
	__be16 rtemp;   	/* Temp storage */
	__be16 toseq;   	/* Transmit out of sequence char */
	__be16 cchars[8];       /* control characters 1-8 */
	__be16 rccm;    	/* receive control character mask */
	__be16 rccr;    	/* receive control character register */
	__be16 rlbc;    	/* receive last break character */
	__be16 res2;    	/* reserved */
	__be32 res3;    	/* reserved, should be cleared */
	u8 res4;		/* reserved, should be cleared */
	u8 res5[3];     	/* reserved, should be cleared */
	__be32 res6;    	/* reserved, should be cleared */
	__be32 res7;    	/* reserved, should be cleared */
	__be32 res8;    	/* reserved, should be cleared */
	__be32 res9;    	/* reserved, should be cleared */
	__be32 res10;   	/* reserved, should be cleared */
	__be32 res11;   	/* reserved, should be cleared */
	__be32 res12;   	/* reserved, should be cleared */
	__be32 res13;   	/* reserved, should be cleared */
/* The rest is for Soft-UART only */
	__be16 supsmr;  	/* 0x90, Shadow UPSMR */
	__be16 res92;   	/* 0x92, reserved, initialize to 0 */
	__be32 rx_state;	/* 0x94, RX state, initialize to 0 */
	__be32 rx_cnt;  	/* 0x98, RX count, initialize to 0 */
	u8 rx_length;   	/* 0x9C, Char length, set to 1+CL+PEN+1+SL */
	u8 rx_bitmark;  	/* 0x9D, reserved, initialize to 0 */
	u8 rx_temp_dlst_qe;     /* 0x9E, reserved, initialize to 0 */
	u8 res14[0xBC - 0x9F];  /* reserved */
	__be32 dump_ptr;	/* 0xBC, Dump pointer */
	__be32 rx_frame_rem;    /* 0xC0, reserved, initialize to 0 */
	u8 rx_frame_rem_size;   /* 0xC4, reserved, initialize to 0 */
	u8 tx_mode;     	/* 0xC5, mode, 0=AHDLC, 1=UART */
	__be16 tx_state;	/* 0xC6, TX state */
	u8 res15[0xD0 - 0xC8];  /* reserved */
	__be32 resD0;   	/* 0xD0, reserved, initialize to 0 */
	u8 resD4;       	/* 0xD4, reserved, initialize to 0 */
	__be16 resD5;   	/* 0xD5, reserved, initialize to 0 */
} __attribute__ ((packed));

/* UCC Protocol Specific Mode Register (UPSMR), when used for UART */
#define UCC_UART_UPSMR_FLC		0x8000
#define UCC_UART_UPSMR_SL		0x4000
#define UCC_UART_UPSMR_CL_MASK		0x3000
#define UCC_UART_UPSMR_CL_8		0x3000
#define UCC_UART_UPSMR_CL_7		0x2000
#define UCC_UART_UPSMR_CL_6		0x1000
#define UCC_UART_UPSMR_CL_5		0x0000
#define UCC_UART_UPSMR_UM_MASK		0x0c00
#define UCC_UART_UPSMR_UM_NORMAL	0x0000
#define UCC_UART_UPSMR_UM_MAN_MULTI	0x0400
#define UCC_UART_UPSMR_UM_AUTO_MULTI	0x0c00
#define UCC_UART_UPSMR_FRZ		0x0200
#define UCC_UART_UPSMR_RZS		0x0100
#define UCC_UART_UPSMR_SYN		0x0080
#define UCC_UART_UPSMR_DRT		0x0040
#define UCC_UART_UPSMR_PEN		0x0010
#define UCC_UART_UPSMR_RPM_MASK		0x000c
#define UCC_UART_UPSMR_RPM_ODD		0x0000
#define UCC_UART_UPSMR_RPM_LOW		0x0004
#define UCC_UART_UPSMR_RPM_EVEN		0x0008
#define UCC_UART_UPSMR_RPM_HIGH		0x000C
#define UCC_UART_UPSMR_TPM_MASK		0x0003
#define UCC_UART_UPSMR_TPM_ODD		0x0000
#define UCC_UART_UPSMR_TPM_LOW		0x0001
#define UCC_UART_UPSMR_TPM_EVEN		0x0002
#define UCC_UART_UPSMR_TPM_HIGH		0x0003

#define UCC_UART_TX_STATE_AHDLC 	0x00
#define UCC_UART_TX_STATE_UART  	0x01
#define UCC_UART_TX_STATE_X1    	0x00
#define UCC_UART_TX_STATE_X16   	0x80

#define UCC_UART_PRAM_ALIGNMENT 0x100

#define UCC_UART_SIZE_OF_BD     UCC_SLOW_SIZE_OF_BD
#define NUM_CONTROL_CHARS       8

/* Private per-port data structure */
struct uart_qe_port {
	struct rcuart_port port;
	struct ucc_slow __iomem *uccp;
	struct ucc_uart_pram __iomem *uccup;
	struct ucc_slow_info us_info;
	struct ucc_slow_private *us_private;
	struct device_node *np;
	unsigned int ucc_num;   /* First ucc is 0, not 1 */

	u16 rx_nrfifos;
	u16 rx_fifosize;
	u16 tx_nrfifos;
	u16 tx_fifosize;
	int wait_closing;
	u32 flags;
	struct qe_bd *rx_bd_base;
	struct qe_bd *rx_cur;
	struct qe_bd *tx_bd_base;
	struct qe_bd *tx_cur;
	unsigned char *tx_buf;
	unsigned char *rx_buf;
	void *bd_virt;  	/* virtual address of the BD buffers */
	dma_addr_t bd_dma_addr; /* bus address of the BD buffers */
	unsigned int bd_size;   /* size of BD buffer space */
};

void xr_stop_tx_ucc(struct rcuart_port *uap);
void xr_start_tx_ucc(struct rcuart_port *uap);
void xr_break_ctl_ucc(struct rcuart_port *uap, int break_state);
int xr_startup_ucc(struct rcuart_port *uap);
void xr_shutdown_ucc(struct rcuart_port *uap);
void xr_enable_485_ucc(struct rcuart_port *port);
void xr_disable_485_ucc(struct rcuart_port *port);
void xr_set_termios_ucc(struct rcuart_port *port, struct rctty_base_param_s * ptty_base_param);
int xr_probe_ucc(int irq);
void xr_unprobe_ucc(void);

#endif

