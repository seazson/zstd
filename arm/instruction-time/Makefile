all:
	arm-linux-gcc -g -c -o start.o start.S
	arm-linux-gcc -g -c -o init.o init.c
	arm-linux-gcc -g -c -o test_cpu.o test_cpu.c
	arm-linux-gcc -g -c -o printf.o printf.c
	arm-linux-gcc -g -c -o div64.o div64.S
	arm-linux-ld -Tlink.lds -g  start.o init.o printf.o div64.o test_cpu.o -o sea.elf
	arm-linux-objcopy -O binary  sea.elf sea.bin
	arm-linux-objdump -D sea.elf > sea.dis
clean:
	rm -f   sea.* *.o
