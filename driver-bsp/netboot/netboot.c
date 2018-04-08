/*============================================================================
 *							从网络启动语音子系统
 *							                   20130527			
 *
============================================================================*/
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/stat.h>

#include "if_csmencaps.h"

/* include/linux/socket.h */
#define SOL_CSM_ENCAPS 			269

#define CSME_ACK_ENABLE			1
#define ETH_P_CSME_BE			0x9b88
#define CSME_OPCODE_CONTROL_BE	0x0100

#define MAX_MSP_MSG_LEN         1500
#define SUPERVISOR				0xFFFF

#define BOOTLOADADDR            0x8000000
/* global csme socket structure */
struct sockaddr_csme sockaddrcsme;
struct csme_ackopt ack_opt;

/* this is the control MAC address of the device */	
unsigned char dev_mac[6]={0x02,0x50,0xc2,0x3b,0x70,0x01};
unsigned char broad_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char host_mac[6];
/* descriptor of the msp_control socket */
int msp_control_sockfd;
int bootdone = 0;
	
/* Below cmd are use in boot mode */
char cmd_query_ready[14]={13,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x1e,		/* Command Class device */
				0x00,0x00,	/* Function code ignore*/
				0x00,0x0e,0x5e,0xaa,0xbb,0x06, /*Param1 soures mac*/
				0x88,0x9b};	/* Param2 889b*/

char cmd_maas_assign[20]={19,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x1b,		/* Command Class device */
				0x00,0x00,	/* Function code ignore*/
				0x02,0x50,  /*Param1 dsp mac*/
				0xc2,0x3b,  /*Param2 dsp mac*/
				0x70,0x00,  /*Param3 dsp mac*/
				0x00,0x0e,  /*Param4 host mac*/
				0x5e,0xaa,  /*Param5 host mac*/
				0xbb,0x06,  /*Param7 host mac*/
				0x88,0x9b};	/* Param7 */

char cmd_set_pll_amba[12]={11,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x1f,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0x00,0x02,  /* Param1 choose abma=137.5M default is 266.66*/
				0x00,0x2b,  /* Param2 clkf=43*/
				0x00,0x01};	/* Param3 clkr=0 clkod=1*/

char cmd_set_pll_arm[12]={11,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x1f,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0x00,0x00,  /* Param1 choose arm=450M default is 450*/
				0x00,0x23,  /* Param2 clkf=0x23*/
				0x00,0x00};	/* Param3 clkr=0 clkod=0*/

char cmd_set_pll_spu[12]={11,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x1f,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0x00,0x01,  /* Param1 choose spu=350M default is 350*/
				0x00,0x37,  /* Param2 clkf=0x37*/
				0x00,0x01};	/* Param3 clkr=0 clkod=1*/

char cmd_sdram_params[14]={13,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x0d,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0xd6,0xdb,  /* Param1 tras=6 trc=b trcd=3 trp=3 cl=3*/
				0x0b,0xca,  /* Param2 twr=3 tmrs=2 trrd=2 bank=1[4bank] 0bca*/
				0x02,0x00,  /* Param3 refrash=0x200*/
				0x00,0x0a};	/* Param4 poweron no use 0x682b*/

char cmd_cs_params[12]={11,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x0f,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0x00,0x00,  /* Param1 cs=0*/
				0x02,0x00,  /* Param2 time*/
				0x00,0x0E};	/* Param3 size=2^14*4KB=64M*/

char cmd_fifowrite[14]={13,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x05,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0x00,0x10,  /* Param1 msb*/
				0x00,0x00,  /* Param2 lsb*/
				0x00,0x02,  /* Param3 len=2*/
				0x55,0xca};	/* Param4 data...*/

char cmd_fiforead[14]={11,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x04,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0x00,0x10,  /* Param1 msb*/
				0x00,0x00,  /* Param2 lsb*/
				0x00,0x02};	/* Param3 len=2*/

char cmd_progstart[14]={9,		/* msg length*/
				0x00,		/* index */
				0x04,		/* Command Type query */
				0x06,		/* Command Class device */
				0x00,0x00,	/* Function code ignore */
				0x08,0x00,  /* Param1 msb ERAM:0x8000000*/
				0x00,0x00}; /* Param2 lsb*/

