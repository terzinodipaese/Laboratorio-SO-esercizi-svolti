#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>



#define C '0'
#define F '1'
#define S '2'

#define P1 0
#define P2 1

#define GIOCA '0'
#define FINE '1'



int up(int semid, int num_semaforo)
{
    struct sembuf operazioni[1] = {{num_semaforo, +1, 0}};
    return semop(semid, operazioni, 1);
}




int down(int semid, int num_semaforo)
{
    struct sembuf operazioni[1] = {{num_semaforo, -1, 0}};
    return semop(semid, operazioni, 1);
}



void giocatore(int shmid, int giocatore, int semvect)
{
	
	//attenzione a questa srand
	srand(time(NULL) * (giocatore+1));
	char continua, mossa;
	char *shmemory;
	
	if((shmemory = (char*) shmat(shmid, NULL, 0)) == (char*) -1)
	{
		perror("shmat:");
		exit(1);
	}
	
	
	while(1)
	{
		down(semvect, giocatore);
		continua = shmemory[3];
	
		if(continua == FINE)
			exit(0);
		
		int temp = rand() % 3;
		switch(temp)
		{
			case 0:
				mossa = C;
				printf("Giocatore %d sceglie carta\n", giocatore);
				break;
				
			case 1:
				mossa = F;
				printf("Giocatore %d sceglie forbice\n", giocatore);
				break;
			
			default:
				mossa = S;
				printf("Giocatore %d sceglie sasso\n", giocatore);
				break;
		}
		
		shmemory[giocatore] = mossa;
		up(semvect, 2);
	}
}



void giudice(int shmid, int semvect)
{
	char* shmemory;	
	char continua, mossap1, mossap2;
	
	if((shmemory = shmat(shmid, NULL, 0)) == (char*) -1)
	{
		perror("shmat:");
		exit(0);
	}
	
	
	while(1)
	{
		down(semvect, 2);
		down(semvect, 2);
		
		continua = shmemory[3];
		if(continua == FINE)
			exit(1);
		
		mossap1 = shmemory[0];
		mossap2 = shmemory[1];
		
		//caso patta
		if(mossap1 == mossap2)
		{
			up(semvect, 0);
			up(semvect, 1);
			continue;
		}
		
		
		//caso non patta
		else
		{
			if((mossap1 == C && mossap2 == S) || (mossap1 == S && mossap2 == F) || (mossap1 == F && mossap2 == C))
				shmemory[2] = '1';
			
			else
				shmemory[2] = '2';
			
			up(semvect, 3);
		}		
	}
}



void tabellone(int shmid, int semvect, int npartite)
{
	int p1 = 0, p2 = 0, i = 0;
	char* shmemory;
	char vincitore;
	
	if((shmemory = (char*) shmat(shmid, NULL, 0)) == (char*) -1)
	{
		perror("shmat:");
		exit(0);
	}
	
	while(i < npartite)
	{
		shmemory[3] = GIOCA;
		
		up(semvect, 0);
		up(semvect, 1);
		down(semvect, 3);
		
		vincitore = shmemory[2];
		
		if(vincitore == '1')
		{
			printf("P1 vince!\n");
			p1++;
		}
		
		else
		{
			printf("P2 vince!\n");
			p2++;
		}
		
		i++;
		
		if(i >= npartite)
		{
			shmemory[3] = FINE;
			
			if(p1 > p2)
				printf("P1 ha vinto il torneo\n");
			else if(p2 > p1)
				printf("P2 ha vinto il torneo\n");
			else
				printf("Torneo pareggiato\n");
			
			up(semvect, 0);
			up(semvect, 1);
			up(semvect, 2);
			up(semvect, 2);
			
			exit(0);
		}
		
		up(semvect, 0);
		up(semvect, 1);
	}
	
}


int main(int argc, char *argv[])
{
	int shmid, pid, semid;
	
	if(argc != 2)
	{
		printf("Usage: %s <numero_partite>\n", argv[0]);
		exit(0);
	}
	
	if((shmid = shmget(IPC_PRIVATE, 4, IPC_CREAT | 0600)) == -1)
	{
		perror("shmget:");
		exit(1);
	}
	
	if((semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0600)) == -1)
	{
		perror("smget:");
		exit(1);
	}
	
	semctl(semid, 0, SETVAL, 0);
	semctl(semid, 1, SETVAL, 0);
	semctl(semid, 2, SETVAL, 0);
	semctl(semid, 3, SETVAL, 0);
	
	pid = fork();
	if(pid == 0)
		tabellone(shmid, semid, atoi(argv[1]));
	else
	{
		pid = fork();
		if(pid == 0)
			giocatore(shmid, 0, semid);
		else
		{
			pid = fork();
			if(pid == 0)
				giocatore(shmid, 1, semid);
			else
			{
				pid = fork();
				if(pid == 0)
					giudice(shmid, semid);
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
	
	shmctl(shmid, IPC_RMID, NULL);
	semctl(semid, 0, IPC_RMID, 0);
	
	return 0;
}
