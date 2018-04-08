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

	sockfd=socket(AF_INET, SOCK_STREAM, 0);

	if(bind(sockfd, (struct sockaddr*)server, sizeof(*server)))
	{
		printf("bind failed\n");
	}

	listen(sockfd, SOMAXCONN);

	int clientfd;
	struct sockaddr_in * client = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	int client_size = sizeof(*client);

	char *buf=(char *)malloc(1000);
	int bytes;

	printf("Wait for connection to port %u\n", echo_port);

	clientfd=accept(sockfd, (struct sockaddr*)client, &client_size);
	printf("Connected to %s:%u\n\n",inet_ntoa(client->sin_addr),ntohs(client->sin_port));
	printf("Numeric: %u\n",ntohl(client->sin_addr.s_addr));

	while(1)
	{
		bytes = read(clientfd,(void*)buf, 1000);
		if(bytes <= 0)
		{
			close(clientfd);
			printf("Connection closed.\n");
			exit(0);
		}
		printf("Bytes reveived : %u\n", bytes);
		printf("Text : '%s'\n",buf);

		write(clientfd,buf,bytes);
	}
}