/* Prefilled buffer for get arm version; Note:this cmd can not be used in boot mode!*/
char get_device_arm_version[8]={0x08,		/* msg length*/
				0x00,		/* index */
				0x01,		/* Command Type query */
				0x06,		/* Command Class device */
				0x21,0x00,	/* Function code GET_ARM_CODE_VERSION */
				0x00,0x00};	/* Reserved */

extern unsigned int load_aif_image(unsigned int addr, unsigned int size);
#define SIOCGIFHWADDR	0x8927		/* Get hardware address		*/
int get_mac(char *name)
{
    int sockfd;
    struct ifreq tmp;   
    char mac_addr[30];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0)
    {
        return -1;
    }

    memset(&tmp,0,sizeof(struct ifreq));
    strncpy(tmp.ifr_name, name, sizeof(tmp.ifr_name)-1);
    if( (ioctl(sockfd,SIOCGIFHWADDR,&tmp)) < 0 )
    {
        printf("mac ioctl error\n");
        return -1;
    }
    close(sockfd);
    memcpy(host_mac,tmp.ifr_hwaddr.sa_data,6);

    return 0;
}

static int change_mac(char *mac)
{
	int result;

	memcpy(&sockaddrcsme.scsme_devmac, mac, 6);

	memcpy(&ack_opt.scsme_devmac, &sockaddrcsme.scsme_devmac, 6);
	ack_opt.scsme_ifindex = sockaddrcsme.scsme_ifindex;
	ack_opt.ack_suppression = 0;

	result = setsockopt(msp_control_sockfd, SOL_CSM_ENCAPS, CSME_ACK_ENABLE, &ack_opt, sizeof(struct csme_ackopt));
	if (result <0)
	{
		printf("setsockopt failed %d; %s\n", result, strerror(errno));
		return result;
		
	}
	return 0;

}
/*------------------------------------------------------------------*/
static int init_interface(char *interface_name)
{
	int result;

	get_mac(interface_name); 

	msp_control_sockfd = socket(AF_CSME, SOCK_DGRAM, ETH_P_CSME_BE); /* the ETH_P_CSME must be in BE */
	if(msp_control_sockfd < 0) 
	{
		printf("Error creating csme socket %s\n", strerror(errno));
		return msp_control_sockfd;
	}

	sockaddrcsme.scsme_family = AF_CSME;
	sockaddrcsme.scsme_ifindex = if_nametoindex(interface_name);
	memcpy(&sockaddrcsme.scsme_devmac, dev_mac, 6);
	sockaddrcsme.scsme_opcode = CSME_OPCODE_CONTROL_BE; /* the CSME_OPCODE_CONTROL must be in BE */

	memcpy(&ack_opt.scsme_devmac, &sockaddrcsme.scsme_devmac, 6);
	ack_opt.scsme_ifindex = sockaddrcsme.scsme_ifindex;
	ack_opt.ack_suppression = 0;

	result = setsockopt(msp_control_sockfd, SOL_CSM_ENCAPS, CSME_ACK_ENABLE, &ack_opt, sizeof(struct csme_ackopt));
	if (result <0)
	{
		printf("setsockopt failed %d; %s\n", result, strerror(errno));
		return result;
		
	}

	return 0;

}

/*------------------------------------------------------------------*/
static int recv_msp_msg(int in_sockfd,int print)
{
	socklen_t sock_len;
	int i, len;
	char *rcv_buf;
	struct sockaddr_csme rcv_sockaddrcsme;

	rcv_buf = (char *)malloc(MAX_MSP_MSG_LEN);
	if(rcv_buf == NULL)
	{
		printf("Receive buffer allocation\n");
		return -1;
	}
	else
	{
		sock_len = sizeof(struct sockaddr_csme);
		len = recvfrom (in_sockfd, (char *)rcv_buf, MAX_MSP_MSG_LEN, 0, (struct sockaddr *)&rcv_sockaddrcsme, &sock_len);
		if(print == 1)
		{
			printf("Received %d bytes\n", len); 
			printf("index  0x%02x\n", rcv_buf[0]);
			printf("length 0x%02x(ignore)\n", rcv_buf[1]);
			printf("Command Class 0x%02x\n", rcv_buf[2]);
			printf("Command Type 0x%02x\n",  rcv_buf[3]);
			printf("Function Code 0x%04x(ignore)\n", rcv_buf[4]);
			printf("Payload: "); 
			for (i = 6; i < len; i++)
				printf("0x%02x ", rcv_buf[i]);
			printf("\n");
		}
		return 0;
	}

	free(rcv_buf);
}

