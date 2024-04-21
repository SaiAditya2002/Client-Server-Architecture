#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <fcntl.h>

#define SEM_NAME1 "/my_semaphore1"
#define SEM_NAME2 "/my_semaphore2"
#define MAX_VERTICES 50
#define NUM_THREADS 100
#define BUF_SIZE 1000
#define PERMS 0644

typedef struct
{
	int vertex;
	int vertices;
	int adjMatrix[MAX_VERTICES][MAX_VERTICES];
	bool visited[MAX_VERTICES];
	int currentLevel[MAX_VERTICES];
	int sum;
	char ans[MAX_VERTICES];
} Graph;
struct my_msgbufs
{
	long mtype;
	long seq;
	char mtext[200];
	int opt;
};
sem_t *read1[21];
sem_t *write1[21];
pthread_mutex_t mutex, mutex2, mutex3;
int numReader = 0;

//
void DFS(int vertex, Graph *g);
void *threadDFS(void *arg)
{
	Graph *g = (Graph *)arg;
	int startVertex = g->vertex;
	pthread_mutex_unlock(&mutex2);
	DFS(startVertex, g);
	pthread_exit(NULL);
}
void DFS(int vertex, Graph *g)
{
	pthread_mutex_lock(&mutex);
	if (g->visited[vertex])
	{
		pthread_mutex_unlock(&mutex);
		return;
	}
	g->visited[vertex] = 1;
	pthread_mutex_unlock(&mutex);
	pthread_t threads[NUM_THREADS];
	int flag = 0;
	for (int i = 0; i < g->vertices; i++)
	{
		if (g->adjMatrix[vertex][i] == 1 && !g->visited[i])
		{
			flag = 1;
			/*      	  thread_args[i] = i % g.vertices;*/
			pthread_mutex_lock(&mutex2);
			g->vertex = i % g->vertices;
			pthread_create(&threads[i], NULL, threadDFS, (void *)g);
		}
	}
	for (int i = 0; i < g->vertices; i++)
	{
		if (g->adjMatrix[vertex][i] == 1 && !g->visited[i])
		{
			pthread_join(threads[i], NULL);
		}
	}

	pthread_mutex_lock(&mutex);
	if (flag == 0)
	{
		char t1[5];
		sprintf(t1, "%d ", vertex);
		strcat(g->ans, t1);
	}
	pthread_mutex_unlock(&mutex);
}
//

void *exploreAdjacent(void *arg)
{
	/*    pthread_mutex_lock(&mutex);*/
	Graph *g = (Graph *)arg;
	int vertex = g->vertex;
	pthread_mutex_unlock(&mutex3);
	if (!g->visited[vertex])
	{
		g->visited[vertex] = true;
		g->sum++;
		/*        printf("Visited %d\n", vertex);*/
		for (int i = 0; i < g->vertices; i++)
		{
			if (g->adjMatrix[vertex][i] == 1 && !g->visited[i])
			{
				g->currentLevel[i] = 1;
				printf("\n%d ", i);
				fflush(stdout);
				char t2[5];
				sprintf(t2, "%d ", i);
				strcat(g->ans, t2);
			}
		}
	}
	/*    pthread_mutex_unlock(&mutex);*/
	pthread_exit(NULL);
}

