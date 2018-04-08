#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void ouch (int sig)
{
	printf("OUCH! - I got signal %d\n", sig);
}


int main()
{
	struct sigaction act;
	act.sa_handler = ouch;
	act.sa_flags = 0;
	sigfillset(&act.sa_mask);
	//sigaddset(&act.sa_mask, SIGINT);
	sigaction(SIGINT, &act, 0);

	while(1)
	{
		printf("hello\n");
		sleep(1);
	}
	

}
