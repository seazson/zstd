#include <stdlib.h>

int add_v1(int a, int b)
{
	return a+b;
}

int add_v2(int a, int b)
{
	return a+b+2;
}

int sub(int a, int b)
{
	return a-b;
}

int sub_in(int a, int b)
{
	return a-b;
}

__asm__(".symver add_v1, add@Version1.0");
__asm__(".symver add_v2, add@@Version2.0");
