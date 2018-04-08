#include <syslog.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	int logmask;
	openlog("sealog", LOG_PID|LOG_CONS, LOG_USER);
	syslog(LOG_INFO, "infomative message, pid = %d", getpid());
	syslog(LOG_DEBUG, "debug message");
	logmask = setlogmask(LOG_UPTO(LOG_NOTICE));
	syslog(LOG_DEBUG, "okkkkkkkk");
	exit(0);

}
