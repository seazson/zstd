/*UART registers*/
#define ULCON0              (*(volatile unsigned long *)0x50000000)
#define UCON0               (*(volatile unsigned long *)0x50000004)
#define UFCON0              (*(volatile unsigned long *)0x50000008)
#define UMCON0              (*(volatile unsigned long *)0x5000000c)
#define UTRSTAT0            (*(volatile unsigned long *)0x50000010)
#define UTXH0               (*(volatile unsigned char *)0x50000020)
#define URXH0               (*(volatile unsigned char *)0x50000024)
#define UBRDIV0             (*(volatile unsigned long *)0x50000028)

#define TXD0READY   (1<<2)
#define RXD0READY   (1)

/*interrupt registes*/
#define SRCPND              (*(volatile unsigned long *)0x4A000000)
#define INTMOD              (*(volatile unsigned long *)0x4A000004)
#define INTMSK              (*(volatile unsigned long *)0x4A000008)
#define PRIORITY            (*(volatile unsigned long *)0x4A00000c)
#define INTPND              (*(volatile unsigned long *)0x4A000010)
#define INTOFFSET           (*(volatile unsigned long *)0x4A000014)
#define SUBSRCPND           (*(volatile unsigned long *)0x4A000018)
#define INTSUBMSK           (*(volatile unsigned long *)0x4A00001c)

/*external interrupt registers*/
#define EINTMASK            (*(volatile unsigned long *)0x560000a4)
#define EINTPEND            (*(volatile unsigned long *)0x560000a8)

/*GPIO registers*/
#define GPBCON              (*(volatile unsigned long *)0x56000010)
#define GPBDAT              (*(volatile unsigned long *)0x56000014)

#define GPFCON              (*(volatile unsigned long *)0x56000050)
#define GPFDAT              (*(volatile unsigned long *)0x56000054)
#define GPFUP               (*(volatile unsigned long *)0x56000058)

#define GPGCON              (*(volatile unsigned long *)0x56000060)
#define GPGDAT              (*(volatile unsigned long *)0x56000064)
#define GPGUP               (*(volatile unsigned long *)0x56000068)

#define GPHCON              (*(volatile unsigned long *)0x56000070)
#define GPHDAT              (*(volatile unsigned long *)0x56000074)
#define GPHUP               (*(volatile unsigned long *)0x56000078)
/*PWM & Timer registers*/
#define	TCFG0		(*(volatile unsigned long *)0x51000000)
#define	TCFG1		(*(volatile unsigned long *)0x51000004)
#define	TCON		(*(volatile unsigned long *)0x51000008)
#define	TCNTB0		(*(volatile unsigned long *)0x5100000c)
#define	TCMPB0		(*(volatile unsigned long *)0x51000010)
#define	TCNTO0		(*(volatile unsigned long *)0x51000014)

#define GSTATUS1    (*(volatile unsigned long *)0x560000B0)


void init_irq(void)
{
    //配置IP脚处于外部中断模式
	GPGCON   |= (2<<(0*2)) | (2<<(3*2)) | (2<<(5*2)) | (2<<(6*2)) | (2<<(7*2)) | (2<<(11*2));
    // 对于EINT8,11,13,14,15,19，需要在EINTMASK寄存器中使能它们
    EINTMASK &= (~(1<<8)) & (~(1<<11)) & (~(1<<13)) & (~(1<<14)) & (~(1<<15)) & (~(1<<19));
    //到此只是配置了和设备相关的中断，他们

    /*
     * 设定优先级：
     * ARB_SEL0 = 00b, ARB_MODE0 = 0: REQ1 > REQ3，即EINT0 > EINT2
     * 仲裁器1、6无需设置
     * 最终：
     * EINT0 > EINT2 > EINT11,EINT19，即K4 > K3 > K1,K2
     * EINT11和EINT19的优先级相同
     */
    PRIORITY = (PRIORITY & ((~0x01) | (0x3<<7))) | (0x0 << 7) ;

    //EINT8_23,timer0使能
    INTMSK   &= (~(1<<5)) & (~(1<<10));
}

