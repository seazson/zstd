all:
	arm-softfloat-linux-gnu-gcc -g -c -o hello.o hello.c
	arm-softfloat-linux-gnu-gcc -fPIC -shared -g -c -o add.o add.c
	arm-softfloat-linux-gnu-ld -T link.lds -g  -verbose hello.o add.o -o hello.elf 
	arm-softfloat-linux-gnu-objcopy -O binary  hello.elf hello.bin
	arm-softfloat-linux-gnu-objdump -D  -t -T  hello.elf > hello.dis
clean:
	rm -f   sea.* *.o
