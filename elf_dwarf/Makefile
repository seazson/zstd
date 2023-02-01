all:
	gcc -o bp_run bp_use_lib.c debuglib.c
	gcc -o run ptrace.c
	gcc -g -o test test.c
	#gcc -g -o test1 test1.c test2.c
	#gcc -o get_func.o get_func.c -ldwarf -lelf
	gcc -g -o dwflline dwfllines.c -ldw -lelf
	gcc -g -o backtrace backtrace.c -ldw -lelf
	gcc -o backtrace-child backtrace-child.c -lpthread
	gcc -o backtrace-d backtrace-data.c -ldw -lelf
