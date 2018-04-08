#include "common.h"

#define CACHEBIT (1<<2)
#define DCACHEBIT (1<<2)
#define ICACHEBIT (1<<12)

/***************************************************************************
*
*	cache使能与禁止		
*
*/
void cache_enable()
{
	unsigned int c1;
	asm("mrc p15,0,%0,c1,c0,0":"=r"(c1)::"cc");
	c1 = c1 &~ CACHEBIT;
	c1 = c1 | CACHEBIT;
	asm("mcr p15,0,%0,c1,c0,0"::"r"(c1):"cc");
}

void icache_enable()
{
	unsigned int c1;
	asm("mrc p15,0,%0,c1,c0,0":"=r"(c1)::"cc");
	c1 = c1 &~ ICACHEBIT;
	c1 = c1 | ICACHEBIT;
	asm("mcr p15,0,%0,c1,c0,0"::"r"(c1):"cc");
}

void dcache_enable()
{
	unsigned int c1;
	asm("mrc p15,0,%0,c1,c0,0":"=r"(c1)::"cc");
	c1 = c1 &~ DCACHEBIT;
	c1 = c1 | DCACHEBIT;
	asm("mcr p15,0,%0,c1,c0,0"::"r"(c1):"cc");
}

void cache_disable()
{
	unsigned int c1;
	asm("mrc p15,0,%0,c1,c0,0":"=r"(c1)::"cc");
	c1 = c1 &~ CACHEBIT;
	asm("mcr p15,0,%0,c1,c0,0"::"r"(c1):"cc");
}

void icache_disable()
{
	unsigned int c1;
	asm("mrc p15,0,%0,c1,c0,0":"=r"(c1)::"cc");
	c1 = c1 &~ ICACHEBIT;
	asm("mcr p15,0,%0,c1,c0,0"::"r"(c1):"cc");
}

void dcache_disable()
{
	unsigned int c1;
	asm("mrc p15,0,%0,c1,c0,0":"=r"(c1)::"cc");
	c1 = c1 &~ DCACHEBIT;
	asm("mcr p15,0,%0,c1,c0,0"::"r"(c1):"cc");
}
/***************************************************************************
*
*	清除cache		
*
*/
void cache_clean()
{
	unsigned int c7 = 0;
	asm("mcr p15,0,%0,c7,c7,0"::"r"(c7):"cc");
}

void icache_clean()
{
	unsigned int c7 = 0;
	asm("mcr p15,0,%0,c7,c5,0"::"r"(c7):"cc");
}

void dcache_clean()
{
	unsigned int c7 = 0;
	asm("mcr p15,0,%0,c7,c6,0"::"r"(c7):"cc");
}

/***************************************************************************
*
*	写回cache，通过组或者地址来清理		
*
*/
void cache_flush()
{
	unsigned int c7 = 0;
	asm("mcr p15,0,%0,c7,c7,0"::"r"(c7):"cc");
}

void icache_flush()
{
	unsigned int c7 = 0;
	asm("mcr p15,0,%0,c7,c5,0"::"r"(c7):"cc");
}

void dcache_flush()
{
	unsigned int c7 = 0;
	asm("mcr p15,0,%0,c7,c6,0"::"r"(c7):"cc");
}

