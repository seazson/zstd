#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>	
#include "sys/times.h"

static int val = 0;
static char *env_init[] = {"DD=BIG","PATH=./",NULL};
extern char *cmd_argv[10];
extern char **environ;
static void print_times(clock_t real, struct tms *tmsstart, struct tms *tmsend)
{
	static long clktck = 0;
	clktck = sysconf(_SC_CLK_TCK);

	printf("HZ : %d\n",clktck);
	printf("  real:  %7.2f\n", real / (double) clktck);
	printf("  user:  %7.2f\n",
		  (tmsend->tms_utime - tmsstart->tms_utime) / (double) clktck);
	printf("  sys:   %7.2f\n",
		  (tmsend->tms_stime - tmsstart->tms_stime) / (double) clktck);
	printf("  child user:  %7.2f\n",
		  (tmsend->tms_cutime - tmsstart->tms_cutime) / (double) clktck);
	printf("  child sys:   %7.2f\n",
		  (tmsend->tms_cstime - tmsstart->tms_cstime) / (double) clktck);
}

int show_ids()
{
	printf("user:%s\n",getlogin());
	printf("pid = %d\n",getpid());
	printf("ppid= %d\n",getppid());
	printf("pgid= %d\n",getpgrp());
	printf("uid = %d\n",getuid());
	printf("euid= %d\n",geteuid());
	printf("gid = %d\n",getgid());
	printf("egid= %d\n",getegid());
	return 0;							
}

int fork_e()
{
	pid_t pid;
	struct tms tmsstart,tmsend;
	clock_t start,end;
	
	start = times(&tmsstart);
	if((pid = fork()) < 0)
	{
		printf("fork err\n");
	}
	else if(pid == 0)
	{
		val++;
	  printf("child pid = %d\n",getpid());
		printf("child ppid= %d\n",getppid());
		printf("child val=%d\n",val);
	}
	else
	{
		printf("fchild pid=%d\n",pid);
		waitpid(pid,NULL,0);
		printf("fchild %d exit val=%d\n",pid,val);
		end = times(&tmsend);
		print_times(end-start, &tmsstart, &tmsend);
	}
	return 0;
}

int vfork_e()
{
	pid_t pid;
    if((pid = vfork()) < 0)
    {
        printf("fork err\n");
    }
    else if(pid == 0)
    {
        val++;
        printf("child pid = %d\n",getpid());
        printf("child ppid= %d\n",getppid());
        printf("child val=%d\n",val);
    }
    else
    {
        printf("child pid=%d\n",pid);
        waitpid(pid,NULL,0);
        printf("child %d exit val=%d\n",pid,val);
    }
	return 0;
}

int execle_e()
{
	printf("%s\n",cmd_argv[1]);
	execle("/bin/sh","sh","-c",cmd_argv[1],(char*)0,environ);	
	printf("execle quit\n");
	return 0;
}

int system_e()
{
	pid_t pid;
	int status;
	
	if(cmd_argv[1] == NULL)
		return 1;
  
  if((pid = fork()) < 0)
  {
  	printf("fork error\n");	
  }
  else if(pid == 0)
  {
  	execl("/bin/sh", "sh", "-c", cmd_argv[1], (char *)0);
  	_exit(127);	
  }
  else
  {
  	waitpid(pid, NULL, 0)	;
  }
  return 0;	
}

