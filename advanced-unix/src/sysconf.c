/*******************************
*
*  this file use for sysconf
*
******************************/
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

void pr_sysconf(char *mesg, int name)
{
	long val;
	fputs(mesg, stdout);
	errno=0;

	if((val = sysconf(name)) < 0)
	{
		if(errno != 0)
		{
			if(errno == EINVAL)
				fputs(" (not supported)\n",stdout);
			else
				printf("sysconf error\n");
		}
		else
		{
			fputs(" (no limit)\n",stdout);
		}
	}
	else
	{
		printf(" %ld\n", val);
	}
}

pr_pathconf(char *mesg, char *path, int name)
{
    long val;
    fputs(mesg, stdout);
    errno=0;
    if((val = pathconf(path, name)) < 0)
	{
	    if(errno != 0)
	    {
	        if(errno == EINVAL)
	            fputs(" (not supported)\n",stdout);
	        else
	            printf("pathconf error\n");
	     }
	     else
	     {
	         fputs(" (no limit)\n",stdout);
	     }
	 }
	 else
	 {
	      printf(" %ld\n", val);
	 }
}

#define PRSYSCONF(name)  pr_sysconf(#name " = ",name)
#define PRPATHCONF(name) pr_pathconf(#name "=","/",name)
int show_sysconf()
{
	printf("*********************************\n");
	PRSYSCONF(_SC_ARG_MAX);
	PRSYSCONF(_SC_CLK_TCK);
	PRSYSCONF(_SC_ATEXIT_MAX);
	PRSYSCONF(_SC_CHILD_MAX);
	PRSYSCONF(_SC_COLL_WEIGHTS_MAX);
	PRSYSCONF(_SC_HOST_NAME_MAX);
	PRSYSCONF(_SC_IOV_MAX);
	PRSYSCONF(_SC_LINE_MAX);
	PRSYSCONF(_SC_LOGIN_NAME_MAX);
	PRSYSCONF(_SC_NGROUPS_MAX);
	PRSYSCONF(_SC_OPEN_MAX);
	PRSYSCONF(_SC_PAGESIZE);
	PRSYSCONF(_SC_PAGE_SIZE);
	PRSYSCONF(_SC_RE_DUP_MAX);
	PRSYSCONF(_SC_STREAM_MAX);
	PRSYSCONF(_SC_SYMLOOP_MAX);
	PRSYSCONF(_SC_TTY_NAME_MAX);
	PRSYSCONF(_SC_TZNAME_MAX);

	PRPATHCONF(_PC_FILESIZEBITS);
	PRPATHCONF(_PC_LINK_MAX);
	PRPATHCONF(_PC_MAX_CANON);
	PRPATHCONF(_PC_MAX_INPUT);
	PRPATHCONF(_PC_NAME_MAX);
	PRPATHCONF(_PC_PATH_MAX);
	PRPATHCONF(_PC_PIPE_BUF);
	PRPATHCONF(_PC_SYMLINK_MAX);
	return 0;
}
