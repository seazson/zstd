#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

extern char *cmd_argv[10];

int Stat()
{
	struct stat buf;
	mode_t old_mask;
	char   tmpbuf[256];
	
	lstat(cmd_argv[1], &buf);

	printf("file type : ");
	if(S_ISREG(buf.st_mode))
		printf("regular\n");
	else if(S_ISDIR(buf.st_mode))
		printf("dir\n");
	else if(S_ISCHR(buf.st_mode))
		printf("char\n");
	else if(S_ISBLK(buf.st_mode))
		printf("block\n");
	else if(S_ISFIFO(buf.st_mode))
		printf("fifo\n");
	else if(S_ISLNK(buf.st_mode))
		printf("link\n");
	else if(S_ISSOCK(buf.st_mode))
		printf("socket\n");

	printf("mode      = 0x%x\n",   buf.st_mode);
	printf("user id   = 0x%x\n",   buf.st_uid);
	printf("grp  id   = 0x%x\n",   buf.st_gid);
	printf("file size = 0x%x\n",   buf.st_size);
	printf("blksize   = 0x%x\n",   buf.st_blksize);
	printf("blknum    = 0x%x\n",   buf.st_blocks);
	printf("atime     = %s\n",     ctime(&buf.st_atime));
	printf("mtime     = %s\n",     ctime(&buf.st_mtime));
	printf("ctime     = %s\n",     ctime(&buf.st_ctime));
	printf("dev       = %d/%d\n",    major(buf.st_dev),minor(buf.st_dev));
	if(S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode))
		printf("rdev      = %d/%d\n", major(buf.st_rdev),minor(buf.st_rdev));

	utime(cmd_argv[1], NULL);
	printf("change move time\n");
	
	old_mask = umask(0);
	printf("mask mode = 0x%x\n",old_mask);
	umask(old_mask);
	
	getcwd(tmpbuf, 256);
	printf("pwd       = %s\n",tmpbuf);
	
		
	return 0;
}
