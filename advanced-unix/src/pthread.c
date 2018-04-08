#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

extern char *cmd_argv[10];
pthread_t ptid[256];
unsigned int cur_tid=0;
static int lockcnt=0;
pthread_mutex_t mutex;

static pthread_key_t key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;


/*�Զ�������*/
void cleanup(void *arg)
{
	printf("cleanup : %s\n", (char *)arg);	
}

/*�������������Գ�ʼ��*/
void init_mutex(pthread_mutex_t *mutex)
{
	pthread_mutexattr_t mutexaddr;

  pthread_mutexattr_init(&mutexaddr);
	pthread_mutexattr_setpshared(&mutexaddr, PTHREAD_PROCESS_PRIVATE);

/*���������Ա����¶�����
	PTHREAD_MUTEX_NORMAL = PTHREAD_MUTEX_TIMED_NP, 
  PTHREAD_MUTEX_RECURSIVE = PTHREAD_MUTEX_RECURSIVE_NP, 
  PTHREAD_MUTEX_ERRORCHECK = PTHREAD_MUTEX_ERRORCHECK_NP, 
  PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL*/ 
	pthread_mutexattr_settype(&mutexaddr, PTHREAD_MUTEX_RECURSIVE_NP);

	pthread_mutex_init(mutex,&mutexaddr);
	
	pthread_mutexattr_destroy(&mutexaddr);	
}

/*��ʼ��˽������*/
void init_pthread()
{
		printf("first init pthread\n");
		init_mutex(&mutex);
		pthread_key_create(&key, cleanup);
}

/*��˽�����ݵ�����*/
void get_mydata(void)
{
	char *buf;
	buf = (char *)pthread_getspecific(key);
	if(buf == NULL)
	{
		printf("no private data\n");
	}
	printf("read buf %s\n",buf);
}

/*�̺߳���*/
void * pthread_func(void *arg)
{
	char *buf;
	pthread_t self_tid;
	self_tid = pthread_self();

	printf("pthread 0x%x running\n",self_tid);
	pthread_once(&init_done, init_pthread);

/*�ҽ��߳���������*/
	pthread_cleanup_push(cleanup, "first cleanup");
	pthread_cleanup_push(cleanup, "second cleanup");

/*����˽������*/
	buf = (char *)pthread_getspecific(key);
	if(buf == NULL)
	{
		printf("malloc new buf\n");
		buf = malloc(256);
		pthread_setspecific(key, buf);
		sprintf(buf,"buf = %x\n",self_tid);
	}
	printf("read buf %s\n",buf);
	
/*�����߳�ȡ��״̬������*/
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); /*�첽���õȴ�ȡ����*/
	
	while(1)
	{
		pthread_mutex_lock(&mutex);
		printf("get lock\n");
		get_mydata();
		pthread_mutex_unlock(&mutex);
		sleep(5);
	}
	
/*ֻ��Ϊ��ƥ�䣬��������*/
	pthread_cleanup_pop(0);   
	pthread_cleanup_pop(0);
	
/*exit(0); ��ʹ����������ֹ*/

	return ((void *)0);
}

int pthread_create_e(void)
{
  pthread_t self_tid;
	pthread_attr_t attr;
	size_t stacksize;
	size_t guardsize;
	unsigned int stackaddr;
	
/*�����̵߳�����*/	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

/*��ȡջ��С�;��仺������С*/
//	pthread_attr_getstack(&attr, &stackaddr, &stacksize);	
	pthread_attr_getstacksize(&attr, &stacksize);
	pthread_attr_getguardsize(&attr, &guardsize);
//	printf("starcaddr = %x\n", stackaddr);
	printf("stacksize = %x\n", stacksize);
	printf("guardsize = %x\n", guardsize);
	
	pthread_create(ptid+cur_tid, &attr, pthread_func, NULL);
	cur_tid++;
	sleep(1);

  self_tid = pthread_self();
  printf("main 0x%x run pthread %d\n",self_tid,cur_tid);

/*�߳���������֮��Ҫ����*/
	pthread_attr_destroy(&attr);
	
  return 0;
}

int pthread_cancel_e(void)
{
	printf("cancel pthread %s\n",cmd_argv[1]);
	pthread_cancel(ptid[atoi(cmd_argv[1])]);
	return 0;	
}

int pthread_lock_e(void)
{
	printf("lock %d\n",++lockcnt);
	pthread_mutex_lock(&mutex);
	return 0;		
}

int pthread_unlock_e(void)
{
	printf("unlock %d\n",lockcnt--);
	pthread_mutex_unlock(&mutex);
	return 0;		
}

int pthread_kill_e(void)
{
	printf("Send signal %s to pthread %s\n", cmd_argv[2], cmd_argv[1]);
	pthread_kill(ptid[atoi(cmd_argv[1])],atoi(cmd_argv[2]));
	return 0;	
}


void * pthread_sigfunc(void *arg)
{
	int signo;
	pthread_t self_tid = pthread_self();
	sigset_t sigset;
	sigfillset(&sigset);
	sigdelset(&sigset,17);/*ȡ���ӽ����ϱ�*/
	
	printf("sigwait is running\n");
	
	while(1)
	{
		sigwait(&sigset, &signo);
		printf("pthread %d siganl %d : %s\n",self_tid, signo, sys_siglist[signo]);
	}
	
	return ((void *)0);
}

int pthread_sigwait_e(void)
{
  pthread_t self_tid;
	pthread_attr_t attr;
/*
	sigset_t mask, oldmask;
	int signo;
	sigemptyset(&mask);
	sigaddset(&mask,SIGQUIT);
	pthread_sigmask(SIG_BLOCK, &mask, &oldmask);	
	sigwait(&mask, &signo);*/

/*�����̵߳�����*/	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_create(ptid+cur_tid, &attr, pthread_sigfunc, NULL);
	cur_tid++;
	sleep(1);

  self_tid = pthread_self();
  printf("main 0x%x run pthread %d\n",self_tid,cur_tid);

/*�߳���������֮��Ҫ����*/
	pthread_attr_destroy(&attr);

  return 0;
	
}
