#include <unistd.h>
#include <stdio.h>
#include <sys/sem.h>

static int sem_id;

/*这个联合的定义需要程序员自己定义，可以参考man semctl帮助*/
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short  *array;
};

int semaphore_p(void)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op  = -1;
	sem_b.sem_flg = SEM_UNDO;

	if(semop(sem_id, &sem_b, 1) == -1)
	{
		printf("semphore_p err\n");
		return 0;
	}
	return 1;
}

int semaphore_v(void)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1;
	sem_b.sem_flg = SEM_UNDO;
	
    if(semop(sem_id, &sem_b, 1) == -1)
	{
	    printf("semphore_p err\n");
	    return 0;
	}
	return 1;
}

int set_semvalue(void)
{
	union semun sem_union;
	sem_union.val = 5;
	if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
		return 0;
	return 1;
}

void del_semvalue(void)
{
	union semun sem_union;
	if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
		printf("del semaphore err\n");
}

char *cmd_argv[10];

int ipcsem_init(void)
{
	sem_id = semget((key_t)1234, 1, 0666 | IPC_CREAT);
	if(atoi(cmd_argv[1]) == 1)
	{
		set_semvalue();
		printf("first init ipcsem\n");	
	}
	return 0; 	
}

int ipcsem_p(void)
{
	semaphore_p();
	printf("get ipcsem\n");	
	return 0;
}

int ipcsem_v(void)
{
	semaphore_v();
	printf("put ipcsem\n");
	return 0;
}

int ipcsem_del(void)
{
	del_semvalue();
	printf("del ipcsem\n");
	return 0;	
}
