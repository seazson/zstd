#include <unistd.h>
#include <stdio.h>
#include <pwd.h>

void pr_passwd(struct passwd *user)
{
	printf("%s %s %d %d %s %s %s \n",user->pw_name, user->pw_passwd, user->pw_uid, user->pw_gid,
									 user->pw_gecos, user->pw_dir, user->pw_shell);
}

int show_passwd()
{
	struct passwd *ptr;
	printf("************************************\n");
	setpwent();
	while((ptr = getpwent()) != NULL)
	{
		pr_passwd(ptr);
	}
	endpwent();

	printf("************************************\n");
	pr_passwd(getpwnam("zengyang"));
	printf("current user:");
	pr_passwd(getpwuid(getuid()));
	return 0;
}
