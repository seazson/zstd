#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include     <unistd.h>
#include     <sys/types.h>
#include     <sys/stat.h>
#include     <fcntl.h>
#include     <termios.h>
#include     <errno.h>

#define UART0_DEV "/dev/ssruart0"
#define UART1_DEV "/dev/ssruart1"
#define UART2_DEV "/dev/ssruart2"
#define UART3_DEV "/dev/ssruart3"

#define  BUF_SIZE 2048


int check_data(int uart_fd0)
{
	int i = 0;
	unsigned char send_buf[BUF_SIZE];
	unsigned char recv_buf[BUF_SIZE];
	int nwrite = 0;
	int nread = 0;
	int count = 0;
	int timeout = 0;
//	int max = 0;

	memset(send_buf, 0, sizeof(send_buf));
	for(i=0; i<BUF_SIZE; i++)
	{
		send_buf[i] = i;
	}

	count = 0;
	while(BUF_SIZE > count)
	{
		nwrite = write(uart_fd0, (send_buf+count), (BUF_SIZE-count));
		if(nwrite < 0)
		{
			printf("%d write err\n", uart_fd0);
			fflush(stdout);
			return -1;
		}

		count+= nwrite;
	}

    sleep(1);
    
	memset(recv_buf, 0, sizeof(recv_buf));

	count = 0;
	timeout = 0;
	while(BUF_SIZE > count)
	{
		nread = read(uart_fd0, (recv_buf+count), (BUF_SIZE-count));
		if(nread < 0)
		{
			printf("%d read err\n", uart_fd0);
			fflush(stdout);
			return -1;
		}
		else if(nread == 0)
		{
			timeout++;
			if(timeout >= 500)
			{
				printf("%d read timeout\n", uart_fd0);
				fflush(stdout);
				return -1;
			}
		}
		else
		{
			count+= nread;
			
			timeout = 0;
		}
	}


	for(i=0; i<BUF_SIZE; i++)
	{
		if(send_buf[i] != recv_buf[i])
		{
			printf("the loopback test failed between %d and %d when %d send %d recv\n", i ,i, send_buf[i], recv_buf[i]);
			fflush(stdout);
			return -1;
		}
	}

        return 0;
}


int test_loopback(const char *pc1)
{
    int uart_fd=0;
	int ret = 0;
	int i = 0;
	unsigned char send_buf[BUF_SIZE];
	unsigned char recv_buf[BUF_SIZE];

	fd_set fs_write;
	struct timeval tv_timeout;

    uart_fd = open(pc1, O_RDWR);
    if(uart_fd < 0)
    {
            printf("cann't open the dev %s\n", pc1);
            fflush(stdout);
            return -1;
    }


/*	ret = write(uart_fd, "hello\n", 7);
	if(ret < 0)
	{
		printf("%d write err\n", uart_fd);
		fflush(stdout);
		return -1;
	}*/

	sleep(1);
	memset(recv_buf, 0x00, sizeof(recv_buf));
	while(1) 
	{
/* 		FD_ZERO (&fs_write);
		FD_SET (uart_fd, &fs_write);
		tv_timeout.tv_sec = 0;
		tv_timeout.tv_usec = 0; 
		ret = select (uart_fd+1, NULL, &fs_write, NULL, &tv_timeout);*/

		ret = read(uart_fd, recv_buf, 20);
		if(ret < 0)
		{
			printf("%d read err\n", uart_fd);
			fflush(stdout);
			return -1;
		}
		for(i=0; i<ret; i++)
		{
			printf("%c",recv_buf[i]);	
		}	
		if(ret > 0)
		{
//				printf("read %d:%s\n",ret,recv_buf);
//			write(uart_fd,recv_buf,ret);
		}
//		sleep(10);
//		if(ret > 0)
//			write(uart_fd,recv_buf,ret);
//		if(ret > 0)
//		for(i=0; i<ret; i++)
//			printf("%d [ %d : %x\n",ret, i,recv_buf[i]);

	}

	return 0;
}

int check_data2(int uart_fd0, int uart_fd1)
{
	int i = 0;
	unsigned char send_buf[BUF_SIZE];
	unsigned char recv_buf[BUF_SIZE];
	int nwrite = 0;
	int nread = 0;
	int count = 0;
	int timeout = 0;
//	int max = 0;

	memset(send_buf, 0, sizeof(send_buf));
	for(i=0; i<BUF_SIZE; i++)
	{
		send_buf[i] = i;
	}

	count = 0;
	while(BUF_SIZE > count)
	{
		nwrite = write(uart_fd0, (send_buf+count), (BUF_SIZE-count));
		if(nwrite < 0)
		{
			printf("%d write err\n", uart_fd0);
			fflush(stdout);
			return -1;
		}

		count+= nwrite;
	}

    sleep(1);
    
	memset(recv_buf, 0, sizeof(recv_buf));

	count = 0;
	timeout = 0;
	while(BUF_SIZE > count)
	{
		nread = read(uart_fd1, (recv_buf+count), (BUF_SIZE-count));
		if(nread < 0)
		{
			printf("%d read err\n", uart_fd1);
			fflush(stdout);
			return -1;
		}
		else if(nread == 0)
		{
			timeout++;
			if(timeout >= 500)
			{
				printf("%d read timeout\n", uart_fd1);
				fflush(stdout);
				return -1;
			}
		}
		else
		{
			count+= nread;
			
			timeout = 0;
		}
	}


	for(i=0; i<BUF_SIZE; i++)
	{
		if(send_buf[i] != recv_buf[i])
		{
			printf("the loopback test failed at %d  when %d send %d recv\n", i, send_buf[i], recv_buf[i]);
			fflush(stdout);
		}
	}

        return 0;
}


int test_loopback2(const char *pc1, const char *pc2)
{
    int uart_fd[2];
	int ret = 0;
	int i = 0;

        memset(uart_fd, 0, sizeof(uart_fd));

        uart_fd[0] = open(pc1, O_RDWR);
        if(uart_fd[0] < 0)
        {
                printf("cann't open the dev %s\n", pc1);
                fflush(stdout);
                return -1;
        }

        uart_fd[1] = open(pc2, O_RDWR);
        if(uart_fd[1] < 0)
        {
                printf("cann't open the dev %s\n", pc2);
                fflush(stdout);

                close(uart_fd[0]);

                return -1;
        }
   
	ret = check_data2(uart_fd[0], uart_fd[1]);
	if(ret < 0)
	{
		goto err;
	}

	ret = check_data2(uart_fd[1], uart_fd[0]);
	if(ret < 0)
	{
		goto err;
	}


        for(i=0; i<2; i++)
        {
                close(uart_fd[i]);
        }

        return 0;

err:

        for(i=0; i<2; i++)
        {
                close(uart_fd[i]);
        }

        return -1;
}


int main(int argv, char *args[])
{
	int ret = 0;
 
 	if(argv < 2)
	{
		printf(" use open dev");
		return 0;
	}
	printf("*************begin uart loop back test %s**********************\n",args[1]);
	ret = test_loopback(args[1]);
	if(ret < 0)
	{
		printf("uart0 SSRUART TEST FAILED\n");
		fflush(stdout);
	}

//	printf("*************begin uart loop back test2**********************\n");
//	ret = test_loopback2(UART0_DEV,UART1_DEV);
	if(ret < 0)
	{
//		printf("uart0 and uart1 SSRUART TEST FAILED\n");
//		fflush(stdout);
	}
	
	return 0;
}

