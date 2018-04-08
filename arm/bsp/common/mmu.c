#include "common.h"

/*��׼���ַ*/
#define TTBBASE 0x100000    /*L1 location*/

/*��Ȩ��*/
#define MMU_AP 				(3 << 10)
#define MMU_DOMAIN 			(0 << 5)
#define MMU_SPECIAL			(1 << 4)
#define MMU_CACHE           (1 << 3)
#define MMU_BUFFER			(1 << 2)
#define MMU_SECTION			(2)

#define MMU_ARGS			(MMU_AP | MMU_DOMAIN | MMU_SPECIAL | MMU_SECTION)
#define MMU_ARGS_CB			(MMU_AP | MMU_DOMAIN | MMU_SPECIAL | MMU_SECTION | MMU_CACHE | MMU_BUFFER)

/*ʹ�� mmu */
void mmu_enable(void)
{
	unsigned long ttb = TTBBASE;
	asm("mov r0,#0 \n"
		"mcr p15, 0, r0, c7, c7 , 0\n"  /*ʹ��ЧI/Dcahce*/
		"mcr p15, 0, r0, c7, c10, 4\n"  /*flush д����*/
		"mcr p15, 0, r0, c8, c7,  0\n"  /*ʹ��Чָ��/����TLB*/
		/*��ʼ��TTB��ַ�Ĵ���*/
		"mov r4, %0\n"
		"mcr p15, 0, r4, c2, c0,  0\n"
		/*���������ȫ������*/
		"mvn r0, #0\n"
		"mcr p15, 0, r0, c3, c0,  0\n"
		/*����c1����λ*/
		"mrc p15, 0, r0, c1, c0,  0\n"  
		"bic r0, r0, #0x3000\n"  /*��ֹicache ѡ��Ͷ��ж�������*/
		"bic r0, r0, #0x0300\n"  /*���RSλ ����mmu��Ȩ�����*/
		"bic r0, r0, #0x0087\n"  /*��ֹMMU,��ֹ��ַ������,��ֹdcache,ѡ��С��ģʽ*/
		"orr r0, r0, #0x0002\n"  /*ʹ�ܶ�����*/
		"orr r0, r0, #0x0004\n"  /*ʹ��Dcache*/
		"orr r0, r0, #0x1000\n"  /*ʹ��Icache */
		"orr r0, r0, #0x0001\n"  /*ʹ��MMU*/
		"mcr p15, 0, r0, c1, c0,  0\n"
		:
		: "r"(ttb)
		);
}

/*��ֹMMU*/
void mmu_disable(void)
{
	unsigned long ttb = TTBBASE;
	asm(/*����c1����λ*/
		"mrc p15, 0, r0, c1, c0,  0\n"  
		"bic r0, r0, #0x0001\n"
		"mcr p15, 0, r0, c1, c0,  0\n"
		);
}

/*�������ַVM ӳ�䵽�����ַPA ��ӳ��*/
void mmu_map(unsigned long VM, unsigned long PA)
{
	unsigned long ttb = (unsigned long)TTBBASE;
	*(unsigned long *)(TTBBASE + (VM >> 18)) = (PA & 0xFFF00000) | MMU_ARGS;
}

/*�������ַVM ӳ�䵽�����ַPA ��ӳ�䣬ͬʱ��cache��д����*/
void mmu_map_cached(unsigned long VM, unsigned long PA)
{
	unsigned long ttb = (unsigned long)TTBBASE;
	*(unsigned long *)(TTBBASE + (VM >> 18)) = (PA & 0xFFF00000) | MMU_ARGS_CB;
}

