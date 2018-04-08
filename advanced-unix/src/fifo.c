#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

char line[4096];

int fifo_create(void)
{
	mkfifo("fsea",S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	return 0;
}

int fifo_read(void)
{
	int fd;
	int n;
	fd = open("fsea",O_RDWR);
	n = read(fd,line,4096);
	if(n > 0)
	write(STDOUT_FILENO, line, n);
	printf("read %d ok\n",n);
	return 0;
}

int fifo_write(void)
{
	int fd;
	int n;
	fd = open("fsea", O_RDWR);
	n = write(fd,"this is write to fifo\n",22);
	printf("write %d ok",n);
	return 0;
}
