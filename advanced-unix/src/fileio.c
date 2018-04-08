#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int g_fd;
extern char *cmd_argv[10];
extern int cmd_argc;
int Open(void)
{
	g_fd = open(cmd_argv[1], O_RDWR | O_CREAT, 0x777);
	if(g_fd < 0)
	{
		printf("open %s err\n",cmd_argv[1]);
		return 1;
	}
	printf("open %s ok\n", cmd_argv[1]);
	return 0;
}

int Close(void)
{
	close(g_fd);
	printf("close \n");
	return 0;  
}

int Lseek(void)
{
	off_t old_pt = lseek(g_fd, 0, SEEK_CUR);
	printf("postion at 0x%lx\n",old_pt);
	if(cmd_argc == 2)
	{
		printf("now move to 0x%x\n", atoi(cmd_argv[1]));
		lseek(g_fd,atoi(cmd_argv[1]), SEEK_SET);
	}
	return 0;
}


int Read(void)
{
	int len,i;
	char buf[4096];
	if(cmd_argc == 2)
	{
		size_t n = atoi(cmd_argv[1]);
		char *p = (char *)malloc(n);
		len = read(g_fd, p, n);
		printf("read len = %d : %s\n",len,p);
	}
	else
	{
		len = read(g_fd, buf, 4096);
		while(len !=0)
		{
			printf("read %d:\n",len);
			for(i = 0; i < len; i++)
			{
				printf("%c",buf[i]);	
			}
			len = read(g_fd, buf, 4096);
		}	
	}
	printf("\n");
	return 0;
}

int Write(void)
{
	write(g_fd, cmd_argv[1], strlen(cmd_argv[1])+1);
	printf("write %s\n",cmd_argv[1]);
	return 0;
}

int Fcntl(void)
{
	int val;
	val = fcntl(g_fd, F_GETFL, 0);
	
	switch(val & O_ACCMODE)
	{
		case O_RDONLY:
			printf("read only");
			break;
		case O_WRONLY:
			printf("write only");
			break;
		case O_RDWR:
			printf("read write");
			break;
		default:
			printf("unknow mode");
			break;	
	}
	
	if(val & O_APPEND)
		printf(", append");
	if(val & O_NONBLOCK)
		printf(", nonblock");
	if(val & O_SYNC)
		printf(", sync");
	if(val & O_FSYNC)
		printf(", fsync");
	putchar('\n');
	
	return 0;	
}



