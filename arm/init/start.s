/*******************************************************
*
*
*										my start code
*
*
********************************************************/
.print "*******************begin gnu-as********************"

.section .stext,"x"

.global _start
_start:

	b reset
	b int_undef
	b int_softint
	b int_perinsstop
	b int_perdatastop
	b int_notused
	b int_irq
	b int_fiq

reset:
    msr cpsr_c, #0xd2       	  @ 进入中断模式
    ldr sp, =0x40000000+1024      @ 设置中断模式栈指针

	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0xd3
	msr	cpsr,r0
    ldr sp, =0x40000000+2048      @ 设置系统模式栈指针，

turnoff_watchdog:
	ldr     r0, =0x53000000
	mov     r1, #0x0
	str     r1, [r0]

init_clk:
	ldr	r0, =0x4C000004
	ldr	r1, =(0x5c<<12)|(0x01<<4)|(0x02)       /*set MPLLCON M=92 P=1 S=2 200M*/
	str	r1, [r0]
	ldr	r0, =0x4C000014
	mov	r1, #3				                   /*set CLKOIVN 1:2:4*/
	str	r1, [r0]
	
	ldr r0, =0x56000000
	bl init_gpio
	bl init_bank
	bl copy_to_sdram
	bl init_irq
	bl init_timer

init_uart:
	ldr     r0, =0x50000000                     /*ULCON0 8bit 1stop*/
	mov     r1, #0x03
	str     r1, [r0]
	
	ldr     r0, =0x50000004                     /*UCON0 set Tx Rx in interrap mode*/
	mov     r1, #0x05
	str     r1, [r0]

	ldr     r0, =0x50000028                     /*UBRDIV0 =PCLK/115200/16-1*/
	mov     r1, #26
	str     r1, [r0]

send_uart:
	ldr     r0, =0x50000020                     /*UBRDIV0 =PCLK/115200/16-1*/
	mov     r1, #0x41
	str     r1, [r0]

.macro msenduart p1
1:
	ldr			r0, =0x50000010
	ldr			r1,[r0]
	tst			r1,#0x04
	beq			1b
	
	ldr     r0, =0x50000020                     /*UBRDIV0 =PCLK/115200/16-1*/
	mov     r1, \p1
	str     r1, [r0]
.endm

	msr cpsr, #0x5f								/*打开IRQ中断*/
	ldr pc, =(0x30000000+0xb0)
loop:

	bl delay
	ldr r5,=val
	bl printasc
	msenduart #'X'
	msenduart #'Y'
#	bl main
	b loop

.ltorg  /*定义数据池，将ldr r, =xxx中xxx的值放在这里（默认放在当前汇编文件末尾），防止程序太大超出ldr范文范围4kB*/
	
/***************	init_bank for 1s*********************/
init_bank:
	adrl r0,bankreg
	ldr r0,=bankreg
	ldr r1,=0x48000000
	add r2,r0,#13*4
1:
	ldr r3,[r0],#4
	str r3,[r1],#4
	cmp r0,r2
	bne 1b

	mov pc,lr
/***************    copy mem to sdram*********************/
copy_to_sdram:
	mov r0,#0
	ldr r1,=0x30000000
	add r2,r0,#1024*4
1:
	ldr r3,[r0],#4
	str r3,[r1],#4
	cmp r0,r2
	bne 1b

	mov pc,lr


/***************	delay fun for 1s*********************/
delay:
	ldr  r1, =(0x4096*30)
	ldr  r0, =0x00000000
1:
	add  r0, r0, #1
	cmp  r0, r1
	ble 1b 
	mov pc,lr
/****************print asc  must put addr in r5 ***********************/
printasc:
2:	
	ldrb		r3,[r5],#1
	teq			r3,#0x0
	beq			3f
	msenduart r3
	b				2b		
3:
	mov pc,lr		

/**********************init pio for asm**********************************/
init_pio:
	ldr r0, =0x56000010 @this is my command    /*set PIOB 5678 out*/
	ldr r1, =0x00015401 @this is command
	str r1,[r0] ;
        ;
	ldr r0, =0x56000014                         /*set PIOB 5678 0000 on led*/
	ldr r1, =0x00000000
	str r1,[r0]

	ldr     r0, =0x56000070                     /*GPHCON enable UART0*/
	mov     r1, #0xa0
	str     r1, [r0]

	ldr     r0, =0x56000078                     /*GPHUP UART0*/
	mov     r1, #0x0c
	str     r1, [r0]

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
	ldr r5,=int_i
	bl printasc
	bl irq_handle
    ldmia   sp!,    { r0-r12,pc }^  @ 中断返回, ^表示将spsr的值复制到cpsr !表示指令执行后更新sp中的值，即sp=sp+4*出栈寄存器数

int_fiq:
	ldr r5,=int_f
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
.byte 0b1,1,01,0x1,'1,'1'
.hword 0x12345678
.short 0x87654321
.word  0x13572468
.int	 0x5a5a5a5a
.long  0x55555555
.quad	 0x12345678
.octa	 0x87654321
.ascii "hello"
.asciz "world"
.string "sea" 
buffo:.fill 100,4,1
xi: .space 100,0xa

.comm buf,10000
.align 5
.word loop
.word .
	
	.end
