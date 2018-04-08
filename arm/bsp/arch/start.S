/*******************************************************************************************
*
*
*										my start code
*
*
********************************************************************************************/
.print "*******************begin gnu-as********************"

.section .stext,"x"

.global _start
_start:
	b       reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq

	_undefined_instruction:	.word int_undef
	_software_interrupt:	.word int_softint
	_prefetch_abort:		.word int_perinsstop
	_data_abort:			.word int_perdatastop
	_not_used:				.word int_notused
	_irq:					.word int_irq
	_fiq:					.word int_fiq

	.balignl 16,0xabcdef20

TextBase: 	.word _start
CodeEnd:	.word _codeend

reset:
	/*
	 * set the cpu to Supervisor mode
	 */
	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0xd3
	msr	cpsr,r0

	bl cache_mmu_init
	bl soc_init


cache_mmu_init:
	/*
	 * flush v4 I/D caches
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

	/*
	 * disable MMU stuff and dcaches,enable i cache
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	@ clear bits 13, 9:8 (--V- --RS)
	bic	r0, r0, #0x00000087	@ clear bits 7, 2:0 (B--- -CAM)
	orr	r0, r0, #0x00000002	@ set bit 2 (A) Align
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-Cache
	orr	r0, r0, #0x00400000	@ set bit 22 (U) 
	mcr	p15, 0, r0, c1, c0, 0

	mov	pc, lr		/* back to my caller */
	


printasc:
	mov pc, r14

loop:
	ldr pc, =loop
/************************interrept****************************/
int_undef:
	ldr r5,=int_ud
	bl printasc
	b loop

int_softint:
	ldr r5,=int_sf
	bl printasc
	b loop

int_perinsstop:
	ldr r5,=int_pi
	bl printasc
	b loop

int_perdatastop:
	ldr r5,=int_pd
	bl printasc
	b loop

int_notused:
	ldr r5,=int_uu
	bl printasc
	b loop

int_irq:
    sub lr, lr, #4                  @ 计算返回地址
    stmdb   sp!,    { r0-r12,lr }   @ 保存使用到的寄存器
#	ldr r5,=int_i
	bl printasc
#	bl irq_handle
    ldmia   sp!,    { r0-r12,pc }^  @ 中断返回, ^表示将spsr的值复制到cpsr !表示指令执行后更新sp中的值，即sp=sp+4*出栈寄存器数

int_fiq:
#	ldr r5,=int_f
	bl printasc
	b loop



/*******************************************************/
#.section .rodata
.align 5
.global val
val:	.string "This is sea\r\n"
int_ud:	.string "Undef ins\r\n"
int_sf:	.string "Soft interrept\r\n"
int_pi:	.string "Per ins wrong\r\n"
int_pd:	.string "Per data wrong\r\n"
int_uu:	.string "Unused\r\n"
int_i:	.string "Irq interrept\r\n"
int_f:	.string "Frq interrept\r\n"
	
.set a1,0x56785555
.equ a2,0x01

.align 4
bankreg:
    .word (0+(0x2<<4)+(0x1<<8)+(0x1<<12)+(0xd<<16)+(0x1<<20)+(0x2<<24)+(0x2<<28))
    .word (0x7<<8)
    .word (0x7<<8)
    .word (0x7<<8)
    .word ((0x3<<11)+(0x7<<8)+(0x1<<6)+(0x3<<2))
    .word (0x7<<8)
    .word (0x7<<8)
    .word ((0x3<<15)+(0x1<<2)+(0x1))
    .word ((0x3<<15)+(0x1<<2)+(0x1))
    .word ((0x4f4<<23)+(0x0<<22)+(0x0<<20)+(0x3<<18)+(0x2<<16)+0x4f4)
    .word 0x32
    .word 0x30
    .word 0x30

.section .data
.align 5

	.end
