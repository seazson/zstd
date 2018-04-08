#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>

#define LOCKFILE "./sea.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

extern int lockfile(int fd);

int one_daemon()
{
/*确保只有一个实例*/
	int fd;
	char buf[16];
	fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
	if(fd < 0)
	{
		printf("can not open %s\n",LOCKFILE);
		return 1;	
	}
	if(lockfile(fd) < 0)
	{
		if(errno == EACCES || errno == EAGAIN)
		{
			printf("can not lockfile\n");
			close(fd);
			return 1;	
		}
		printf("can not lockfile\n");
		return 1;
	}
	ftruncate(fd,0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf)+1);
	return 0;
	
}

int daemon_e()
{
	int i, fd0,fd1,fd2;
	pid_t pid;
	struct rlimit rl;
	struct sigaction sa;
/*step 1*/
	umask(0);

	getrlimit(RLIMIT_NOFILE, &rl);
	
/*step 2*/	
	if((pid=fork()) < 0)
		printf("fork err \n");
	else if(pid != 0)
		exit(0);

/*step 3*/	
	setsid(); /*这个时候虽然没有控制终端，但是文件描述符还在，还可以输出*/
	printf("this is daemon\n");	

/*在创建一次，确保没有控制终端*/	
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGHUP, &sa, NULL);
	if((pid=fork()) < 0)
		printf("fork err \n");
	else if(pid != 0)
		exit(0);
	
	if(one_daemon() == 1)
		return 1;

/*step 4*/
	chdir("/");
	
/*step 5*/
	if(rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for(i = 0; i < rl.rlim_max; i++)
	{
		close(i);	
	}

/*step 6*/
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	openlog("sea",LOG_CONS, LOG_DAEMON);
	
	i=0;
	while(1)
	{
		syslog(LOG_ERR, "%d ***********pid = %d*************",i++,getpid());
		sleep(10);	
	}
	
	return 0;
}


