#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

int show_host(void)
{
	struct addrinfo *ailist, *aip;
	struct addrinfo hint;
	struct sockaddr_in *sinp;
	const char *addr;
	int err;
	char abuf[INET_ADDRSTRLEN];

	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = 0;
	hint.ai_socktype = 0;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

	return 0;
}

int show_hostinfo(void)
{
	char *host, **names, **addrs;
	struct hostent *hostinfo;
	char myname[256];
	
	gethostname(myname,255);
	host = myname;
	
	hostinfo = gethostbyname(host);
	
	printf("host = %s\n",host);
	printf("Name : %s\n",hostinfo->h_name);
	printf("Aliases:\n");	
	names = hostinfo->h_aliases;
	while(*names)
	{
		printf("  %s\n",*names);
		names++;	
	}
	
	if(hostinfo->h_addrtype != AF_INET)
	{
		printf("not a ip host\n");	
		return 1;
	}
	
	addrs = hostinfo->h_addr_list;
	while(*addrs)
	{
		printf("  %s\n", inet_ntoa(*(struct in_addr *)*addrs));	
		addrs++;
	}
	
	return 0;
}
