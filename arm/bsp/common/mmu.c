#include "common.h"

/*基准表地址*/
#define TTBBASE 0x100000    /*L1 location*/

/*域权限*/
#define MMU_AP 				(3 << 10)
#define MMU_DOMAIN 			(0 << 5)
#define MMU_SPECIAL			(1 << 4)
#define MMU_CACHE           (1 << 3)
#define MMU_BUFFER			(1 << 2)
#define MMU_SECTION			(2)

#define MMU_ARGS			(MMU_AP | MMU_DOMAIN | MMU_SPECIAL | MMU_SECTION)
#define MMU_ARGS_CB			(MMU_AP | MMU_DOMAIN | MMU_SPECIAL | MMU_SECTION | MMU_CACHE | MMU_BUFFER)

/*使能 mmu */
void mmu_enable(void)
{
	unsigned long ttb = TTBBASE;
	asm("mov r0,#0 \n"
		"mcr p15, 0, r0, c7, c7 , 0\n"  /*使无效I/Dcahce*/
		"mcr p15, 0, r0, c7, c10, 4\n"  /*flush 写缓存*/
		"mcr p15, 0, r0, c8, c7,  0\n"  /*使无效指令/数据TLB*/
		/*初始化TTB基址寄存器*/
		"mov r4, %0\n"
		"mcr p15, 0, r4, c2, c0,  0\n"
		/*访问域控制全部开启*/
		"mvn r0, #0\n"
		"mcr p15, 0, r0, c3, c0,  0\n"
		/*设置c1控制位*/
		"mrc p15, 0, r0, c1, c0,  0\n"  
		"bic r0, r0, #0x3000\n"  /*禁止icache 选择低端中断向量表*/
		"bic r0, r0, #0x0300\n"  /*清除RS位 用于mmu域权限相关*/
		"bic r0, r0, #0x0087\n"  /*禁止MMU,禁止地址对齐检查,禁止dcache,选择小端模式*/
		"orr r0, r0, #0x0002\n"  /*使能对齐检测*/
		"orr r0, r0, #0x0004\n"  /*使能Dcache*/
		"orr r0, r0, #0x1000\n"  /*使能Icache */
		"orr r0, r0, #0x0001\n"  /*使能MMU*/
		"mcr p15, 0, r0, c1, c0,  0\n"
		:
		: "r"(ttb)
		);
}

/*禁止MMU*/
void mmu_disable(void)
{
	unsigned long ttb = TTBBASE;
	asm(/*设置c1控制位*/
		"mrc p15, 0, r0, c1, c0,  0\n"  
		"bic r0, r0, #0x0001\n"
		"mcr p15, 0, r0, c1, c0,  0\n"
		);
}

/*将虚拟地址VM 映射到物理地址PA 段映射*/
void mmu_map(unsigned long VM, unsigned long PA)
{
	unsigned long ttb = (unsigned long)TTBBASE;
	*(unsigned long *)(TTBBASE + (VM >> 18)) = (PA & 0xFFF00000) | MMU_ARGS;
}

/*将虚拟地址VM 映射到物理地址PA 段映射，同时打开cache和写缓存*/
void mmu_map_cached(unsigned long VM, unsigned long PA)
{
	unsigned long ttb = (unsigned long)TTBBASE;
	*(unsigned long *)(TTBBASE + (VM >> 18)) = (PA & 0xFFF00000) | MMU_ARGS_CB;
}

