#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/msg.h>

#define PERMS 0644
struct my_msgbufs{
	long mtype;
	long seq;
	char mtext[200];
	int opt;
};

int main(){
	struct my_msgbufs buf;
	int msqid;
	key_t key;
	if((key = ftok("load_balancer.c", 'A'))==-1)
	{ 
		perror("Error in ftok in line 20\n");
		exit(-1);
	}
	if((msqid = msgget(key, PERMS | IPC_CREAT))==-1)
	{ 
		perror("Error in msgget in line 25\n");
		exit(-2);
	}
	char c;
	while(1)
	{
		printf("Want to terminate the application? Press Y (Yes) or N (No)\n");
		scanf("%c", &c);
		if(c=='N')
		{
			continue;
		}
		else if(c=='Y')
		{
			buf.mtype=1001;
			buf.opt=5;
			if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 0) == -1)
				{ 
					perror("Error in msgsnd in line 45\n");
					exit(-3);
				}
			return 0;
		}
	}
}
