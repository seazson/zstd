#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>       /*包含ip4地址定义*/
#include <errno.h>

void str_echo(int sockfd)
{
	ssize_t n;
	char buf[1024];

again:
	while((n = read(sockfd, buf, 1024)) > 0)
		write(sockfd, buf, n);
	
	if(n < 0 && errno == EINTR)
		goto again;
	else if(n < 0)
		printf("read error\n");
}

int main(int argc, char **argv)
{
	int listenfd; /*监听套接字*/
	int connfd;   /*已连接套接字*/
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	char buff[100];
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(9917);
	bind(listenfd, (struct sockaddr *)&(servaddr), sizeof(servaddr));
	
	listen(listenfd,5);
	
	while(1)
	{
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		printf("Connect from %s,port %d\n",inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));
		if((childpid = fork()) == 0)
		{/*means child*/
			close(listenfd);
			str_echo(connfd);
			exit(0);
		}
		close(connfd);
	}
	
	return 0;
}
