#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>

int main()
{
	char * echo_host = "192.168.10.30";
	int echo_port = 7777;
	int sockfd;
	struct sockaddr_in *server=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));

	server->sin_family = AF_INET;
	server->sin_port = htons(echo_port);
	server->sin_addr.s_addr = inet_addr(echo_host);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	printf("Connecting to %s \n", echo_host);
	printf("Numeric : %u\n",server->sin_addr);

	connect(sockfd, (struct sockaddr*)server, sizeof(*server));

	char* msg = "hello world";
	printf("\nSend: '%s'\n", msg);
	write(sockfd, msg, strlen(msg));

	char *buf = (char *)malloc(1000);
	int bytes = read(sockfd, (void*)buf, 1000);
	printf("\nBytes received: %u\n",bytes);
	printf("Text: '%s'\n",buf);

	close(sockfd);
}
