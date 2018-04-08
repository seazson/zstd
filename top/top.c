#include <stdio.h>


typedef struct jiffy_counts_t {
	unsigned long long usr,nic,sys,idle,iowait,irq,softirq,steal;
	unsigned long long total;
	unsigned long long busy;
} jiffy_counts_t;

jiffy_counts_t prev_jif,jif;

int main()
{
	FILE* fp;
	unsigned long long tot;

	while(1)
	{
		fp = fopen("/proc/stat", "r");
		if (fscanf(fp, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld",
				&prev_jif.usr,&prev_jif.nic,&prev_jif.sys,&prev_jif.idle,
				&prev_jif.iowait,&prev_jif.irq,&prev_jif.softirq,&prev_jif.steal) < 4) {
				printf("read error\n");
		}
		fclose(fp);
	
		sleep(1);
		
		fp = fopen("/proc/stat", "r");
		if (fscanf(fp, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld",
				&jif.usr,&jif.nic,&jif.sys,&jif.idle,
				&jif.iowait,&jif.irq,&jif.softirq,&jif.steal) < 4) {
				printf("read error\n");
		}
		fclose(fp);

		printf("\e[H\e[J");
		printf("user:%lld nic:%lld sys:%lld idle:%lld io:%lld irq:%lld softirq:%lld steal:%lld\n",			
				prev_jif.usr,prev_jif.nic,prev_jif.sys,prev_jif.idle,
				prev_jif.iowait,prev_jif.irq,prev_jif.softirq,prev_jif.steal);
		printf("user:%lld nic:%lld sys:%lld idle:%lld io:%lld irq:%lld softirq:%lld steal:%lld\n",			
				jif.usr,jif.nic,jif.sys,jif.idle,
				jif.iowait,jif.irq,jif.softirq,jif.steal);
		
		tot = (jif.usr+jif.nic+jif.sys+jif.idle+jif.iowait+jif.irq+jif.softirq+jif.steal)-
					(prev_jif.usr+prev_jif.nic+prev_jif.sys+prev_jif.idle+prev_jif.iowait+prev_jif.irq+prev_jif.softirq+prev_jif.steal);
		printf("%%user:%lld nic:%lld sys:%lld idle:%lld io:%lld irq:%lld softirq:%lld steal:%lld\n\n",			
				(jif.usr-prev_jif.usr)*100/tot,
				(jif.nic-prev_jif.nic)*100/tot,
				(jif.sys-prev_jif.sys)*100/tot,
				(jif.idle-prev_jif.idle)*100/tot,
				(jif.iowait-prev_jif.iowait)*100/tot,
				(jif.irq-prev_jif.irq)*100/tot,
				(jif.softirq-prev_jif.softirq)*100/tot,
				(jif.steal-prev_jif.steal)*100/tot);
	}
			
}
