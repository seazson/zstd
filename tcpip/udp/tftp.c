#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>       /*包含ip4地址定义*/
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/stat.h>

/*********************************tftp 格式******************************************
*read/write request:
*      20B       8B           2B              NB     1B          8/5B             1B
*   | iphead | udphead | opcode(1:r 2:w) | filename | 0 | mode(netascii/octet) | 0 |
*data pakage:          |                 |
*                             2B              2B                 0~512B
*   | iphead | udphead | opcode(3:data)  | blocknum |             data             |
*ack pakage:           |                 |
*                             2B              2B	         
*   | iphead | udphead | opcode(4:ack)   | blocknum |
*err pakage:           |                 |
*                             2B              2B                   NB
*   | iphead | udphead | opcode(5:error) | errorcode|       error infromation      |
*************************************************************************************/
/******************RTT算法部分*******************************************************/
#define RTT_RXTMIN    2
#define RTT_RXTMAX    60
#define RTT_MAXNREXMT 3
struct rtt_info{
	float rtt_rtt;
	float rtt_srtt;
	float rtt_rttvar;
	float rtt_rto;
	int rtt_nrexmt;
	uint32_t rtt_base;
};

int	rtt_d_flag = 0;
#define	RTT_RTOCALC(ptr) ((ptr)->rtt_srtt + (4.0 * (ptr)->rtt_rttvar))

static float rtt_minmax(float rto)
{
	if (rto < RTT_RXTMIN)
		rto = RTT_RXTMIN;
	else if (rto > RTT_RXTMAX)
		rto = RTT_RXTMAX;
	return(rto);
}

void rtt_init(struct rtt_info *ptr)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	ptr->rtt_base = tv.tv_sec;		/* # sec since 1/1/1970 at start */

	ptr->rtt_rtt    = 0;
	ptr->rtt_srtt   = 0;
	ptr->rtt_rttvar = 0.75;
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
		/* first RTO at (srtt + (4 * rttvar)) = 3 seconds */
}

uint32_t rtt_ts(struct rtt_info *ptr)
{
	uint32_t		ts;
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	ts = ((tv.tv_sec - ptr->rtt_base) * 1000) + (tv.tv_usec / 1000);
	return(ts);
}

void rtt_newpack(struct rtt_info *ptr)
{
	ptr->rtt_nrexmt = 0;
}

int rtt_start(struct rtt_info *ptr)
{
	return((int) (ptr->rtt_rto + 0.5));		/* round float to int */
		/* 4return value can be used as: alarm(rtt_start(&foo)) */
}

void rtt_stop(struct rtt_info *ptr, uint32_t ms)
{
	double		delta;

	ptr->rtt_rtt = ms / 1000.0;		/* measured RTT in seconds */

	/*
	 * Update our estimators of RTT and mean deviation of RTT.
	 * See Jacobson's SIGCOMM '88 paper, Appendix A, for the details.
	 * We use floating point here for simplicity.
	 */

	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta / 8;		/* g = 1/8 */

	if (delta < 0.0)
		delta = -delta;				/* |delta| */

	ptr->rtt_rttvar += (delta - ptr->rtt_rttvar) / 4;	/* h = 1/4 */

	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}

int rtt_timeout(struct rtt_info *ptr)
{
	ptr->rtt_rto *= 2;		/* next RTO */

	if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
		return(-1);			/* time to give up for this packet */
	return(0);
}

void rtt_debug(struct rtt_info *ptr)
{
	if (rtt_d_flag == 0)
		return;

	fprintf(stderr, "rtt = %.3f, srtt = %.3f, rttvar = %.3f, rto = %.3f\n",
			ptr->rtt_rtt, ptr->rtt_srtt, ptr->rtt_rttvar, ptr->rtt_rto);
	fflush(stderr);
}

/**************************************************************************/
static struct rtt_info rttinfo;
static int rttinit = 0;
static struct msghdr msgsend, msgrecv;
static struct hdr{
	uint16_t opcode;
	uint16_t num;
} sendhdr,recvhdr;

