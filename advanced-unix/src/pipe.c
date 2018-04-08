#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>	

#define MAXLINE 4096

int pipe_create(void)
{
	int fd[2];
	int n;
	pid_t pid;
	char line[MAXLINE];

	pipe(fd);
	printf("create pipe ok\n");

	if((pid = fork()) < 0)
		printf("fork err\n");
	else if( pid >0)
	{
//		close(fd[0]);
		sleep(2);
		write(fd[1],"father write to child\n",22);

		sleep(1);   //�������sleep��֤�ӽ���д�˲��ܶ���
								//��Ϊ�ܵ��ǵ������д�ֶ���������������Ҫ�ֶ�����ö�д˳���������ǿ���ʵ��˫���
		n = read(fd[0],line, MAXLINE);
		if(n > 0)write(STDOUT_FILENO, line, n);
		printf("read ok %d\n",n);
		wait();
	}
	else
	{
//		close(fd[1]);
		n = read(fd[0], line, MAXLINE);
		write(STDOUT_FILENO, line, n);
	
		write(fd[1],"child write to father!!!\n",25);
		printf("write ok %d\n",n);
		exit(0);
	}
	return 0;
	
}
