all:
	arm-linux-gcc -g -c -o crt0.o crt0.s
	arm-linux-ld -Tlink.lds -g crt0.o -o crt0_elf
	arm-linux-objcopy -O binary  crt0_elf crt0.bin
	arm-linux-objdump -D crt0_elf > crt0_elf.dis
	#arm-linux-objdump -D -b binary -m arm crt0.bin > crt0.dis
clean:
	rm -f   crt.bin crt_elf *.o