/*信号处理部分*/
static sigjmp_buf jmpbuf;
static void sig_alrm(int signo)
{
	siglongjmp(jmpbuf, 1);
}

unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;	
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}
/********************************客户端上传*********************************/
/*     1 客户端向69端口发送write命令(opcode=2)
*      2 服务器回复block=0的ack(opcode=4),同时回复服务器的端口号x
*      3 客户端向端口号x发送block=1,2,3...的数据包，大小512(opcode=3)
*      4 服务器回复block=1,2,3....的ack(opcode=4)
*	   5 客户端向端口号x发送block=n, 大小<512的数据包，结束通信
*      6 服务器回复block=n的ack(opcode=4)
*/
int tftp_put(char *localfile, char *remotefile, char *serverip)
{
	int sockfd;
	struct sockaddr_in servaddr;
	struct sockaddr_in recvaddr;
	socklen_t servaddr_len;	
	struct iovec iovsend[2], iovrecv[2];

	int n,i;
	char sendline[1600];
	char recvline[1600];
	char printbuf[100];
	int fd;
	unsigned long filesize;
	unsigned long count=0;
	uint16_t num=0;
	uint32_t ts;

/*打开本地文件*/	
	fd = open(localfile, O_RDWR);
	if(fd < 0)
	{
		printf("Error:open %s\n",localfile);
		return 1;
	}
	filesize = get_file_size(localfile);
/*创建udp套接字*/	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(69); /*第一个包是69，后面服务器端口会在这个返回包中再次修改*/
	inet_pton(AF_INET, serverip, &servaddr.sin_addr);
	servaddr_len = sizeof(servaddr);

	if(rttinit == 0){
		rtt_init(&rttinfo);
		rttinit = 1;
		rtt_d_flag = 1;
	}
	
/*发送tftp write请求*/	
	sendline[0] = 0;
	sendline[1] = 2;  /*2：write request*/
	strcpy(sendline+2,remotefile);
	n = strlen(remotefile);
	strcpy(sendline+2+n+1,"octet");
	sendto(sockfd, sendline, 2+n+1+6, 0, (struct sockaddr *)&servaddr, servaddr_len);

	msgrecv.msg_name = &servaddr;       /*接收时要获取服务器端口号，在下次发送时使用*/
	msgrecv.msg_namelen = servaddr_len;
	msgrecv.msg_iov = iovrecv;
	msgrecv.msg_iovlen = 2;
	iovrecv[0].iov_base = &recvhdr;
	iovrecv[0].iov_len = sizeof(struct hdr);
	iovrecv[1].iov_base = recvline;
	iovrecv[1].iov_len = 512;
	
	msgsend.msg_name = &servaddr;
	msgsend.msg_namelen = servaddr_len;
	msgsend.msg_iov = iovsend;
	msgsend.msg_iovlen = 2;
	iovsend[0].iov_base = &sendhdr;
	iovsend[0].iov_len = sizeof(struct hdr);
	iovsend[1].iov_base = sendline;
	iovsend[1].iov_len = 512;	
	sendhdr.opcode = htons(3); /*3：data block*/          
/*服务器回复 block0*/
	n = recvmsg(sockfd, &msgrecv, 0);
//	printf("recv Port:%x %x,%x\n",servaddr.sin_port,ntohs(recvhdr.opcode),ntohs(recvhdr.num));
	printf("###Put %s[%d]###\n",localfile,filesize);
	signal(SIGALRM, sig_alrm);
		
	while(1)
	{
/*从block1 开始发送数据块*/
		num++;
		sendhdr.num = htons(num);	
		n = read(fd,sendline,512);
		iovsend[1].iov_len = n;
		count +=n;
		rtt_newpack(&rttinfo);
sendagain:
		ts = rtt_ts(&rttinfo);
		sendmsg(sockfd, &msgsend, 0);
		alarm(rtt_start(&rttinfo));
		if(sigsetjmp(jmpbuf, 1) != 0)
		{
			if(rtt_timeout(&rttinfo) < 0){
				printf("Error: no response,give up\n");
				rttinit = 0;
				return 1;
			}
			goto sendagain;
		}
/*必须等到接收到的block号与发送的一致*/		
		do{
			recvmsg(sockfd, &msgrecv, 0);
			if(ntohs(recvhdr.opcode) == 5)
			{
				printf("Tftp error %d: %s\n",ntohs(recvhdr.num),recvline);
			}
		}while(ntohs(recvhdr.num) != num || ntohs(recvhdr.opcode) == 5);

		alarm(0);
		rtt_stop(&rttinfo, rtt_ts(&rttinfo)-ts);
//		printf("recv %x,%x\n",ntohs(recvhdr.opcode),ntohs(recvhdr.num));
		if(ntohs(recvhdr.num)%25 == 0 || filesize == count)
		{
			printf("\r");
			sprintf(printbuf,"|                    | : %02d%%",count*100/filesize);
			for(i=1; i<=count*20/filesize; i++)
				printbuf[i] = '=';
			printf("%s",printbuf);
			fflush(stdout);
		}

		if(n<512)
		{
			printf(".\nSend : %d done.\n",count);
			return 0;
		}
	}
}
/********************************客户端下载*********************************/
/*     1 客户端向69端口发送read命令(opcode=1)
*      2 服务器回复block=1,2,3的data(opcode=3)，大小为512,同时回复服务器的端口号x
*      3 客户端向端口号x发送block=1,2,3....的ack(opcode=4)
*	   4 服务器回复block=n, 大小<512的数据包，结束通信
*      5 客户端发送block=n的ack(opcode=4) 可有可无
*/
int tftp_get(char *localfile, char *remotefile, char *serverip)
{
	int sockfd;
	struct sockaddr_in servaddr;
	struct sockaddr_in recvaddr;
	socklen_t servaddr_len;	
	struct iovec iovsend[2], iovrecv[2];

	int n,i;
	char sendline[1600];
	char recvline[1600];
	char printbuf[100];
	int fd;
	unsigned long filesize;
	unsigned long count=0;
	uint16_t num=1;
	uint32_t ts;

/*打开本地文件*/	
	fd = open(localfile, O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if(fd < 0)
	{
		printf("Error:create %s\n",localfile);
		return 1;
	}

/*创建udp套接字*/	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(69); /*第一个包是69，后面服务器端口会在这个返回包中再次修改*/
	inet_pton(AF_INET, serverip, &servaddr.sin_addr);
	servaddr_len = sizeof(servaddr);

	if(rttinit == 0){
		rtt_init(&rttinfo);
		rttinit = 1;
		rtt_d_flag = 1;
	}
	
/*发送tftp read请求*/	
	sendline[0] = 0;
	sendline[1] = 1;  /*2：read request*/
	strcpy(sendline+2,remotefile);
	n = strlen(remotefile);
	strcpy(sendline+2+n+1,"octet");
	sendto(sockfd, sendline, 2+n+1+6, 0, (struct sockaddr *)&servaddr, servaddr_len);

	msgrecv.msg_name = &servaddr;       /*接收时要获取服务器端口号，在下次发送时使用*/
	msgrecv.msg_namelen = servaddr_len;
	msgrecv.msg_iov = iovrecv;
	msgrecv.msg_iovlen = 2;
	iovrecv[0].iov_base = &recvhdr;
	iovrecv[0].iov_len = sizeof(struct hdr);
	iovrecv[1].iov_base = recvline;
	iovrecv[1].iov_len = 512;
	
	msgsend.msg_name = &servaddr;
	msgsend.msg_namelen = servaddr_len;
	msgsend.msg_iov = iovsend;
	msgsend.msg_iovlen = 2;
	iovsend[0].iov_base = &sendhdr;
	iovsend[0].iov_len = sizeof(struct hdr);
	iovsend[1].iov_base = sendline;
	iovsend[1].iov_len = 0;	
	sendhdr.opcode = htons(4); /*3：ack block*/          
/*服务器回复 block1*/
	printf("###Get %s###\n",localfile);
	n = recvmsg(sockfd, &msgrecv, 0);
	if(ntohs(recvhdr.num) == num && ntohs(recvhdr.opcode) == 3 && n>=4)
	{
		write(fd,recvline,n-4);
		num++;
		count +=n-4;
	}
	else
	{
		printf("Block1 recv error\n");
		return 1;
	}
	signal(SIGALRM, sig_alrm);
		
	while(1)
	{
		sendhdr.num = ntohs(recvhdr.num);	
		rtt_newpack(&rttinfo);		
sendagain:
		ts = rtt_ts(&rttinfo);
		sendmsg(sockfd, &msgsend, 0);
		alarm(rtt_start(&rttinfo));
		if(sigsetjmp(jmpbuf, 1) != 0)
		{
			if(rtt_timeout(&rttinfo) < 0){
				printf("Error: no response,give up\n");
				rttinit = 0;
				return 1;
			}
			goto sendagain;
		}
		
/*必须等到接收到的block号与期望的一致*/
		do{
			n = recvmsg(sockfd, &msgrecv, 0);
			if(ntohs(recvhdr.opcode) == 5)
			{
				printf("Tftp error %d: %s\n",ntohs(recvhdr.num),recvline);
			}
			
			if(ntohs(recvhdr.num) == num && ntohs(recvhdr.opcode) == 3 && n>=4)
			{
				write(fd,recvline,n-4);
				num++;
				count +=n-4;
				break;
			}
		}while(1);

		alarm(0);
		rtt_stop(&rttinfo, rtt_ts(&rttinfo)-ts);
//		printf("recv %x,%x\n",ntohs(recvhdr.opcode),ntohs(recvhdr.num));
		if(ntohs(recvhdr.num)%25 == 0)
		{
			printf("\rRecv : %d",count);
			fflush(stdout);
		}

		if(n<512)
		{
			printf("\rRecv : %d done.\n",count);
			return 0;
		}
	}
}

void show_usage(void)
{
	printf("Usage: tftp [OPTION]... HOST\n");
	printf("Options:\n");
	printf("\t-l FILE -Local FILE\n");
	printf("\t-r FILE -Remote FILE\n");
	printf("\t-g      -Get file from server\n");
	printf("\t-p      -Put file to server\n");
}

/*主函数*/
extern char *optarg;  /*带冒号的参数的值*/
extern int optind;    /*getopt后下一个要处理的argv[]的下标*/
int main(int argc, char **argv)
{
	int opt;
	int put;
	char localfile[1024]={0};
	char remotefile[1024]={0};
	char *local,*remote;
	
	while ((opt = getopt(argc, argv, "gpr:l:")) != -1) /*循环返回每一个需要捕捉的参数*/
	{
		switch(opt)
		{
			case 'p' :
				put = 1; 
				break;
			case 'g' : 
				put = 0; 
				break;
			case 'r' : 
				memcpy(remotefile,optarg,strlen(optarg)); /*带冒号的参数有附件值*/
				break;
			case 'l' : 
				memcpy(localfile,optarg,strlen(optarg));
				break;
			default:
				show_usage();
				break;
		}
	}
	
	if(localfile[0] == 0 && remotefile[0] == 0 || argv[optind] == NULL)
	{
		show_usage();
		return 1;
	}
	else if(localfile[0] == 0)
	{
		local = remotefile;
		remote = remotefile;
	}
	else if(remotefile[0] == 0)
	{
		local = localfile;
		remote = localfile;
	}
	
	if(put == 1)
	{
		tftp_put(local,remote,argv[optind]);
	}
	else
	{
		tftp_get(local,remote,argv[optind]);
	}
//	tftp_get(argv[1],argv[1],argv[2]);
//	tftp_put(argv[1],"dada",argv[2]);

	return 0;
}
