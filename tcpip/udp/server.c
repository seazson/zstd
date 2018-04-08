#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>       /*包含ip4地址定义*/

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	int n;
	socklen_t len;
	char mesg[1600];
	char buff[100];
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(9917);
	
	bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

#if 1	
	for(;;)
	{
		len = sizeof(cliaddr);
		n = recvfrom(sockfd, mesg, 1500, 0, &cliaddr, &len);
		printf("Connect from %s,port %d\n",inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));
		sendto(sockfd, mesg, n, 0, &cliaddr, len);
	}
#endif
}
