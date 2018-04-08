#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */

#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<netdb.h>       /* addrinfo*/

#include	<netinet/udp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define	BUFSIZE		1500

struct rec {					/* format of outgoing UDP data */
  u_short	rec_seq;			/* sequence number */
  u_short	rec_ttl;			/* TTL packet left with */
  struct timeval	rec_tv;		/* time packet left */
};

			/* globals */
char	 recvbuf[BUFSIZE];
char	 sendbuf[BUFSIZE];

int		 datalen;			/* # bytes of data following ICMP header */
char	*host;
u_short	 sport, dport;
int		 nsent;				/* add 1 for each sendto() */
pid_t	 pid;				/* our PID */
int		 probe, nprobes;
int		 sendfd, recvfd;	/* send on UDP sock, read on raw ICMP sock */
int		 ttl, max_ttl;
int		 verbose;
int 	 gotalarm;

struct proto {
  const char	*(*icmpcode)(int);
  int	 (*recv)(int, struct timeval *);
  struct sockaddr  *sasend;	/* sockaddr{} for send, from getaddrinfo */
  struct sockaddr  *sarecv;	/* sockaddr{} for receiving */
  struct sockaddr  *salast;	/* last sockaddr{} for receiving */
  struct sockaddr  *sabind;	/* sockaddr{} for binding source port */
  socklen_t   		salen;	/* length of sockaddr{}s */
  int			icmpproto;	/* IPPROTO_xxx value for ICMP */
  int	   ttllevel;		/* setsockopt() level to set TTL */
  int	   ttloptname;		/* setsockopt() name to set TTL */
} *pr;

/************************************signal*********************************/
typedef	void	Sigfunc(int);	/* for signal handlers */
Sigfunc * signal(int signo, Sigfunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} else {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
}
/* end signal */

Sigfunc *Signal(int signo, Sigfunc *func)	/* for our signal() function */
{
	Sigfunc	*sigfunc;

	if ( (sigfunc = signal(signo, func)) == SIG_ERR)
		printf("signal error");
	return(sigfunc);
}

void sig_alrm(int signo)
{
	gotalarm = 1;	/* set flag to note that alarm occurred */
	return;			/* and interrupt the recvfrom() */
}
/************************************************************************************/
uint16_t
in_cksum(uint16_t *addr, int len)
{
	int				nleft = len;
	uint32_t		sum = 0;
	uint16_t		*w = addr;
	uint16_t		answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

		/* 4mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w ;
		sum += answer;
	}

		/* 4add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

void tv_sub(struct timeval *out, struct timeval *in)
{
	if ( (out->tv_usec -= in->tv_usec) < 0) {	/* out -= in */
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

char * Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}

	default:
		snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}


const char * icmpcode_v4(int code)
{
	static char errbuf[100];
	switch (code) {
	case  0:	return("network unreachable");
	case  1:	return("host unreachable");
	case  2:	return("protocol unreachable");
	case  3:	return("port unreachable");
	case  4:	return("fragmentation required but DF bit set");
	case  5:	return("source route failed");
	case  6:	return("destination network unknown");
	case  7:	return("destination host unknown");
	case  8:	return("source host isolated (obsolete)");
	case  9:	return("destination network administratively prohibited");
	case 10:	return("destination host administratively prohibited");
	case 11:	return("network unreachable for TOS");
	case 12:	return("host unreachable for TOS");
	case 13:	return("communication administratively prohibited by filtering");
	case 14:	return("host recedence violation");
	case 15:	return("precedence cutoff in effect");
	default:	sprintf(errbuf, "[unknown code %d]", code);
				return errbuf;
	}
}

int recv_v4(int seq, struct timeval *tv)
{
	int				hlen1, hlen2, icmplen, ret;
	socklen_t		len;
	ssize_t			n;
	struct ip		*ip, *hip;
	struct icmp		*icmp;
	struct udphdr	*udp;

	gotalarm = 0;
	alarm(3);
	for ( ; ; ) {
		if (gotalarm)
			return(-3);		/* alarm expired */
		len = pr->salen;
		n = recvfrom(recvfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			else
				printf("recvfrom error");
		}

		ip = (struct ip *) recvbuf;	/* start of IP header */
		hlen1 = ip->ip_hl << 2;		/* length of IP header */
	
		icmp = (struct icmp *) (recvbuf + hlen1); /* start of ICMP header */
		if ( (icmplen = n - hlen1) < 8)
			continue;				/* not enough to look at ICMP header */
	
		if (icmp->icmp_type == ICMP_TIMXCEED &&
			icmp->icmp_code == ICMP_TIMXCEED_INTRANS) {
			if (icmplen < 8 + sizeof(struct ip))
				continue;			/* not enough data to look at inner IP */

			hip = (struct ip *) (recvbuf + hlen1 + 8);
			hlen2 = hip->ip_hl << 2;
			if (icmplen < 8 + hlen2 + 4)
				continue;			/* not enough data to look at UDP ports */

			udp = (struct udphdr *) (recvbuf + hlen1 + 8 + hlen2);
 			if (hip->ip_p == IPPROTO_UDP &&
				udp->source == htons(sport) &&
				udp->dest == htons(dport + seq)) {
				ret = -2;		/* we hit an intermediate router */
				break;
			}

		} else if (icmp->icmp_type == ICMP_UNREACH) {
			if (icmplen < 8 + sizeof(struct ip))
				continue;			/* not enough data to look at inner IP */

			hip = (struct ip *) (recvbuf + hlen1 + 8);
			hlen2 = hip->ip_hl << 2;
			if (icmplen < 8 + hlen2 + 4)
				continue;			/* not enough data to look at UDP ports */

			udp = (struct udphdr *) (recvbuf + hlen1 + 8 + hlen2);
 			if (hip->ip_p == IPPROTO_UDP &&
				udp->source == htons(sport) &&
				udp->dest == htons(dport + seq)) {
				if (icmp->icmp_code == ICMP_UNREACH_PORT)
					ret = -1;	/* have reached destination */
				else
					ret = icmp->icmp_code;	/* 0, 1, 2, ... */
				break;
			}
		}
		if (verbose) {
			printf(" (from %s: type = %d, code = %d)\n",
					Sock_ntop_host(pr->sarecv, pr->salen),
					icmp->icmp_type, icmp->icmp_code);
		}
		/* Some other ICMP error, recvfrom() again */
	}
	alarm(0);					/* don't leave alarm running */
	gettimeofday(tv, NULL);		/* get time of packet arrival */
	return(ret);
}