void init_timer(void)
{
    TCFG0  = 99;        // 预分频器0 = 99
    TCFG1  = 0x03;      // 选择16分频
    TCNTB0 = 31250;     // 0.5秒钟触发一次中断
    TCON   |= (1<<1);   // 手动更新
    TCON   = 0x09;      // 自动加载，清“手动更新”位，启动定时器0
}

void putc(unsigned char c);
void irq_handle(void)
{
	unsigned long IRQ_nom = INTOFFSET;
	static unsigned long led=0;
	switch(IRQ_nom)
	{
		case 5 :
			if (EINTPEND & (1<<8))
	        	putc('1');
	        if (EINTPEND & (1<<11))
	        	putc('2');
	        if (EINTPEND & (1<<13))
	        	putc('3');
	        if (EINTPEND & (1<<14))
	        	putc('4');
	        if (EINTPEND & (1<<15))
	        	putc('5');
	        if (EINTPEND & (1<<19))
	        	putc('6');
	        break;
		case 10 :
			led++;
	        GPBDAT = ~(GPBDAT & (0xf << 5));
	        GPBDAT &= 0x1e0;
				break;
		default :
				break;

	}
	putc('\n');

    EINTPEND = (1<<8) | (1<<11) | (1<<13) | (1<<14) | (1<<15) | (1<<19);   // EINT8_23合用IRQ5
    SRCPND = 1<<INTOFFSET;
    INTPND = INTPND;
}

void init_gpio(unsigned long gpio)
{
	*(volatile unsigned long *)gpio = 0x007FFFFF;    /*GPIO_A GPACON*/

	*(volatile unsigned long *)(gpio+0x10) = 0x00044555;  /*GPIO_B GPBCON output:10,8,6~0  input:7,9*/
	*(volatile unsigned long *)(gpio+0x18) = 0x000007FF;  /*GPBUP 0~10*/

	*(volatile unsigned long *)(gpio+0x20) = 0xAAAAAAAA;  /*GPIO_C GPCCON function*/
	*(volatile unsigned long *)(gpio+0x28) = 0x0000FFFF;  /*GPCUP 0~15*/

	*(volatile unsigned long *)(gpio+0x30) = 0xAAAAAAAA;  /*GPIO_D GPDCON*/
	*(volatile unsigned long *)(gpio+0x38) = 0x0000FFFF;  /*GPDUP 0~15*/

	*(volatile unsigned long *)(gpio+0x40) = 0xAAAAAAAA;  /*GPIO_E GPECON*/
	*(volatile unsigned long *)(gpio+0x48) = 0x0000FFFF;  /*GPEUP 0~15*/

	*(volatile unsigned long *)(gpio+0x50) = 0x000055AA;  /*GPIO_F GPFCON eint：0~3  output:4~7*/
	*(volatile unsigned long *)(gpio+0x58) = 0x000000FF;  /*GPFUP 0~7*/

	*(volatile unsigned long *)(gpio+0x60) = 0xFF95FFBA;  /*GPIO_G GPGCON spi lcd nss0 EINT:8,9,11,19 output:10,9,8*/
	*(volatile unsigned long *)(gpio+0x68) = 0x0000FFFF;  /*GPGUP 0~15*/

	*(volatile unsigned long *)(gpio+0x70) = 0x002AFAAA;  /*GPIO_H GPHCON nRTS0 UART0 UART1 outclk0/1 extclk*/
	*(volatile unsigned long *)(gpio+0x78) = 0x000007FF;  /*GPHUP 0~10*/
}


void putc(unsigned char c)
{
    
    while (!(UTRSTAT0 & TXD0READY));
    
    UTXH0 = c;
}

unsigned char getc(void)
{    /* 等待，直到接收缓冲区中的有数据 */
    while (!(UTRSTAT0 & RXD0READY));
    
    return URXH0;
}

 
int main()
{
	putc('Z');
	while(1);
	return 0;
}
