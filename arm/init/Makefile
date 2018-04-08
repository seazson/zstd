all:
	/opt/crosstool/gcc-3.4.5-glibc-2.3.6/arm-softfloat-linux-gnu/bin/arm-softfloat-linux-gnu-gcc -g -c -o start.o start.s
	/opt/crosstool/gcc-3.4.5-glibc-2.3.6/arm-softfloat-linux-gnu/bin/arm-softfloat-linux-gnu-gcc -g -c -o delay.o delay.s
	/opt/crosstool/gcc-3.4.5-glibc-2.3.6/arm-softfloat-linux-gnu/bin/arm-softfloat-linux-gnu-gcc -g -c -o init.o init.c
	/opt/crosstool/gcc-3.4.5-glibc-2.3.6/arm-softfloat-linux-gnu/bin/arm-softfloat-linux-gnu-ld -Tlink.lds -g start.o delay.o init.o -o sea.elf
	/opt/crosstool/gcc-3.4.5-glibc-2.3.6/arm-softfloat-linux-gnu/bin/arm-softfloat-linux-gnu-objcopy -O binary  sea.elf sea.bin
	/opt/crosstool/gcc-3.4.5-glibc-2.3.6/arm-softfloat-linux-gnu/bin/arm-softfloat-linux-gnu-objdump -D  -t -T  sea.elf > sea.dis
	#arm-linux-objdump -D -b binary -m arm crt0.bin > crt0.dis
clean:
	rm -f   sea.* *.o
