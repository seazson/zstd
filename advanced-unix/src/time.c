#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int show_time(void)
{
	time_t now;
	struct timeval timeval_v;
	struct tm *tm_v,*tm_local;
	char buf[256];
	
	printf("**********************************\n");
	time(&now);
	printf("time val = %ld\n",now);
	gettimeofday(&timeval_v, NULL);
	printf("tv_sec=%ld tv_usec=%ld\n",timeval_v.tv_sec, timeval_v.tv_usec);

	tm_v = gmtime(&now);
	tm_local = localtime(&now);
	printf("ctime = %s\n",ctime(&now));
	printf("asctime = %s\n",asctime(tm_v));
	printf("local time = %s\n",asctime(tm_local));

	strftime(buf,256,"%A+%B+%H %M %S%p\n",tm_v);
	printf("format time:%s\n",buf);
	return 0;
}