void traceloop(void)
{
	int					seq, code, done;
	double				rtt;
	struct rec			*rec;
	struct timeval		tvrecv;

	recvfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
	if(recvfd < 0)
	{
		printf("Create socket error %d\nneed root premission\n",recvfd);
		return;
	}	
	setuid(getuid());		/* don't need special permissions anymore */

	sendfd = socket(pr->sasend->sa_family, SOCK_DGRAM, 0);

	pr->sabind->sa_family = pr->sasend->sa_family;
	sport = (getpid() & 0xffff) | 0x8000;	/* our source UDP port # */
	((struct sockaddr_in *) pr->sabind)->sin_port = htons(sport);
	bind(sendfd, pr->sabind, pr->salen);

	sig_alrm(SIGALRM);

	seq = 0;
	done = 0;
	for (ttl = 1; ttl <= max_ttl && done == 0; ttl++) {
		setsockopt(sendfd, pr->ttllevel, pr->ttloptname, &ttl, sizeof(int));
		bzero(pr->salast, pr->salen);

		printf("%2d ", ttl);
		fflush(stdout);

		for (probe = 0; probe < nprobes; probe++) {
			rec = (struct rec *) sendbuf;
			rec->rec_seq = ++seq;
			rec->rec_ttl = ttl;
			gettimeofday(&rec->rec_tv, NULL);

			((struct sockaddr_in *) pr->sasend)->sin_port = htons(dport + seq);			
			sendto(sendfd, sendbuf, datalen, 0, pr->sasend, pr->salen);

			if ( (code = (*pr->recv)(seq, &tvrecv)) == -3)
				printf(" *");		/* timeout, no reply */
			else {
				char	str[NI_MAXHOST];

				if (memcmp(&((struct sockaddr_in *)pr->sarecv)->sin_addr, &((struct sockaddr_in *)pr->salast)->sin_addr, pr->salen) != 0) {
					if (getnameinfo(pr->sarecv, pr->salen, str, sizeof(str),
									NULL, 0, 0) == 0)
						printf(" %s (%s)", str,
								Sock_ntop_host(pr->sarecv, pr->salen));
					else
						printf(" %s",
								Sock_ntop_host(pr->sarecv, pr->salen));
					memcpy(pr->salast, pr->sarecv, pr->salen);
				}
				tv_sub(&tvrecv, &rec->rec_tv);
				rtt = tvrecv.tv_sec * 1000.0 + tvrecv.tv_usec / 1000.0;
				printf("  %.3f ms", rtt);

				if (code == -1)		/* port unreachable; at destination */
					done++;
				else if (code >= 0)
					printf(" (ICMP %s)", (*pr->icmpcode)(code));
			}
			fflush(stdout);
		}
		printf("\n");
	}
}

/*********************************************************************************/
struct proto	proto_v4 = { icmpcode_v4, recv_v4, NULL, NULL, NULL, NULL, 0,
							 IPPROTO_ICMP, IPPROTO_IP, IP_TTL };

int		datalen = sizeof(struct rec);	/* defaults */
int		max_ttl = 30;
int		nprobes = 3;
u_short	dport = 32768 + 666;

int main(int argc, char ** argv)
{
	int				c;
	struct addrinfo	*ai;
	char *h;

	host = argv[1];

	pid = getpid();
	Signal(SIGALRM, sig_alrm);

	int				n;
	struct addrinfo	hints;
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;	/* always return canonical name */
	hints.ai_family = 0;		/* 0, AF_INET, AF_INET6, etc. */
	hints.ai_socktype = 0;	/* 0, SOCK_STREAM, SOCK_DGRAM, etc. */
	if ( (n = getaddrinfo(host, NULL, &hints, &ai)) != 0)
		printf("host_serv error for %s, %s: %d",
				 (host == NULL) ? "(no hostname)" : host,
				 "(no service name)",
				 n);
	
    static char str[128];		/* Unix domain is largest */
	struct sockaddr_in	*sin = (struct sockaddr_in *) ai->ai_addr;
	inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str));
	h = str;
	printf("traceroute to %s (%s): %d hops max, %d data bytes\n",
		   ai->ai_canonname ? ai->ai_canonname : h,
		   h, max_ttl, datalen);

		   /* initialize according to protocol */
	if (ai->ai_family == AF_INET) {
		pr = &proto_v4;
	} else
		printf("unknown address family %d", ai->ai_family);

	pr->sasend = ai->ai_addr;		/* contains destination address */
	pr->sarecv = calloc(1, ai->ai_addrlen);
	pr->salast = calloc(1, ai->ai_addrlen);
	pr->sabind = calloc(1, ai->ai_addrlen);
	pr->salen = ai->ai_addrlen;

	traceloop();

	exit(0);
}

