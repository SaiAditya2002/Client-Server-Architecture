#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/shm.h>

#define BUF_SIZE 1000
#define PERMS 0644
struct my_msgbufs{
	long mtype;
	long seq;
	char mtext[200];
	int opt;
};

int main(){
	struct my_msgbufs buf;
	int shmid; 
	char *shmPtr;
	int msqid, len;
	key_t key1, key2;
	if((key1 = ftok("load_balancer.c", 'A'))==-1)
	{ 
		perror("Error in ftok in line 20\n");
		exit(-1);
	}
	
	if((msqid = msgget(key1, PERMS | IPC_CREAT))==-1)
	{ 
		perror("Error in msgget in line 25\n");
		exit(-2);
	}
	char temp_str[BUF_SIZE];
	while(1)
	{
		long cid;
		printf("Enter Sequence Number:\n");
		scanf("%ld", &cid);
/*		printf("se no. : %ld", cid );*/
		buf.seq=cid;
		if((key2 = ftok("load_balancer.c", cid))==-1)
		{ 
			perror("Error in ftok in line 20\n");
			exit(-1);
		}
		buf.mtype=1001;
		int x;
		printf("1. Add a new graph to the database\n2. Modify an existing graph of the database\n3. Perform DFS on an existing graph of the database\n4. Perform BFS on an existing graph of the database\n");
		printf("Enter Operation Number:\n");
		scanf("%d", &x);
		buf.opt=x;
		printf("Enter Graph File Name:\n");
		scanf("%s", buf.mtext);
		shmid=shmget(key2, BUF_SIZE, PERMS | IPC_CREAT);
		if(shmid==-1)
		{
			perror("Error in shmget in line 54\n");
			exit(-1);
		}
		shmPtr=shmat(shmid, NULL, 0);
		if(shmPtr==(void*)-1)
		{
			perror("Error in shmPtr in line 60\n");
			exit(-2);
		}
		switch(x)
		{
			case(1):
			case(2):
				printf("Enter number of nodes of the graph:\n");
				int n;
				scanf("%d", &n);
				sprintf(shmPtr, "%d\n", n);
				printf("Enter adjacency matrix, each row on a separate line and elements of a single row separated by whitespace characters:\n");
/*				char temp_str[2*(n*n+1)];*/
				for(int i=0;i<n;i++)
				{
					for(int i=0;i<n;i++)
					{	int x;
						scanf("%d", &x);
						sprintf(temp_str, "%d ", x);
						strcat(shmPtr, temp_str);
					}
					strcat(shmPtr, "\n");
				}
/*				printf("\n%s\n",shmPtr);*/
			break;
			case(3):
			case(4):
				printf("Enter starting vertex:\n");
/*				scanf("%c", &shmPtr[0]);*/
				int y;
				scanf("%d", &y);
				sprintf(shmPtr, "%d\n", y);
			break;	
		}
		
		if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 0) == -1)
		{ 
			perror("Error in msgsnd in line 44\n");
			exit(-3);
		}
		memset(buf.mtext, '\0', sizeof(buf.mtext));
		if(msgrcv(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), cid*1000, 0) == -1)
		{ 
			perror("Error in msgrcv in line 35\n");
			exit(-3);
		}
		printf("Msg Receieved: %s \n", buf.mtext);
		if(shmdt(shmPtr)==-1)
		{
			perror("Error in shmdt in line 101\n");
			exit(-6);
		}
		
		
	}
	return 0;
}
