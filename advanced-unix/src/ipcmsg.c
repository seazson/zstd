#include <stdio.h>
#include <unistd.h>
#include <sys/msg.h>

struct my_msg_st
{
	long int my_msg_type;
	char some_text[512];
};

/*从消息队列中接收消息*/
int ipcmsg_send(void)
{
	struct my_msg_st some_data;
	int msgid;
	char buf[512];

	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
	if(msgid == -1)
	{
		printf("msgget err\n");
	}

	printf("Enter some text: ");
	fgets(buf,512,stdin);
	some_data.my_msg_type = 1;
	strcpy(some_data.some_text, buf);

	if(msgsnd(msgid, (void*)&some_data, 512, 0) == -1)
	{
		printf("send err\n");
	}

	return 0;
}

/*向消息队列中发送消息*/
int ipcmsg_recv(void)
{
	struct my_msg_st some_data;
	int msgid;
	char buf[512];
	long int msg_to_receive = 0;
	
	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
	if(msgid == -1)
	{   
		printf("msgget err\n");
		return 1;
	}
	while(1)
	{
		if(msgrcv(msgid, (void *)&some_data, 512, msg_to_receive, 0) == -1 )
		{
			printf("recv err\n");
			return 1;
		}
		printf("Your enter : %s",some_data.some_text);	
	}
	msgctl(msgid, IPC_RMID, 0);
	
	return 0;
}