/*------------------------------------------------------------------*/
static int send_msg_to_channel(int in_sockfd, char *in_buf, int in_buf_size, unsigned short channel)
{
	int bytes_sent;
	struct sockaddr_csme stSockAddr;
	
	memcpy(&stSockAddr, &sockaddrcsme, sizeof(struct sockaddr_csme));
	/* we are not in boot phase*/
	if(bootdone)
		stSockAddr.scsme_flags = 0x1;
	else
		stSockAddr.scsme_flags = 0x0;

	stSockAddr.scsme_channelid = channel; /*channel (need to be in Network order) */

	bytes_sent = sendto(in_sockfd, (char *)in_buf, in_buf_size, MSG_DONTWAIT, (struct sockaddr *)&stSockAddr, sizeof(sockaddrcsme)); 
	
	return bytes_sent;
}

/*------------------------------------------------------------------*/
static int wait_for_response(int print)
{
	fd_set read_fd;
	int max_fds;
	int result;
	
	FD_ZERO(&read_fd);
	FD_SET(msp_control_sockfd, &read_fd);
	
	max_fds = msp_control_sockfd + 1;
	
	/* wait for reception */
	result = select(max_fds, &read_fd, 0, 0, NULL);
	if (result <= 0)
	{
		printf("Select error =%s\n", strerror(errno));
		return (result);
	}

	if (FD_ISSET(msp_control_sockfd, &read_fd))
	{
		/* read the message from the device */
		result = recv_msp_msg(msp_control_sockfd,print);
	}

	return result;
}

/********************************************************************/
int maas_assign(void)
{
	int result;
	char msg_buf[20];

	change_mac(broad_mac);

	msg_buf[0] = 19;
	msg_buf[1] = 0x00;
	msg_buf[2] = 0x04;
	msg_buf[3] = 0x1B;
	msg_buf[4] = 0x00; msg_buf[5] = 0x00;
	msg_buf[6]  = dev_mac[0]; msg_buf[7]  = dev_mac[1]; msg_buf[8]  = dev_mac[2];
	msg_buf[9]  = dev_mac[3]; msg_buf[10] = dev_mac[4]; msg_buf[11] = dev_mac[5];
	msg_buf[12] = host_mac[0]; msg_buf[13] = host_mac[1]; msg_buf[14] = host_mac[2];
	msg_buf[15] = host_mac[3]; msg_buf[16] = host_mac[4]; msg_buf[17] = host_mac[5];
	msg_buf[18] = 0x88;
	msg_buf[19] = 0x9b;
	
    printf("Assign DSP MAC: %02x:%02x:%02x:%02x:%02x:%02x   \n",dev_mac[0],dev_mac[1],dev_mac[2],dev_mac[3],dev_mac[4],dev_mac[5]);
    printf("Host MAC: %02x:%02x:%02x:%02x:%02x:%02x...",host_mac[0],host_mac[1],host_mac[2],host_mac[3],host_mac[4],host_mac[5]);

    result = send_msg_to_channel(msp_control_sockfd, msg_buf, 20, 0x0000);
	if (result < 0)
	{
		printf("Failed to send msg = %s\n", strerror(errno));
		return (result);
	}

	change_mac(dev_mac);

	wait_for_response(0);
	printf("\tDone!\n");
	return result;
}

int query_ready(void)
{
	int result;
	char msg_buf[14];

	msg_buf[0] = 13;
	msg_buf[1] = 0x00;
	msg_buf[2] = 0x04;
	msg_buf[3] = 0x1e;
	msg_buf[4] = 0x00; msg_buf[5] = 0x00;
	msg_buf[6]  = host_mac[0]; msg_buf[7]  = host_mac[1]; msg_buf[8]  = host_mac[2];
	msg_buf[9]  = host_mac[3]; msg_buf[10] = host_mac[4]; msg_buf[11] = host_mac[5];
	msg_buf[12] = 0x88;
	msg_buf[13] = 0x9b;
	
	printf("Send Query_READY...");          
    result = send_msg_to_channel(msp_control_sockfd, msg_buf, 14, 0x0000);
	if (result < 0)
	{
		printf("Failed to send msg = %s\n", strerror(errno));
		return (result);
	}
	wait_for_response(0);
	printf("\t\tDone!\n");
	return result;
}

