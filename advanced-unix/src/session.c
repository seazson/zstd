#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

int set_pgid(void)
{
	setpgid(getpid(),getpid());
	printf("set %d as pgid\n",getpid());
	return 0;
}

int set_sid(void)
{
	pid_t pid;
	pid = setsid();
	printf("session pid = %d\n",pid);
	return 0;
}
