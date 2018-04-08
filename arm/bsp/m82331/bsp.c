#include "bsp.h"
#include "common.h"
#include "reg.h"

unsigned int map;

void boot(void)
{
	asm("ldr pc,=0xb8000000":::"cc");
}

void s1(void)
{
	unsigned int i,j,k; 
	unsigned int data;	
	printf("test begin...\n");
	for(k=0 ;k<10; k++)
	{
		tbegin();
		for(j=0;j<8*64;j+=4)
		{
			data = (*(volatile unsigned int*)(0x200000+j));
			map = data + j;
		}
		tend();
	}
}

void test_cache(void)
{

	dcache_disable();
	icache_disable();	
	s1();
	cache_clean();
	s1();
	
	dcache_disable();
	icache_enable();
	s1();
	icache_clean();
	s1();
	
	icache_disable();
	dcache_enable();
	s1();
	dcache_clean();
	s1();
	
	dcache_enable();
	icache_enable();
	s1();
	cache_clean();
	s1();

}

extern struct uart_regs *uart;
extern struct uart_dl_regs *uart_dl;
extern unsigned int timer_base;

test_mmu()
{
	mmu_map(0x0,0x0);
	mmu_map(0x100000,0x100000);
	mmu_map_cached(0x200000,0x200000);
//	mmu_map(0x10000000,0x10000000);
	mmu_map(0x30000000,0x10000000);
	mmu_map(0xb8000000,0xb8000000);
	mmu_enable();
	uart = (struct uart_regs *) (COMCERTO_UART0_BASE+0x20000000);
	uart_dl = (struct uart_dl_regs *) (COMCERTO_UART0_BASE+0x20000000);
	timer_base += 0x20000000;
	printf("after mmu\n");

}

void bsp_init(void)
{
	tbegin();
	serial_init();
	serial_puts("season in the sun\n");
	printf("sea\n");
	show_cpuid();
	show_cacheinfo();
	tend();
	
	test_cache();
	test_mmu();
	test_cache();
	mmu_disable();
	while(1)
	{
		boot();
	}
}