int go_to_start(unsigned int addr)
{
	int result;
	char msg_buf[10];

	msg_buf[0] = 9;
	msg_buf[1] = 0x00;
	msg_buf[2] = 0x04;
	msg_buf[3] = 0x06;
	msg_buf[4] = 0x00; msg_buf[5] = 0x00;
	msg_buf[6] = addr>>24;
	msg_buf[7] = addr>>16;
	msg_buf[8] = addr>>8;
	msg_buf[9] = addr&0xFF;

	printf("Go at EntryPoint 0x%08x......",addr);
	result = send_msg_to_channel(msp_control_sockfd, msg_buf, 10, 0x0000);
	if (result < 0)
	{
		printf("Failed to send msg = %s\n", strerror(errno));
		return (result);
	}
	wait_for_response(0);
	printf("\tDone!\n");
	return result;
}


/********************************************************************/
/*8 bit write/read, mainaly use to thansimition data*/
int write_to_voip(unsigned int addr,unsigned int len, char *buf)
{
	int result;
	char *msg_buf;
	msg_buf = malloc(len+12);

//	printf("Send data len=%d...\n",len);  

	msg_buf[0] = len+11;
	msg_buf[1] = 0x00;
	msg_buf[2] = 0x04;
	msg_buf[3] = 0x05;
	msg_buf[4] = 0x00; msg_buf[5] = 0x00;
	msg_buf[6] = addr>>24;
	msg_buf[7] = addr>>16;
	msg_buf[8] = addr>>8;
	msg_buf[9] = addr&0xFF;
	msg_buf[10] = len>>8;
	msg_buf[11] = len&0xFF;

	memcpy(msg_buf+12, buf, len);
        
	result = send_msg_to_channel(msp_control_sockfd, msg_buf, len+12, 0x0000);
	if (result < 0)
	{
		printf("Failed to send msg = %s\n", strerror(errno));
		return (result);
	}
//	printf("send msg %d done. now wait for response...\n",result);
	
	wait_for_response(0);

	if(result>=12)
		result -= 12;

	return result;
}

int read_from_voip(unsigned int addr,unsigned int len)
{
	int result;
	char *msg_buf;
	msg_buf = malloc(len+12);

	printf("Send data len=%d...\n",len);  

	msg_buf[0] = len+11;
	msg_buf[1] = 0x00;
	msg_buf[2] = 0x04;
	msg_buf[3] = 0x04;
	msg_buf[4] = 0x00; msg_buf[5] = 0x00;
	msg_buf[6] = addr>>24;
	msg_buf[7] = addr>>16;
	msg_buf[8] = addr>>8;
	msg_buf[9] = addr&0xFF;
	msg_buf[10] = len>>8;
	msg_buf[11] = len&0xFF;

	result = send_msg_to_channel(msp_control_sockfd, msg_buf, 12, 0x0000);
	if (result < 0)
	{
		printf("Failed to send msg = %s\n", strerror(errno));
		return (result);
	}
//	printf("send msg %d done. now wait for response...\n",result);
	
	result = wait_for_response(1);
}

/************32 bit write/read, we can use it to config the cpu register*****************/
void write_reg(unsigned int addr, unsigned int val)
{
	int result;
	char msg_buf[14];

	msg_buf[0] = 13;
	msg_buf[1] = 0x00;
	msg_buf[2] = 0x04;
	msg_buf[3] = 0x17;
	msg_buf[4] = 0x00; msg_buf[5] = 0x00;
	msg_buf[6] = addr>>24;
	msg_buf[7] = addr>>16;
	msg_buf[8] = addr>>8;
	msg_buf[9] = addr&0xFF;
	msg_buf[10] = val>>24;
	msg_buf[11] = val>>16;
	msg_buf[12] = val>>8;
	msg_buf[13] = val&0xff;
        
	result = send_msg_to_channel(msp_control_sockfd, msg_buf, 14, 0x0000);
	if (result < 0)
	{
		printf("Failed to send msg = %s\n", strerror(errno));
	}
	wait_for_response(0);
}

