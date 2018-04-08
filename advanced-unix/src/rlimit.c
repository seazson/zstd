#include <unistd.h>
#include <stdio.h>
#include <sys/resource.h>

#define doit(name) print_limits(#name, name)
static void print_limits(char *name, int resource)
{
	struct rlimit limit;
	if(getrlimit(resource, &limit) < 0)
		printf("get limits error\n");

	printf("%-14s  ",name);

	if(limit.rlim_cur == RLIM_INFINITY)
		printf("(infinity)  ");
	else
		printf("%10ld  ", limit.rlim_cur);

	if(limit.rlim_max == RLIM_INFINITY)
        printf("(infinity)  ");
    else
	    printf("%10ld  ", limit.rlim_max);

	putchar('\n');	
}

int show_rlimit(void)
{
	printf("*********************************\n");
	doit(RLIMIT_AS);
	doit(RLIMIT_CORE);
	doit(RLIMIT_CPU);
	doit(RLIMIT_DATA);
	doit(RLIMIT_FSIZE);
	doit(RLIMIT_LOCKS);
	doit(RLIMIT_MEMLOCK);
	doit(RLIMIT_NOFILE);
	doit(RLIMIT_NPROC);
	doit(RLIMIT_RSS);
	doit(RLIMIT_STACK);
//	doit(RLIMIT_SBSIZE);
//	doit(RLIMIT_VMEM);
	return 0;
}

int show_usage(void)
{
	struct rusage r_usage;
	printf("prio= %d\n",getpriority(PRIO_PROCESS,getpid()));
	getrusage(RUSAGE_SELF, &r_usage);
	printf("CPU usage: Use = %ld.%06ld, System = %ld.%06ld\n",
				r_usage.ru_utime.tv_sec,r_usage.ru_utime.tv_usec,
				r_usage.ru_stime.tv_sec,r_usage.ru_stime.tv_usec);	
}
