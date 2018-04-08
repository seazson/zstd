#include "reg.h"

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
    TCFG0  = 0;        // 预分频器 不分频
    TCFG1  = 0x00;      // 选择2分频
    TCNTB0 = 30000;     // 0.5秒钟触发一次中断
    TCON   |= (1<<1);   // 手动更新 timer0=pclk/2
}

inline void timer0_begin(void)
{
	TCON   = 0x00;
	TCNTB0 = 30000;     // 0.5秒钟触发一次中断
    TCON   |= (1<<1);   // 手动更新
    TCON   = 0x01;
}

inline unsigned int timer0_read(void)
{
    unsigned int res;
	res = TCNTO0;
	return res;
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

void puts(const char *s)
{
	while (*s) 
	{
		putc(*s++);
	}
}

void put_int(unsigned int i)
{
	printf("ASM Timer is %d\r\n",30000-i);
}

int main()
{
	unsigned int off;
	int i;
	while(1)
	{
/*		timer0_begin();
		for(i=0;i<100;i++);
		off = timer0_read();
		delay();
		printf("Timer is %d\r\n",30000-off);*/
		printf("***********************test begin**************************\r\n");
		int_add();
		int_des();
		int_multi();
		int_div();
		load_test();
		store_test();
		delay();
	}
	return 0;
}