unsigned int read_reg(unsigned int addr)
{
	int result;
	int len = 4;
	char *msg_buf;
	fd_set read_fd;
	int max_fds;
	unsigned int val=0;
	
	msg_buf = malloc(len+12);
	msg_buf[0] = len+11;
	msg_buf[1] = 0x00;
	msg_buf[2] = 0x04;
	msg_buf[3] = 0x04;
	msg_buf[4] = 0x00; msg_buf[5] = 0x00;
	msg_buf[6] = addr>>24;
	msg_buf[7] = addr>>16;
	msg_buf[8] = addr>>8;
	msg_buf[9] = addr&0xFF;
	msg_buf[10] = len>>8;
	msg_buf[11] = len&0xFF;

	result = send_msg_to_channel(msp_control_sockfd, msg_buf, 12, 0x0000);
	if (result < 0)
	{
		printf("Failed to send msg = %s\n", strerror(errno));
		return (result);
	}

	FD_ZERO(&read_fd);
	FD_SET(msp_control_sockfd, &read_fd);
	max_fds = msp_control_sockfd + 1;
	/* wait for reception */
	result = select(max_fds, &read_fd, 0, 0, NULL);
	if (result <= 0)
	{
		printf("Select error =%s\n", strerror(errno));
		return (result);
	}

	if (FD_ISSET(msp_control_sockfd, &read_fd))
	{
		socklen_t sock_len;
		int i, len;
		char *rcv_buf;
		struct sockaddr_csme rcv_sockaddrcsme;

		rcv_buf = (char *)malloc(MAX_MSP_MSG_LEN);
		if(rcv_buf == NULL)
		{
			printf("Receive buffer allocation\n");
			return -1;
		}
		else
		{
			sock_len = sizeof(struct sockaddr_csme);
			len = recvfrom (msp_control_sockfd, (char *)rcv_buf, MAX_MSP_MSG_LEN, 0, (struct sockaddr *)&rcv_sockaddrcsme, &sock_len);
			if(len >= 10)
				val=(rcv_buf[9]<<24)|(rcv_buf[8]<<16)|(rcv_buf[7]<<8)|(rcv_buf[6]);
			else
				printf("Read error\n");
		}
		free(rcv_buf);
		return val;
	}
}

/**************************************************************************************/
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

void send_file_to_voip(char *name, unsigned int addr)
{
	FILE *fp;
	char buf[1000];
	unsigned int len;
	unsigned int addr_t=addr;
	
	if((fp=fopen(name,"rb")) == NULL)
	{
		printf("ERROR:open %s\n", name);
		return;
	}
	
	while(feof(fp)==0)
	{
		len = fread(buf, 1, 1000, fp);
		write_to_voip(addr_t,len,buf);
		addr_t += len;
	}
	printf("Write %d to voip\n", addr_t-addr);
	fclose(fp);
}

unsigned int load_msp_to_voip(char *name)
{
	FILE *fp;
	char *buf;
	unsigned int len;
	unsigned int addr_t = 0;
	unsigned int load_addr = 0;

	unsigned long filesize = get_file_size(name);
	if(filesize == -1)
	{
		printf("ERROR: Get %s size :0x%x\n", name, filesize);
		return -1;
	}
	
	buf = malloc(filesize);	
	if(buf == NULL)
	{
		printf("ERROR: Can not alloc mem %s\n", name);
		return -1;
	}

	if((fp = fopen(name,"rb")) == NULL)
	{
		printf("ERROR: Open %s\n", name);
		return -1;
	}
	
	while(feof(fp)==0)
	{
		len = fread(buf+addr_t, 1, 1000, fp);
		addr_t += len;
	}
	printf("MSP size %d\n", addr_t);
	fclose(fp);

	load_addr = load_aif_image((unsigned int)buf, filesize);
	printf("Load addr: 0x%08x\n", load_addr);

	free(buf);

	return load_addr;
}

