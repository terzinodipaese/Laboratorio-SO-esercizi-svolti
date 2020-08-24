#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>




#define FILECONS 0
#define DIRCONS 1
#define MUTEX 2


#define DIM 1024


int WAIT(int sem_des, int num_semaforo)
{
    struct sembuf operazioni[1] = {{num_semaforo, -1, 0}};
    return semop(sem_des, operazioni, 1);
}

int SIGNAL(int sem_des, int num_semaforo) 
{
    struct sembuf operazioni[1] = {{num_semaforo, +1, 0}};
    return semop(sem_des, operazioni, 1);
}


int is_reg_file(char* path)
{
	struct stat file;
	
	if(stat(path, &file) == -1)
	{
		perror("stat");
		exit(1);
	}
	
	if(S_ISREG(file.st_mode))
		return file.st_size;
	else
		return -1;
}



int is_dir(char* path)
{
	struct stat dir;
	
	if(stat(path, &dir) == -1)
	{
		perror("stat");
		exit(1);
	}
	
	if(S_ISDIR(dir.st_mode))
		return 0;
	else
		return -1;
}



void reader(char* path, int semid, int shmid)
{
	char* shmaddr;
	char buffer[DIM];		//stringa d'appoggio
	int* fsize;
	int size;

	if((shmaddr = (char*) shmat(shmid, NULL, 0)) == (char*) -1)
	{
		perror("shmat");
		exit(1);
	}
	
	DIR* dir;
	struct dirent* entry;
	
	if((dir = opendir(path)) == NULL)
	{
		perror("opendir");
		exit(1);
	}
	
	while((entry = readdir(dir)))
	{
		if(entry->d_name[0] == '.')
			continue;
		
		//preparo la stringa
		strcpy(buffer, path);
		if(buffer[strlen(buffer) - 1] != '/')
			strcat(buffer, "/");
		strcat(buffer, entry->d_name);
		
		if(is_dir(buffer) != -1)
		{
			WAIT(semid, MUTEX);
			
			memset(shmaddr, 0, DIM);
			strcpy(shmaddr, buffer);
			
			SIGNAL(semid, DIRCONS);
		}
		
		else if((size = is_reg_file(buffer)) != -1)
		{
			WAIT(semid, MUTEX);
			
			memset(shmaddr, 0, DIM);
			strcpy(shmaddr, buffer);
			fsize = (int*) shmaddr + strlen(buffer + 1);
			*fsize = size;
			
			SIGNAL(semid, FILECONS);
		}
	}
	
	exit(0);
}



void file_cons(int semid, int shmid)
{
	char* shmaddr;
	int* size;
	
	if((shmaddr = (char*) shmat(shmid, NULL, 0)) == (char*) -1)
	{
		perror("shmat");
		exit(1);
	}
	
	while(1)
	{
		WAIT(semid, FILECONS);
		
		if(strcmp(shmaddr, "quit") == 0)
			break;
		
		size = (int*) shmaddr + strlen(shmaddr + 1);
		printf("%s [file di %d bytes]\n", shmaddr, *size);
		
		SIGNAL(semid, MUTEX);
	}
	
	exit(0);
}


void dir_cons(int semid, int shmid)
{
	char* shmaddr;

	if((shmaddr = (char*) shmat(shmid, NULL, 0)) == (char*) -1)
	{
		perror("shmat");
		exit(1);
	}
	
	while(1)
	{
		WAIT(semid, DIRCONS);
		
		if(strcmp(shmaddr, "quit") == 0)
			break;
		
		printf("%s [directory]\n", shmaddr);
		
		SIGNAL(semid, MUTEX);
	}
	
	exit(0);
}




int main(int argc, char** argv)
{
	int semid, shmid;
	char* shmaddr;
	
	if(argc < 2)
	{
		printf("Nessuna directory specificata... Esci!\n");
		exit(0);
	}
	
	if((shmid = shmget(IPC_PRIVATE, DIM, IPC_CREAT | 0600)) == -1)
	{
		perror("shmget");
		exit(1);
	}
	
	if((semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0600)) == -1)
	{
		perror("semget");
		exit(1);
	}
	
	if((shmaddr = (char*) shmat(shmid, NULL, 0)) == (char*) -1)
	{
		perror("shmat");
		exit(1);
	}
	
	
	semctl(semid, MUTEX, SETVAL, 1);
	semctl(semid, FILECONS, SETVAL, 0);
	semctl(semid, DIRCONS, SETVAL, 0);
	
	for(int i = 1; i < argc; i++)
	{
		if(fork() == 0)
			reader(argv[i], semid, shmid);
	}
	
	if(fork() == 0)
		file_cons(semid, shmid);
	
	if(fork() == 0)
		dir_cons(semid, shmid);
	
	for(int i = 1; i < argc; i++)
		wait(NULL);
	
	memset(shmaddr, 0, DIM);
	strcpy(shmaddr, "quit");
	SIGNAL(semid, FILECONS);
	SIGNAL(semid, DIRCONS);
	
	wait(NULL);
	wait(NULL);
	
	shmctl(shmid, IPC_RMID, NULL);
	semctl(semid, 0, IPC_RMID, 0);
	
	return 0;
}





















