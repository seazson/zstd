#include <sys/types.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	uid_t uid;
	gid_t gid;
	struct passwd *pw;
	char computer[256];
	struct utsname uts;

	uid = getuid();
	gid = getgid();

	printf("User is %s\n", getlogin());
	printf("User IDs: uid=%d gid=%d\n", uid, gid);

	pw = getpwuid(uid);

        printf("UID passwd entry:\n name=%s, uid=%d, gid=%d, home=%s, shell=%s\n",
                pw->pw_name, pw->pw_uid, pw->pw_gid, pw->pw_dir, pw->pw_shell);

	pw = getpwnam("root");
	
        printf("root passwd entry:\n name=%s, uid=%d, gid=%d, home=%s, shell=%s\n",
                pw->pw_name, pw->pw_uid, pw->pw_gid, pw->pw_dir, pw->pw_shell);

	gethostname(computer, 255);
	uname(&uts);

	printf("%s\n",computer);
	printf("%s,%s,%s,%s,%s\n",uts.sysname, uts.machine, uts.nodename, uts.release, uts.version);	

	exit(0);
}
