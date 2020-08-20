#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>



#define ROW_SIZE 2048


struct msg1
{
	long type;
	char buffer[ROW_SIZE];
};


struct msg2
{
	long type;
	int occ[26];
};



void reader(char* filename, int msgid, int idreader)
{
	FILE* fp = fopen(filename, "r");
	struct msg1 mymsg;
	
	while(fgets(mymsg.buffer, ROW_SIZE, fp) != NULL)
	{
		mymsg.type = idreader;
		if(mymsg.buffer[strlen(mymsg.buffer) - 1] != '\0')
			mymsg.buffer[strlen(mymsg.buffer) - 1] = '\0';
		
		if(msgsnd(msgid, &mymsg, sizeof(mymsg.buffer), 0) == -1)
		{
			perror("1) msgsnd");
			exit(1);
		}
	}
	
	
	//messaggio di fine del reader
	//qua puliamo il buffer
	for(int i = 0; i < ROW_SIZE; i++)
	    mymsg.buffer[i] = 0;
	
	
	mymsg.buffer[0] = 7;	//carattere BELL
	mymsg.type = idreader;
	if(msgsnd(msgid, &mymsg, sizeof(mymsg.buffer), 0) == -1)
	{
		perror("2) msgsnd");
		exit(1);
	}
	
	fclose(fp);
	exit(0);
}


void counter(int msgid1, int msgid2, int n_proc)
{   
    //numero di processi reader terminati
	int proc_term = 0;
	
	//due messaggi, uno per la 1a coda e uno per la 2a
	struct msg1 mymsg1;
	struct msg2 mymsg2;
	
	
	while(proc_term < n_proc)
	{
		if(msgrcv(msgid1, &mymsg1, sizeof(mymsg1.buffer), 0, 0) == -1)
		{
			perror("1) msgrcv");
			exit(1);
		}
		
		
		//vuol dire che uno dei reader ha terminato
		if(mymsg1.buffer[0] == 7)
		{
		    printf("reader n.%ld terminato\n", mymsg1.type);
			proc_term++;
			continue;
		}
		
		
		//qua analizzo la stringa
		char c;
		int i = 0;
		
		//puliamo il vettore
		for(int i = 0; i < 26; i++)
			mymsg2.occ[i] = 0;
		
		while(1)
		{
			c = mymsg1.buffer[i];
			if(c == '\0')
				break;
			
			//lettere maiuscole
			if(c >= 65 && c <= 90)
				mymsg2.occ[(int) c - 65]++;
			
			//lettere minuscole
			if(c >= 97 && c <= 122)
				mymsg2.occ[(int) c - 97]++;
			
			i++;
		}
		
		
		/*
		//per debugging
		printf("stampa occorrenze messaggio\n");
		for(int i = 0; i < 26; i++)
		    if(mymsg2.occ[i] != 0)
		        printf("%c = %d\n", i + 97, mymsg2.occ[i]);
		*/
		
		
		mymsg2.type = 1;
		if(msgsnd(msgid2, &mymsg2, sizeof(mymsg2.occ), 0) == -1)
		{
			perror("3) msgsnd");
			exit(1);
		}		
	}
	
	
	//messaggio per segnalare la fine al processo padre
	mymsg2.type = 1;
	mymsg2.occ[0] = -1;
	if(msgsnd(msgid2, &mymsg2, sizeof(mymsg2.occ), 0) == -1)
	{
		perror("4) msgsnd");
		exit(1);
	}
}



int main(int argc, char** argv)
{
	int msgid1, msgid2, pid;
	
	if(argc < 2)
	{
		printf("Non hai specificato nessun file... Esci!\n");
		exit(0);
	}	
	
	if((msgid1 = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1)
	{
		perror("msgget");
		exit(1);
	}
	
	if((msgid2 = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1)
	{
		perror("msgget");
		exit(1);
	}
	
	for(int i = 1; i <= argc-1; i++)
	{
		pid = fork();
		if(pid == 0)
			reader(argv[i], msgid1, i);
	}
	
	pid = fork();
	if(pid == 0)
		counter(msgid1, msgid2, argc - 1);
	
	
	
	int totali[26];
	for(int i = 0; i < 26; i++)
		totali[i] = 0;
	
	
	
	struct msg2 mymsg;
	while(1)
	{
		if(msgrcv(msgid2, &mymsg, sizeof(mymsg.occ), 1, 0) == -1)
		{
			perror("2) msgrcv");
			exit(1);
		}
		
		
		if(mymsg.occ[0] == -1)
			break;
		
		for(int i = 0; i < 26; i++)
			totali[i] += mymsg.occ[i];
	}
	
	
	printf("Totali:\n\n");
	for(int i = 0; i < 26; i++)
		printf("%c = %d\n", i + 97, totali[i]);
	
	
	
	exit(1);
}













