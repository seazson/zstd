#include <stdio.h>
#include <unistd.h>

extern char **cmd_argv;
extern char *cmd_buf;
//extern char cmd_buf[];

struct tag_t{
	unsigned int p1 : 1;
	unsigned int p2 : 1;
	unsigned int p3 : 6;
	unsigned int p4 : 8;
	unsigned int p5 : 10;
	unsigned int p6 : 1;
	unsigned int p7;
};

void show_size(char a[12][25])
{
	printf("size= %d\n",sizeof(a));
}

int c_adv(void)
{
	int a,b,c;
	const int i=1;
	int const j=1;
	struct tag_t tag; 
	char b20[20][25];
/*出错
	i++;
	j++;
*/
	const int *p1=&a;
	int const *p2=&b;
	int * const p3 = &c;

//出错	(*p1)++;
//出错	(*p2)++;
	(*p3)++;

	p1++;
	p2++;
//出错	p3++;


	printf("tag =%x\n",&tag);
/*	printf("p1 = %x\n",&(tag.p1));
    printf("p2 = %x\n",&(tag.p2));
    printf("p3 = %x\n",&(tag.p3));
    printf("p4 = %x\n",&(tag.p4));
    printf("p5 = %x\n",&(tag.p5));
    printf("p6 = %x\n",&(tag.p6));*/
    printf("p7 = %x\n",&(tag.p7));
	
	printf("%x\n",&cmd_buf);

	printf("sizeb= %d\n",sizeof(b20));
	show_size(b20);
	return 0;
}
