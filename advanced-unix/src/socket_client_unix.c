#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int client_unix(void)
{
	int sockfd;
	int len;
	struct sockaddr_un address;
	int result;
	int ch = 'A';

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "server_socket");
	len = sizeof(address);
	result = connect(sockfd, (struct sockaddr *)&address, len);

	write(sockfd, &ch, 1);
	read(sockfd, &ch, 1);
	printf("char form server = %c\n",ch);
	
	close(sockfd);
	return 0;	
}
