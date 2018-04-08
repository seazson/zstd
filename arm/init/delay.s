.globl delay_1
.type delay_1,%object
.size delay_1,4
delay_1:
	.rept 0xA
		mov r0,r0
	.endr
	mov pc,lr
.globl delaytime
.set delaytime,0x4096*60

