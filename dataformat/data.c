#include <stdio.h>

union dd {
	char c[4];
	int a;
	unsigned int b;
	float f;
}da;

int main()
{
	da.a = 100;
	printf("%x %x %x %x\n",da.c[0],da.c[1],da.c[2],da.c[3]);
    
	da.b = -100;
	printf("%x %x %x %x\n",da.c[0],da.c[1],da.c[2],da.c[3]);
	
    da.f = -1.0;
    printf("%x %x %x %x\n",da.c[0],da.c[1],da.c[2],da.c[3]);

	da.b = -100;
	da.a = da.b;
	printf("%x %x %x %x\n",da.c[0],da.c[1],da.c[2],da.c[3]);

    da.a = 0x80000001;
    da.b = da.a;
    printf("%x %x %x %x\n",da.c[0],da.c[1],da.c[2],da.c[3]);	
	
	return 0;
}
