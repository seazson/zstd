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
/*ϵͳ*/
int version(void);
int show_sysconf(void);
int show_passwd(void);
int show_uname(void);
int show_time(void);
/*��������*/
int show_env(void);
int set_env(void);
int get_env(void);
/*ϵͳ��Դ���ֲ�*/
int show_rlimit(void);
int show_usage(void);
int show_valueaddr(void);
/*����*/
int show_ids(void);
int fork_e(void);
int vfork_e(void);
int execle_e(void);
int system_e(void);
/*�Ự����*/
int set_pgid(void);
int set_sid(void);
/*�ź�*/
int alarm_e(void);
int show_sigmask(void);
int show_sigpending(void);
int set_sigmask(void);
int set_sigclr(void);
/*�߳�*/
int pthread_create_e(void);
int pthread_cancel_e(void);
int pthread_lock_e(void);
int pthread_unlock_e(void);
int pthread_sigwait_e(void);
int pthread_kill_e(void);
/*�ػ�����*/
int daemon_e(void);
/*�ļ����¼��*/
int lock_file_e(void);
/*�ܵ�*/
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
	{"version              ��ʾ�汾",          version},
	{"Open path            �򿪻򴴽��ļ�",    Open},
	{"Close                �ر��ļ�",          Close},
	{"Lseek [offset]       ��ʾ|��λ�ļ�λ��", Lseek},
	{"Read [num]           ���ļ�",            Read},
	{"Write data           д�ļ�",            Write},
	{"Fcntl                ��ʾ�ļ�״̬��־",  Fcntl},
	
	{"Stat", Stat},

	{"show_sysconf         ��ʾϵͳ���ü�����",show_sysconf},
	{"show_passwd          ��ʾ�û�",          show_passwd },
	{"show_uname           ��ʾϵͳ��Ϣ",      show_uname},
	{"show_time            ��ʾʱ��",          show_time},
	{"show_env             ��ʾ���л�������",  show_env},
	{"set_env name val     �����ض���������",  set_env},
	{"get_env name         ��ʾ�ض���������",  get_env},
	{"show_rlimit          ��ʾ��������",      show_rlimit},
	{"show_usage           ��ʾ����ռ��ʱ��",  show_usage},
	{"show_valueaddr       ��ʾ���̿ռ����",  show_valueaddr},
	{"show_ids             ��ʾ����idֵ",      show_ids},

	{"fork                 fork����",          fork_e},
	{"vfork                vfork����",         vfork_e},
	{"execle cmd           execle����",        execle_e},
	{"system cmd           fork+execle����",   system_e},

	{"set_pgid             ���õ�ǰ����Ϊ�鳤",set_pgid},
	{"set_sid              ���ûỰ",          set_sid},

	{"alarm_e              ������ʱ����",      alarm_e},
	{"show_sigmask         ��ʾ��ǰ��������",  show_sigmask},
	{"show_sigpending      ��ʾ����δ���ź�",  show_sigpending},
	{"set_sigmask signom   ��ӵ�ǰ��������",  set_sigmask},
	{"set_sigclr           �����ǰ��������",  set_sigclr},

	{"pthread_create       �����߳�",          pthread_create_e},
	{"pthread_cancel nom   ȡ��ĳ���߳�",      pthread_cancel_e},
	{"pthread_lock         ���߳�����",        pthread_lock_e},
	{"pthread_unlock       �����߳�ͬ��",      pthread_unlock_e},
	{"pthread_sigwait      �߳��еȴ��ź�",    pthread_sigwait_e},
	{"pthread_kill         ���̷߳��ź�",      pthread_kill_e},

	{"daemon               �ػ�����",          daemon_e},

	{"lock_file            �ļ���",            lock_file_e},

	{"pipe_create          ���ӹܵ�",          pipe_create},
	{"fifo_create          ����fifo",          fifo_create},
	{"fifo_read            ��fifo",            fifo_read},
	{"fifo_write           дfifo",            fifo_write},

	{"ipcsem_init          ��ʼ��IPC�ź���",   ipcsem_init},
	{"ipcsem_p             ��ȡ�ź��� -1",     ipcsem_p},
	{"ipcsem_v             �ͷ��ź��� +1",     ipcsem_v},
	{"ipcsem_del           ɾ���ź���",        ipcsem_del},
	{"ipcmsg_recv          ��ȡ��Ϣ��������",  ipcmsg_recv},
	{"ipcmsg_send          ����Ϣ���д�������",ipcmsg_send},
	{"ipcshm_read          �������ڴ�����",    ipcshm_read},
	{"ipcshm_write         д�����ڴ�����",    ipcshm_write},

	{"server_unix          unix�������", server_unix},
	{"client_unix          unix��ͻ���", client_unix},
	{"show_hostinfo        ��ʾ������Ϣ", show_hostinfo},

	{"c_adv                C����",       c_adv},
	
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
