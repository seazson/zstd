#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

struct shared_use_st{
	int written_by_you;
	char some_text[2048];
};

int ipcshm_read(void)
{
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;
	int shmid;

	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666| IPC_CREAT);
	if(shmid == -1)
	{
		printf("shmget err\n");
		return 1;
	}

	shared_memory = shmat(shmid, (void *)0, 0);
	if(shared_memory == (void *)-1)
	{
		printf("shmat err\n");
		return 1;
	}
	printf("Memory attached at %X\n", (int)shared_memory);

	shared_stuff = (struct shared_use_st *)shared_memory;
	shared_stuff->written_by_you = 0;

	while(1)
	{
		if(shared_stuff->written_by_you)
		{
			printf("You writes : %s", shared_stuff->some_text);
			sleep(1);
			shared_stuff->written_by_you = 0;
		}
	}

	if(shmdt(shared_memory) == -1)
	{
		printf("shmdt err\n");
		return 1;
	}

	if(shctl(shmid, IPC_RMID, 0) == -1)
	{
		printf("shctl err\n");
		return 1;
	}

	return 0;
}

int ipcshm_write(void)
{
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;
	int shmid;
	char buf[2048];
	
	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666| IPC_CREAT);
	if(shmid == -1)
	{
		printf("shmget err\n");
		return 1;
	}

	shared_memory = shmat(shmid, (void *)0, 0);
	if(shared_memory == (void *)-1)
	{
		printf("shmat err\n");
		return 1;
	}
	printf("Memory attached at %X\n", (int)shared_memory);
	
	shared_stuff = (struct shared_use_st *)shared_memory;
	
	while(1)
	{
		printf("Enter some text: ");	
		fgets(buf, 2048, stdin);
		strcpy(shared_stuff->some_text, buf);
		shared_stuff->written_by_you = 1;
	}

	if(shmdt(shared_memory) == -1)
	{
		printf("shmdt err\n");
		return 1;
	}

	return 0;	
	
}
	
