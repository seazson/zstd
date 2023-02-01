#include <stdlib.h>
#include <stdio.h>

extern int add(int, int);
extern int sub(int, int);
extern int sub_in(int, int);

void main(int argc, char *argv[])
{
	printf("add %d\n", add(1,3));
	printf("sub %d\n", sub(3,1));

	//不能调用sub_in,他是一个local函数
	//printf("sub_in %d\n", sub_in(3,1));
}