/*------------------------------------------------------------------*/
int main(int argc, char** argv)
{
	int result;
	int i;

	printf("Boot voip by ethernet\n");	

/*init process*/	
	result = init_interface("ipc0");
	if (result < 0)
	{
		printf("ERROR: Fail to init interface: %s\n", strerror(errno));
		return (result);
	}
	
/*Assign mac and read query*/
	maas_assign();
	query_ready();

/*Init pll and ddr. Note: we cann't init it twice, then dsp will down*/
	if(read_reg(0x1008007c) != 0x1921)
	{
		printf("Send Set_Pll for amba...");          
		result = send_msg_to_channel(msp_control_sockfd, cmd_set_pll_amba, sizeof(cmd_set_pll_amba), 0x0000);
		if (result < 0)
		{
			printf("ERROR: Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(0);
		printf("\tDone!\n");

		printf("Send Set_Pll for arm... ");          
		result = send_msg_to_channel(msp_control_sockfd, cmd_set_pll_arm, sizeof(cmd_set_pll_arm), 0x0000);
		if (result < 0)
		{
			printf("ERROR: Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(0);
		printf("\tDone!\n");

		printf("Send Set_Pll for spu... ");          
		result = send_msg_to_channel(msp_control_sockfd, cmd_set_pll_spu, sizeof(cmd_set_pll_spu), 0x0000);
		if (result < 0)
		{
			printf("ERROR: Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(0);
		printf("\tDone!\n");

		printf("Send Sdram_Params...");          
		result = send_msg_to_channel(msp_control_sockfd, cmd_sdram_params, sizeof(cmd_sdram_params), 0x0000);
		if (result < 0)
		{
			printf("ERROR: Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(0);
		printf("\t\tDone!\n");

		printf("Send CS_Params...");          
		result = send_msg_to_channel(msp_control_sockfd, cmd_cs_params, sizeof(cmd_cs_params), 0x0000);
		if (result < 0)
		{
			printf("ERROR: Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(0);
		printf("\t\tDone!\n");

		write_reg(0x10080010, 0x0b262333);
		write_reg(0x10080014, 0x01102223);
		write_reg(0x10080078, 0x08010404);
		write_reg(0x1008007c, 0x00001921);		
#if 0
		printf("Send Fifowrite...\n");          
		result = send_msg_to_channel(msp_control_sockfd, cmd_fifowrite, sizeof(cmd_fifowrite), 0x0000);
		if (result < 0)
		{
			printf("Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(0);

		printf("Send Fiforead...\n");          
		result = send_msg_to_channel(msp_control_sockfd, cmd_fiforead, sizeof(cmd_fiforead), 0x0000);
		if (result < 0)
		{
			printf("Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(1);

		printf("0x%08x ", read_reg(0x10060000));
		for(i=0; i<64 ;i++)
		{
			if(i%4 == 0)
				printf("\n0x%08x :",i*4);
			printf("0x%08x ", read_reg(0x10080000+i*4));
		}
		printf("\n");

	//	write_reg(0x10060000, 0x0000000e);
	//	write_reg(0x10080000, 0x00000200);
	//	write_reg(0x10080004, 0x00000886);
	//	write_reg(0x10080008, 0x00000910);
	//	write_reg(0x1008000c, 0x0000c910);
		write_reg(0x10080010, 0x0b262333);
		write_reg(0x10080014, 0x01102223);
	//	write_reg(0x10080018, 0x00000200);
	//	write_reg(0x1008001c, 0x0000000a);

		write_reg(0x10080078, 0x08010404);
		write_reg(0x1008007c, 0x00001921);

		printf("0x%08x ", read_reg(0x10060000));
		for(i=0; i<64 ;i++)
		{
			if(i%4 == 0)
				printf("\n0x%08x :",i*4);
			printf("0x%08x ", read_reg(0x10080000+i*4));
		}
		printf("\n");

//		write_to_voip(0x1000000,10,"1234567890123");
//		read_from_voip(0x1000000,10);
#endif
	}

/*Method 1: send boot to eram, and load msp from tftp*/
	if(0)
	{
		printf("Download boot at 0x%x...\n", BOOTLOADADDR);          
		send_file_to_voip("/var/uboot.bin", BOOTLOADADDR);
		//read_from_voip(BOOTLOADADDR,10);

		system("ifconfig ipc0 10.1.1.18");
		system("udpsvd -vE 0.0.0.0 69 tftpd /var &");

		go_to_start(BOOTLOADADDR);
	}

/*Method 2: directly load msp to ddr*/
	if(1)
	{
		unsigned int load_addr;

		/*check ddr init ok*/
		write_reg(0x1000000,0x12345678);
		if(0x12345678 != read_reg(0x1000000))
		{
			printf("ERROR: Check ddr mem error, Please init it or use ERAM.\n");
			return 1;
		}

		load_addr = load_msp_to_voip("/var/msp_823v2.axf");
		if(load_addr == -1)
			return 1;
		
//		go_to_start(load_addr);		
	}

/*Check if dsp is running*/
	if(0)
	{
		printf("Wait DSP load msp...\n");
		sleep(10);

		bootdone=1;
		result = send_msg_to_channel(msp_control_sockfd, get_device_arm_version, sizeof(get_device_arm_version), SUPERVISOR);
		if (result < 0)
		{
			printf("ERROR: Failed to send msg = %s\n", strerror(errno));
			return (result);
		}
		result = wait_for_response(1);
	}
	
	return 0;
}
