#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define read_lock(fd, offset, whence, len) \
					lock_reg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define readw_lock(fd, offset, whence, len) \
	                lock_reg((fd), F_SETLKW, F_RDLCK, (offset), (whence), (len))
#define write_lock(fd, offset, whence, len) \
	                lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define writew_lock(fd, offset, whence, len) \
	                lock_reg((fd), F_SETLKW, F_WRLCK, (offset), (whence), (len))
#define un_lock(fd, offset, whence, len) \
	                lock_reg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))

#define is_read_lockable(fd, offset, whence, len) \
									(lock_test((fd), F_RDLCK, (offset), (whence), (len)) == 0)
#define is_write_lockable(fd, offset, whence, len) \
									(lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;

	lock.l_type = type;
	lock.l_start = offset;
	lock.l_whence = whence;
	lock.l_len = len;

	return (fcntl(fd, cmd, &lock));
}

pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;
	
	lock.l_type = type;
	lock.l_start = offset;
	lock.l_whence = whence;
	lock.l_len = len;
	lock.l_pid = 0;

	if(fcntl(fd, F_GETLK, &lock) < 0)
		printf("fcntl err\n");
	
	if(lock.l_pid != 0)
	{
		printf("file is locked by %d\n",lock.l_pid);
		return lock.l_pid;		
	}
		
	if(lock.l_type == F_UNLCK)
	{
		printf("file is not locked\n");
		return 0 ;	
	}
	
	printf("file is locked by %d\n",lock.l_pid);
	return lock.l_pid;		
}

int lockfile(int fd)
{
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return (fcntl(fd,F_SETLK,&fl));	
}

int lock_file_e(void)
{
	int fd;
	pid_t pid;
	
	if((fd = open("tmpfile", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0 )
		printf("open err\n");
	
	if(is_write_lockable(fd, 0, SEEK_SET, 0) == 0)
		write_lock(fd, 0, SEEK_SET, 0);
	
	printf("now we lock the file\n");
	
	return 0;
					
}

