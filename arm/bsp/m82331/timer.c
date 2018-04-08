#include "reg.h"
#include "common.h"

#define readl(addr) 	(*(volatile unsigned int*)(addr))
#define writel(b,addr) 	((*(volatile unsigned int *) (addr)) = (b))

unsigned int last_timer;
unsigned int timer_base = 0x10050000;


void timer_init()
{
	writel(0xffffffff, timer_base);
}

unsigned int clock(void)
{
	return readl(timer_base + 0x4);
}

void tbegin(void)
{
	timer_init();
}

void tend(void)
{
	last_timer = clock();
	printf("use clock %d [%x]\n",last_timer, last_timer);
}
