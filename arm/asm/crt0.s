.text
.global _start
_start:
	ldr r0, =0x56000010 @this is my command
	ldr r1, =0x00001e01 @this is command
	str r1,[r0] ;
        ;
	ldr r0, =0x56000014
	ldr r1, =0x000000a0
	str r1,[r0]
loop:
	b loop
1:
	b 1f
2:
	b 2f
1:
	b 2b	
2:
	b 1b
.byte 'A
.word loop
.align 4
.word .
.string "hello world\n"
