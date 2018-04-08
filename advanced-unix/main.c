#include <stdio.h>
#include <string.h>
#include <unistd.h>
/**************************************************/
/*io*/
int Open(void);
int Close(void);
int Lseek(void);
int Read(void);
int Write(void);
int Fcntl(void);
/*file dir*/
int Stat(void);
/*系统*/
int version(void);
int show_sysconf(void);
int show_passwd(void);
int show_uname(void);
int show_time(void);
/*环境变量*/
int show_env(void);
int set_env(void);
int get_env(void);
/*系统资源及分布*/
int show_rlimit(void);
int show_usage(void);
int show_valueaddr(void);
/*进程*/
int show_ids(void);
int fork_e(void);
int vfork_e(void);
int execle_e(void);
int system_e(void);
/*会话和组*/
int set_pgid(void);
int set_sid(void);
/*信号*/
int alarm_e(void);
int show_sigmask(void);
int show_sigpending(void);
int set_sigmask(void);
int set_sigclr(void);
/*线程*/
int pthread_create_e(void);
int pthread_cancel_e(void);
int pthread_lock_e(void);
int pthread_unlock_e(void);
int pthread_sigwait_e(void);
int pthread_kill_e(void);
/*守护进程*/
int daemon_e(void);
/*文件或记录锁*/
int lock_file_e(void);
/*管道*/
int pipe_create(void);
int fifo_create(void);
int fifo_read(void);
int fifo_write(void);
/*IPC*/
int ipcsem_init(void);
int ipcsem_p(void);
int ipcsem_v(void);
int ipcsem_del(void);
int ipcmsg_recv(void);
int ipcmsg_send(void);
int ipcshm_read(void);
int ipcshm_write(void);
/*net*/
int server_unix(void);
int client_unix(void);
int show_hostinfo(void);

int c_adv(void);

/***************************************************/
char console_buffer[256];
char *cmd_argv[10];
int  cmd_argc;
char cmd_buf[10] = "012345678";

struct menu_entry
{
	char *name;
	int (*func)(void);
} menu[] =
{
	{"version              显示版本",          version},
	{"Open path            打开或创建文件",    Open},
	{"Close                关闭文件",          Close},
	{"Lseek [offset]       显示|定位文件位置", Lseek},
	{"Read [num]           读文件",            Read},
	{"Write data           写文件",            Write},
	{"Fcntl                显示文件状态标志",  Fcntl},
	
	{"Stat", Stat},

	{"show_sysconf         显示系统配置及限制",show_sysconf},
	{"show_passwd          显示用户",          show_passwd },
	{"show_uname           显示系统信息",      show_uname},
	{"show_time            显示时间",          show_time},
	{"show_env             显示所有环境变量",  show_env},
	{"set_env name val     设置特定环境变量",  set_env},
	{"get_env name         显示特定环境变量",  get_env},
	{"show_rlimit          显示进程限制",      show_rlimit},
	{"show_usage           显示进程占用时间",  show_usage},
	{"show_valueaddr       显示进程空间分配",  show_valueaddr},
	{"show_ids             显示各种id值",      show_ids},

	{"fork                 fork调用",          fork_e},
	{"vfork                vfork调用",         vfork_e},
	{"execle cmd           execle调用",        execle_e},
	{"system cmd           fork+execle调用",   system_e},

	{"set_pgid             设置当前进程为组长",set_pgid},
	{"set_sid              设置会话",          set_sid},

	{"alarm_e              产生定时闹铃",      alarm_e},
	{"show_sigmask         显示当前进程屏蔽",  show_sigmask},
	{"show_sigpending      显示进程未决信号",  show_sigpending},
	{"set_sigmask signom   添加当前进程屏蔽",  set_sigmask},
	{"set_sigclr           清除当前进程屏蔽",  set_sigclr},

	{"pthread_create       创建线程",          pthread_create_e},
	{"pthread_cancel nom   取消某个线程",      pthread_cancel_e},
	{"pthread_lock         给线程上锁",        pthread_lock_e},
	{"pthread_unlock       解锁线程同步",      pthread_unlock_e},
	{"pthread_sigwait      线程中等待信号",    pthread_sigwait_e},
	{"pthread_kill         向线程发信号",      pthread_kill_e},

	{"daemon               守护进程",          daemon_e},

	{"lock_file            文件锁",            lock_file_e},

	{"pipe_create          父子管道",          pipe_create},
	{"fifo_create          创建fifo",          fifo_create},
	{"fifo_read            读fifo",            fifo_read},
	{"fifo_write           写fifo",            fifo_write},

	{"ipcsem_init          初始化IPC信号量",   ipcsem_init},
	{"ipcsem_p             获取信号量 -1",     ipcsem_p},
	{"ipcsem_v             释放信号量 +1",     ipcsem_v},
	{"ipcsem_del           删除信号量",        ipcsem_del},
	{"ipcmsg_recv          获取消息队列数据",  ipcmsg_recv},
	{"ipcmsg_send          向消息队列传送数据",ipcmsg_send},
	{"ipcshm_read          读共享内存数据",    ipcshm_read},
	{"ipcshm_write         写共享内存数据",    ipcshm_write},

	{"server_unix          unix域服务器", server_unix},
	{"client_unix          unix域客户端", client_unix},
	{"show_hostinfo        显示主机信息", show_hostinfo},

	{"c_adv                C语言",       c_adv},
	
	{NULL,NULL}
};


int version()
{
	int i;
	printf("%s %s\n",__DATE__,__TIME__);
	for(i=0;i<cmd_argc; i++)
	{
		printf("%d : %s\n",i,cmd_argv[i]);	
	}
	return 0;
}

int show_menu()
{
	int list;
    printf("********************************************\n");
    printf("*                 UNIX STD                 *\n");
    printf("********************************************\n");
	for(list=0; menu[list].name!=NULL ;list++)
	{
		printf("%02d : %s\n",list,menu[list].name);
	}
	return 0;
}

int main(int argc, char * argv[])
{
	int len;
	int list;
	char *p=" ";
	
	show_menu();
	while(1)	
	{
		printf("============================================\n");
		printf("Enter your word : ");	
		len = get_cmdline(console_buffer);
		if(len==-1)
			return 0;

		printf("You enter %d words : %s\n",len, console_buffer);
		printf("----------------------------------------->>>\n");
		cmd_argv[0] = strtok(console_buffer,p);
		cmd_argc=0;
		while(cmd_argv[(cmd_argc++)+1]=strtok(NULL," "));
		
		if(strcmp(console_buffer,"?") == 0)
		{
			show_menu();
		}
		if(strcmp(console_buffer,"quit") == 0)
		{
			return 0;
		}
        
		if(console_buffer[0]>='0' && console_buffer[0]<='9')
	    {
		    menu[atoi(console_buffer)].func();
		    continue;	
		}		
		
		for(list=0; menu[list].name != NULL; list++)
		{
			if(strcmp(console_buffer,menu[list].name) == 0)
			{
				menu[list].func();
			}
		}
	}
	return 0;
}
