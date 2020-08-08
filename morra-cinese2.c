#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>


typedef struct
{
	long tipo;
	int mossa;
} msg;


#define P1 1
#define P2 2
#define G1 3
#define G2 4


#define C 0
#define F 1
#define S 2


#define GIOCA 0
#define FINE 1


//a è mossa di P1, b è mossa di P2
char controlla(int a, int b)
{
	if(a == C && b == S) return '1';
	if(a == F && b == C) return '1';
	if(a == S && b == F) return '1';
	
	if(b == C && a == S) return '2';
	if(b == F && a == C) return '2';
	if(b == S && a == F) return '2';
}



void giudice(int partite, int msg_fd, int fifo_fd)
{
	msg p1, p2;
	char buffer[1];
	printf("giudice pronto...\n");
	
	int temp = 0;
	while(temp < partite)
	{
		p1.tipo = G1;
		p1.mossa = GIOCA;
		p2.tipo = G2;
		p2.mossa = GIOCA;
		
		if(msgsnd(msg_fd, &p1, sizeof(p1.mossa), 0) == -1)
		{
			printf("errore in invio mossa gioca a P1\n");
			exit(1);
		}
		
		if(msgsnd(msg_fd, &p2, sizeof(p2.mossa), 0) == -1)
		{
			printf("errore in invio mossa gioca a P2\n");
			exit(1);
		}
		
		
		if(msgrcv(msg_fd, &p1, sizeof(p1.mossa), P1, 0) == -1)
		{
			printf("errore in ricezione mossa di P1\n");
			exit(1);
		}
		
		if(msgrcv(msg_fd, &p2, sizeof(p2.mossa), P2, 0) == -1)
		{
			printf("errore in ricezione mossa di P2\n");
			exit(1);
		}
		
		
		if(p1.mossa == p2.mossa)
		{
			printf("partita patta, da rigiocare\n");
			continue;
		}
		
		buffer[0] = controlla(p1.mossa, p2.mossa);
		if(buffer[0] == '1')
			printf("vince il giocatore 1\n");
		else
			printf("vince il giocatore 2\n");
		
		if(write(fifo_fd, buffer, 1) == -1)
		{
			printf("errore nella scrittura sulla coda fifo\n");
			exit(1);
		}
		
		temp++;
	}
	
	p1.tipo = G1;
	p1.mossa = FINE;
	p2.tipo = G2;
	p2.mossa = FINE;
	
	if(msgsnd(msg_fd, &p1, sizeof(p1.mossa), 0) == -1)
	{
		printf("non riesco a inviare messaggio di fine a P1\n");
		exit(1);
	}
	
	if(msgsnd(msg_fd, &p2, sizeof(p2.mossa), 0) == -1)
	{
		printf("non riesco a inviare messaggio di fine a P2\n");
		exit(1);
	}
	
	buffer[0] = 'F';
	if(write(fifo_fd, buffer, 1) == -1)
	{
		printf("non riesco ad inviare messaggio di fine a tabellone\n");
		exit(1);
	}
	
	exit(0);
}


void giocatore1(int msg_fd)
{
	msg p1;
	printf("P1 pronto a giocare...\n");
	srand(time(NULL));
	
	while(1)
	{
		if(msgrcv(msg_fd, &p1, sizeof(p1.mossa), G1, 0) == -1)
		{
			printf("P1 non riesce a leggere il comando del giudice\n");
			exit(1);
		}
		
		if(p1.mossa == FINE)
		{
			printf("P1 finisce la sua partita\n");
			exit(0);
		}
		
		else
		{
			p1.mossa = (rand() % rand()) % 3;
			p1.tipo = P1;
			
			if(p1.mossa == C)
				printf("P1 ha scelto: carta\n");
			else if(p1.mossa == F)
				printf("P1 ha scelto: forbice\n");
			else
				printf("P1 ha scelto: sasso\n");
			
			if(msgsnd(msg_fd, &p1, sizeof(p1.mossa), 0) == -1)
			{
				printf("P1 non riesce ad inviare la sua mossa al giudice...\n");
				exit(1);
			}
			
		}
	}
}

void giocatore2(int msg_fd)
{
	msg p2;
	printf("P2 pronto a giocare...\n");
	srand(time(NULL));
	
	
	while(1)
	{
		if(msgrcv(msg_fd, &p2, sizeof(p2.mossa), G2, 0) == -1)
		{
			printf("P2 non riesce a ricevere la mossa del giudice\n");
			exit(1);
		}
		
		if(p2.mossa == FINE)
		{
			printf("P2 finisce la sua partita\n");
			exit(0);
		}
		
		else
		{
			p2.mossa = rand() % 3;
			p2.tipo = P2;
			
			if(p2.mossa == C)
				printf("P2 ha scelto: carta\n");
			else if(p2.mossa == F)
				printf("P2 ha scelto: forbice\n");
			else
				printf("P2 ha scelto: sasso\n");
			
			if(msgsnd(msg_fd, &p2, sizeof(p2.mossa), 0) == -1)
			{
				printf("P2 non è riuscito a inviare la sua mossa...\n");
				exit(1);
			}
			
		}
	}
}


void tabellone(int fifo_fd)
{
	char buffer[1];
	int p1 = 0, p2 = 0;
	printf("Tabellone pronto...\n");
	
	while(1)
	{
		if(read(fifo_fd, buffer, 1) == -1)
		{
			printf("errore nella lettura da coda fifo...\n");
			exit(1);
		}
		
		if(buffer[0] == '1')
			p1++;
		
		else if(buffer[0] == '2')
			p2++;
		
		else
		{
			if(p1 == p2)
				printf("il torneo è terminato con un pareggio\n");
			
			else
			{
				printf("il vincitore assoluto è\t");
				if(p1 > p2)
					printf("P1\n");
				
				else
					printf("P2\n");	
			}
			
			exit(0);
		}
	}
}


int main(int argc, char *argv[])
{	
	int msg_fd, pid, fifo_fd;
	char *path_name = "./fifo";	
	
	if(argc != 2)
	{
		fprintf(stderr, "uso: %s <numero_partite>\n", argv[0]);
		exit(1);
	}
	
	
	if((msg_fd = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1)
	{
		printf("errore nella creazione della coda di messaggi...\n");
		exit(1);
	}
	
	if(mkfifo(path_name, 0600) == -1)
	{
		printf("errore nella creazione della coda fifo\n");
		msgctl(msg_fd, IPC_RMID, NULL);
		exit(1);
	}
	
	if((fifo_fd = open(path_name, O_RDWR)) == -1)
	{
		printf("errore durante l'apertura della coda fifo\n");
		unlink(path_name);
		exit(1);
	}
	
	pid = fork();
	
	if(pid == 0)
		giudice(atoi(argv[1]), msg_fd, fifo_fd);
	
	//padre
	else
	{
		pid = fork();
		
		if(pid == 0)
			giocatore1(msg_fd);
		
		//padre
		else
		{
			pid = fork();
			
			if(pid == 0)
				giocatore2(msg_fd);
			
			//padre
			else
			{
				pid = fork();
				
				if(pid == 0)
					tabellone(fifo_fd);
				
				//padre
				else
				{
					wait(NULL);
					wait(NULL);
					wait(NULL);
					wait(NULL);
				}
			}
		}
	}
	
	msgctl(msg_fd, IPC_RMID, NULL);
	close(fifo_fd);
	unlink(path_name);
	
	printf("fine programma\n");
	
	return 0;
}




