#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>       /*包含ip4地址定义*/

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;
	int n;
	socklen_t len;
	char sendline[1600];
	char recvline[1600];
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9917);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	len = sizeof(servaddr);
	
	while(fgets(sendline, 1500, stdin)!=NULL)
	{
		sendto(sockfd, sendline, strlen(sendline), 0, &servaddr, len);
		n = recvfrom(sockfd, recvline, 1600, 0, NULL, NULL);
		recvline[n] = 0;
		fputs(recvline,stdout);
	}
}
