#include <unistd.h>
#include <stdio.h>
#include <signal.h>

extern char *cmd_argv[10];
int show_sigmask(void);
void pr_siglist(sigset_t *sigset);
/*****************************************************
  *
  *             信号处理函数
  *
******************************************************/
typedef void Sigfunc(int);
Sigfunc * signal_sea(int signo, Sigfunc *func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask,SIGABRT);
	act.sa_flags = 0;
//	act.sa_flags |= SA_INTERRUPT;
	act.sa_flags |= SA_RESTART;
	sigaction(signo, &act, &oact);
  show_sigmask();
	return(oact.sa_handler);
}

/*****************************************************
  *
  *             产生闹钟信号
  *
******************************************************/
static void alarm_fun(int signo)
{
	printf("siganl %d : %s\n",signo,sys_siglist[signo]);
	show_sigmask();
}

int alarm_e()
{
	signal_sea(SIGALRM, alarm_fun);
	alarm(3);
	return 0;
}
/*****************************************************
  *
  *             打印屏蔽字
  *
******************************************************/
#define PRSIGMASK(name) if(sigismember(sigset,name)) printf(#name "\n");
void pr_siglist(sigset_t *sigset)
{
	printf("*****************signal masked*****************\n");
	PRSIGMASK(SIGABRT);
	PRSIGMASK(SIGALRM);
	PRSIGMASK(SIGBUS);
	//PRSIGMASK(SIGCANCEL);
	PRSIGMASK(SIGCHLD);
	PRSIGMASK(SIGCONT);
	//PRSIGMASK(SIGEMT);
	PRSIGMASK(SIGFPE);
	//PRSIGMASK(SIGFREEZE);
	PRSIGMASK(SIGHUP);
	PRSIGMASK(SIGILL);
	//PRSIGMASK(SIGINFO);
	PRSIGMASK(SIGINT);
	PRSIGMASK(SIGIO);
	PRSIGMASK(SIGIOT);
	PRSIGMASK(SIGKILL);
	//PRSIGMASK(SIGLWP);
	PRSIGMASK(SIGPIPE);
	PRSIGMASK(SIGPOLL);
	PRSIGMASK(SIGPROF);
	PRSIGMASK(SIGPWR);
	PRSIGMASK(SIGQUIT);
	PRSIGMASK(SIGSEGV);
	PRSIGMASK(SIGSTKFLT);
	PRSIGMASK(SIGSTOP);
	PRSIGMASK(SIGSYS);
	PRSIGMASK(SIGTERM);
	//PRSIGMASK(SIGTHAW);
	PRSIGMASK(SIGTRAP);
	PRSIGMASK(SIGTSTP);
	PRSIGMASK(SIGTTIN);
	PRSIGMASK(SIGTTOU);
	PRSIGMASK(SIGURG);
	PRSIGMASK(SIGUSR1);
	PRSIGMASK(SIGUSR2);
	PRSIGMASK(SIGVTALRM);
	//PRSIGMASK(SIGWAITING);
	PRSIGMASK(SIGWINCH);
	PRSIGMASK(SIGXCPU);
	PRSIGMASK(SIGXFSZ);
	//PRSIGMASK(SIGXRES);
}

/*显示当前进程中的信号屏蔽字*/
int show_sigmask(void)
{
	sigset_t sigset;
	sigprocmask(0, NULL, &sigset);
	pr_siglist(&sigset);
	return 0;	
}

/*显示当前进程中未决的信号*/
int show_sigpending(void)
{
	sigset_t sigset;
	sigpending(&sigset);
	pr_siglist(&sigset);
	return 0;
}

/*在当前进程中添加信号屏蔽字*/
int set_sigmask(void)
{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset,atoi(cmd_argv[1]));
	
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	return 0;	
}

/*清除当前进程中的信号屏蔽字*/
int set_sigclr(void)
{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigprocmask(SIG_SETMASK, &sigset, NULL);
	return 0;	
}
