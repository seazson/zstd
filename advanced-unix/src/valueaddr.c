#include <unistd.h>
#include <stdio.h>

int g_val=10;
int g_val0;
extern char **environ;
extern int cmd_argc;

int show_valueaddr()
{
	int i;
	char *p;
	printf("***************mem map**************\n");
	p = malloc(10);
	printf("stack addr: %x\n", &i);
	printf("arg   addr: %x\n", &cmd_argc);
	printf("init  data: %x\n", &g_val);
	printf("bss   data: %x\n", &g_val0);
	printf("env   addr: %x\n", &environ);
	printf("alloc addr: %x\n", p);
	printf("text  addr: %x\n", show_valueaddr);
	free(p);
}
