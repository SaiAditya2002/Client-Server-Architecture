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

int main()
{

	int msqid, len;
	key_t key;

/*	fflush(stdin);*/
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

	while(1)
	{
		struct my_msgbufs buf;
		if(msgrcv(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 1001, 0) == -1)
		{ 
			perror("Error in msgrcv in line 35\n");
			exit(-3);
		}
		
		switch(buf.opt)
		{
			case(1):
			case(2):
				buf.mtype=1002;
				if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs)- sizeof(long), 0) == -1)
				{ 
					perror("Error in msgsnd in line 125\n");
					exit(-3);
				}
				break;
			case(3):
			case(4):
				if(buf.seq%2==0)
				{
					buf.mtype=1003;
					if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs)- sizeof(long), 0) == -1)
					{ 
						perror("Error in msgsnd in line 125\n");
						exit(-3);
					}
				}
				else
				{
					buf.mtype=1004;
					if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs)- sizeof(long), 0) == -1)
					{ 
						perror("Error in msgsnd in line 125\n");
						exit(-3);
					}
				}
				break;
			case(5):
				buf.mtype=1002;
				if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs)- sizeof(long), 0) == -1)
				{ 
					perror("Error in msgsnd in line 125\n");
					exit(-3);
				}
				buf.mtype=1003;
				if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs)- sizeof(long), 0) == -1)
				{ 
					perror("Error in msgsnd in line 125\n");
					exit(-3);
				}
				buf.mtype=1004;
				if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs)- sizeof(long), 0) == -1)
				{ 
					perror("Error in msgsnd in line 125\n");
					exit(-3);
				}
				sleep(5);
				if(msgctl(msqid, IPC_RMID, NULL) == -1)
				{ 
					perror("Error in msgctl in line 124\n");
					exit(-4);
				}
				return 0;
				break;
			default:
	    			printf(" ");
		}
	}
	return 0;
}
