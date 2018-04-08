#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>       /*包含ip4地址定义*/

#include <net/if.h>
#include <sys/param.h>
//#define HAVE_MSGHDR_MSG_CONTROL /*linux不支持*/
struct unp_in_pktinfo{
	struct in_addr ipi_addr;
	int ipi_ifindex;
};

ssize_t recvform_flags(int fd, void *ptr, size_t nbytes, int *flagsp, 
                       struct sockaddr *sa, socklen_t *salenptr, struct unp_in_pktinfo *pktp)
{
	struct msghdr msg;
	struct iovec iov[1];
	ssize_t n;

#ifdef HAVE_MSGHDR_MSG_CONTROL	
	struct cmsghdr *cmptr;
	union{
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(struct in_addr)) + CMSG_SPACE(sizeof(struct unp_in_pktinfo))];
	}control_un;
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_flags = 0;
#else
	bzero(&msg, sizeof(msg));
#endif	
	msg.msg_name = sa;
	msg.msg_namelen = *salenptr;
	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iov = 1;
	
	if((n = recvmsg(fd, &msg, *flagsp)) < 0)
		return n;
	
	*salenptr = msg.msg_namelen;
	if(pktp)
		bzero(pktp, sizeof(struct unp_in_pktinfo));
#ifndef HAVE_MSGHDR_MSG_CONTROL
	*flagsp = 0;
	return n;
#else
	*flagsp = msg.msg_flags;
	if(msg.msg_controllen < sizeof(struct cmsghdr) || 
	  (msg.msg_flags & MSG_CTRUNC) || pktp == NULL)
	  return n;

	for(cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL; cmptr = CMSG_NXTHDR(&msg, cmptr))
	{
#ifdef IP_RECVDSTADDR
		if(cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVDSTADDR)
		{
			memcpy(&pktp->ipi_addr, CMSG_DATA(cmptr), sizeof(struct in_addr));
			continue;
		}
#endif

#ifdef IP_RECVIF
		if(cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVIF)
		{
			struct sockaddr_dl *sdl;
			sdl = (struct sockaddr_dl *)CMSG_DATA(cmptr);
			pktp->ipi_ifindex = sdl->sdl_index;
			continue;
		}
#endif
		printf("unknow ancillary data, len=%d, level=%d, type=%d", cmptr->cmsg_len, cmptr->cmsg_level, cmptr->cmsg_type);
	}
	return n;
#endif
}

sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char		portstr[8];
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		if (ntohs(sin->sin_port) != 0) {
			snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
			strcat(str, portstr);
		}
		return(str);
	}

	default:
		snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}

#define MAXLINE 20
void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int flags;
	const int on =1;
	socklen_t len;
	ssize_t n;
	char mesg[MAXLINE], str[INET6_ADDRSTRLEN],ifname[IFNAMSIZ];
	struct in_addr in_zero;
	struct unp_in_pktinfo pktinfo;
	
#ifdef IP_RECVDSTADDR
	if(setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR), &on, sizeof(on)) < 0)
		printf("seterror iprecvdstaddr\n");
#endif
#ifdef IP_RECVIF
	if(setsockopt(sockfd, IPPROTO_IP, IP_RECVIF), &on, sizeof(on)) < 0)
		printf("seterror iprecvif\n");
#endif
	bzero(&in_zero, sizeof(struct in_addr));
	
	for(;;){
		len = clilen;
		flags = 0;
		n =recvform_flags(sockfd, mesg, MAXLINE, &flags, pcliaddr, &len, &pktinfo);
		printf("%d-byte datagram from %s",n,sock_ntop(pcliaddr,len));
		if(memcmp(&pktinfo.ipi_addr, &in_zero, sizeof(in_zero))!=0)
			printf(", to %s", inet_ntop(AF_INET, &pktinfo.ipi_addr, str, sizeof(str)));
		
		if(pktinfo.ipi_ifindex > 0)
			printf(",recv i/f=%s", if_indextoname(pktinfo.ipi_ifindex, ifname));

		if(flags & MSG_TRUNC)
				printf(" (datagram truncated)");
		if(flags & MSG_CTRUNC)
				printf(" (control info truncated)");
#ifdef MSG_BCAST
		if(flags & MSG_BCAST)
				printf(" (broadcast)");
#endif
#ifdef MSG_MCAST
		if(flags & MSG_MCAST)
				printf(" (multicast)");
#endif
		printf("\n");
		sendto(sockfd,mesg,n,0,pcliaddr,len);
	}

}

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
/*获取接收标志，索引。linux不支持*/
//	dg_echo(sockfd,(struct sockaddr*)&cliaddr,sizeof(cliaddr));
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
