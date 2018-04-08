#include <unistd.h>
#include <sys/utsname.h>

int show_uname(void)
{
	struct utsname usname;
	char hostname[256];
	printf("***********************************\n");
	uname(&usname);
	printf("%s\n %s\n %s\n %s\n %s\n",usname.sysname, usname.nodename, usname.release, usname.version, usname.machine);
	
	gethostname(hostname,256);
	printf("hostname = %s\n",hostname);
}
