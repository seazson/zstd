#include <stdio.h>
#include <unistd.h>

extern char *cmd_argv[10];
extern char **environ;

int show_env(void)
{
	int i=0;
	for(i = 0; environ[i] != NULL; i++)
		printf("%s\n",environ[i]);
}

int set_env(void)
{
	printf("set %s = %s\n",cmd_argv[1], cmd_argv[2]);
	setenv(cmd_argv[1],cmd_argv[2],1);
}

int get_env(void)
{
	printf("%s = %s\n",cmd_argv[1],getenv(cmd_argv[1]));
}