void *func(void *arg)
{
	/*	static int val=1;*/
	int shmid;
	char *shmPtr;
	int msqid;
	key_t key, key2;
	struct my_msgbufs buf = *(struct my_msgbufs *)arg;
	char temp3[20] = "/";
	strncat(temp3, buf.mtext, 3);
	strcat(temp3, "r");
	sem_t *read1 = sem_open(temp3, O_CREAT, PERMS, 1);
	char temp4[20] = "/";
	strncat(temp3, buf.mtext, 3);
	strcat(temp4, "w");
	sem_t *write1 = sem_open(temp4, O_CREAT, PERMS, 1);
	if ((key = ftok("load_balancer.c", 'A')) == -1)
	{
		perror("Error in ftok in line 20\n");
		exit(-1);
	}
	if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1)
	{
		perror("Error in msgget in line 25\n");
		exit(-2);
	}
	if ((key2 = ftok("load_balancer.c", buf.seq)) == -1)
	{
		perror("Error in ftok in line 20\n");
		exit(-1);
	}
	shmid = shmget(key2, BUF_SIZE, PERMS | IPC_CREAT);
	if (shmid == -1)
	{
		perror("Error in shmget in line 54\n");
		exit(-1);
	}
	shmPtr = shmat(shmid, NULL, 0);
	if (shmPtr == (void *)-1)
	{
		perror("Error in shmPtr in line 60\n");
		exit(-2);
	}
	int x;
	if (buf.mtext[2] == '.')
		x = (int)buf.mtext[1] - 48;
	else if (buf.mtext[3] == '.')
	{
		x = ((int)buf.mtext[1] - 48) * 10 + (int)buf.mtext[1] - 48;
	}
	if (buf.opt == 4)
	{
		char data[1000];
		memset(data, '\0', sizeof(data));
		sem_wait(read1);
		numReader++;
		if (numReader == 1)
			sem_wait(write1);
		sem_post(read1);
		/*		sem_wait(&mutex[(int)buf.mtext[1]-48]); // enter critical section*/
		// Read File only do operations later
		FILE *fptr;
		fptr = fopen(buf.mtext, "r");
		if (fptr == NULL)
		{
			printf("no such file.");
			return 0;
		}
		Graph g;
		fscanf(fptr, "%d", &g.vertices);
		for (int a = 0; a < g.vertices; a++)
		{
			for (int b = 0; b < g.vertices; b++)
			{
				fscanf(fptr, "%d", &g.adjMatrix[a][b]);
			}
		}
		fclose(fptr);
		sem_wait(read1);
		numReader--;
		if (numReader == 0)
			sem_post(write1);
		sem_post(read1);
		/*		sem_post(&mutex[(int)buf.mtext[1]-48]); //Exit critical section*/

		// Call thread and pass data string to it as parameter
		pthread_t threads[NUM_THREADS];
		int thread_args[MAX_VERTICES];
		/*	     printf("\n%d ", shmPtr[0]);*/
		/*	     fflush(stdout);*/
		int startVertex = (int)shmPtr[0] - 48;
		g.sum = 0;
		memset(g.visited, 0, sizeof(g.visited));
		memset(g.currentLevel, 0, sizeof(g.currentLevel));
		g.currentLevel[startVertex] = 1;
		int i = startVertex;
		/*	     printf("\n%d ", g.vertices);*/
		/*	     fflush(stdout);*/
		char t1[5];
		sprintf(t1, "%d ", i);
		strcat(g.ans, t1);
		while (true)
		{
			int copymatrix[g.vertices];

			if (g.currentLevel[i])
			{
				memcpy(copymatrix, g.currentLevel, g.vertices * sizeof(int));
				for (int j = 0; j < g.vertices; j++)
				{
					if (copymatrix[j] == 1)
					{
						pthread_mutex_lock(&mutex3);
						g.vertex = j;
						pthread_create(&threads[j % NUM_THREADS], NULL, exploreAdjacent, (void *)&g);
						/*						pthread_join(threads[j], NULL);*/
						/*				 		g.currentLevel[i] = 0;*/
					}
				}
				for (int j = 0; j < g.vertices; j++)
				{
					if (copymatrix[j] == 1)
					{
						pthread_join(threads[j], NULL);
						g.currentLevel[j] = 0;
					}
				}
			}
			i++;
			i = i % g.vertices;
			if (g.sum == g.vertices)
				break;
		}
		memset(buf.mtext, '\0', sizeof(buf.mtext));
		strcpy(buf.mtext, g.ans);
		buf.mtype = buf.seq * 1000;
		if (msgsnd(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 0) == -1)
		{
			perror("Error in msgsnd in line 44\n");
			exit(-3);
		}
	}
	else if (buf.opt == 3)
	{
		char data[1000];
		memset(data, '\0', sizeof(data));
		sem_wait(read1);
		numReader++;
		if (numReader == 1)
			sem_wait(write1);
		sem_post(read1);
		/*		sem_wait(&mutex[(int)buf.mtext[1]-48]); // enter critical section*/
		// Read File only do operations later
		FILE *fptr;
		fptr = fopen(buf.mtext, "r");
		if (fptr == NULL)
		{
			printf("no such file.\n");
			return 0;
		}
		Graph g;
		fscanf(fptr, "%d", &g.vertices);
		for (int a = 0; a < g.vertices; a++)
		{
			for (int b = 0; b < g.vertices; b++)
			{
				fscanf(fptr, "%d", &g.adjMatrix[a][b]);
			}
		}
		fclose(fptr);
		sem_wait(read1);
		numReader--;
		if (numReader == 0)
			sem_post(write1);
		sem_post(read1);
		/*		sem_post(&mutex[(int)buf.mtext[1]-48]); //Exit critical section*/

		// Call thread and pass data string to it as parameter
		pthread_t threads[NUM_THREADS];
		int thread_args[MAX_VERTICES];
		int startVertex = (int)shmPtr[0] - 48;
		g.sum = 0;
		memset(g.visited, 0, sizeof(g.visited));
		pthread_mutex_lock(&mutex2);
		g.vertex = startVertex;
		pthread_create(&threads[startVertex], NULL, threadDFS, (void *)&g);
		pthread_join(threads[startVertex], NULL);

		memset(buf.mtext, '\0', sizeof(buf.mtext));
		strcpy(buf.mtext, g.ans);
		buf.mtype = buf.seq * 1000;
		if (msgsnd(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 0) == -1)
		{
			perror("Error in msgsnd in line 44\n");
			exit(-3);
		}
	}
	else
	{
		// cleanup activity(not reqd now)
	}
	sem_close(read1);
	sem_close(write1);

	if (shmdt(shmPtr) == -1)
	{
		perror("Error in shmdt in line 101\n");
		exit(-6);
	}
	if (shmctl(shmid, IPC_RMID, 0) == -1)
	{
		perror("Error in shmctl in line 106\n");
		exit(-7);
	}
	return NULL;
}

int main()
{
	int msqid;
	key_t key;

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutex2, NULL);
	pthread_mutex_init(&mutex3, NULL);
	/*	fflush(stdin);*/
	if ((key = ftok("load_balancer.c", 'A')) == -1)
	{
		perror("Error in ftok in line 20\n");
		exit(-1);
	}

	if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1)
	{
		perror("Error in msgget in line 25\n");
		exit(-2);
	}

	pthread_t thread[100];
	int i = 0;
	while (1)
	{
		struct my_msgbufs buf;
		if (msgrcv(msqid, &buf, sizeof(struct my_msgbufs) - sizeof(long), 1004, 0) == -1)
		{
			perror("Error in msgrcv in line 35\n");
			exit(-3);
		}
		if (buf.opt == 5)
			break;
		pthread_create(&thread[i % 100], NULL, func, (void *)(&buf));
		i++;
	}
	// cleanup
	for (int j = 0; j < i; j++)
		pthread_join(thread[j], NULL);

	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutex2);
	pthread_mutex_destroy(&mutex3);
	return 0;
}
