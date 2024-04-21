#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/shm.h>
#include <pthread.h>
#include<semaphore.h>
#include <fcntl.h>

#define SEM_NAME1 "/my_semaphore1"
#define SEM_NAME2 "/my_semaphore2"
#define BUF_SIZE 1000
#define PERMS 0644
struct my_msgbufs{
	long mtype;
	long seq;
	char mtext[200];
	int opt;
};


void *func(void* arg)
{
/*	static int val=1;*/
	int shmid; 
	char *shmPtr;
	int msqid;
	key_t key, key2;
	struct my_msgbufs buf = *(struct my_msgbufs*)arg;
	char temp1[20]="/";
	strncat(temp1, buf.mtext, 3);
	strcat(temp1, "r");
	sem_t* write1 = sem_open(temp1, O_CREAT, PERMS, 1);
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
	if((key2 = ftok("load_balancer.c", buf.seq))==-1)
	{ 
		perror("Error in ftok in line 20\n");
		exit(-1);
	}
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

	if(buf.opt==1)
	{
/*		printf("%d\n", (int)buf.mtext[1]-48);*/
		sem_wait(write1);
/*		sem_wait(&mutex[(int)buf.mtext[1]-48]); // enter critical section*/
		FILE *fptr;
		fptr = fopen(buf.mtext, "w");
		fprintf(fptr,"%s",shmPtr);
		fclose(fptr);
		sem_post(write1);
/*		sem_post(&mutex[(int)buf.mtext[1]-48]); //Exit critical section*/
		memset(buf.mtext, '\0', sizeof(buf.mtext));
		strcpy(buf.mtext, "File successfully added\n");
		buf.mtype=buf.seq*1000;
		if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 0) == -1)
		{ 
			perror("Error in msgsnd in line 44\n");
			exit(-3);
		}
	}
	else if(buf.opt==2)
	{
		sem_wait(write1);
/*		sem_wait(&mutex[(int)buf.mtext[1]-48]); // enter critical section*/
		FILE *fptr;
		fptr = fopen(buf.mtext, "w+");
		fprintf(fptr,"%s",shmPtr);
		fclose(fptr);
		sem_post(write1);
/*		sem_post(&mutex[(int)buf.mtext[1]-48]); //Exit critical section*/
		memset(buf.mtext, '\0', sizeof(buf.mtext));
		strcpy(buf.mtext, "File successfully modified\n");
		buf.mtype=buf.seq*1000;
		if(msgsnd(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 0) == -1)
		{ 
			perror("Error in msgsnd in line 44\n");
			exit(-3);
		}
	}
	else
	{
		//cleanup activity(not reqd now)
	}
	sem_close(write1);
	if (sem_unlink(temp1) == -1) {
            perror("Error unlinking semaphore in Primary server\n");
            exit(EXIT_FAILURE);
        }
	if(shmdt(shmPtr)==-1)
	{
		perror("Error in shmdt in line 101\n");
		exit(-6);
	}
	if(shmctl(shmid, IPC_RMID, 0)==-1)
	{
		perror("Error in shmctl in line 120\n");
		exit(-7);
	}
	return NULL;
}

int main()
{
/*	fflush(stdin);*/
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

	pthread_t thread[100];	
	int i=0;
	while(1)
	{
		struct my_msgbufs buf;
		if(msgrcv(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 1002, 0) == -1)
		{ 
			perror("Error in msgrcv in line 35\n");
			exit(-3);
		}
		if(buf.opt==5)
			break;
		pthread_create(&thread[i%100], NULL, func, (void*)(&buf));
		i++;
	}
	//cleanup
	for(int j=0;j<i;j++)
		pthread_join(thread[i], NULL);

	return 0;
}
