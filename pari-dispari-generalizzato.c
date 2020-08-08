#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <math.h>



#define GIOCA 10
#define FINE 11


struct msg
{
    long type;
    int cmd;
};




void giocatore(int idg, int msgds, int num_gioc)
{
    struct msg mymsg1, mymsg2;
    
    srand(time(NULL));
    
    while(1)
    {
        if(msgrcv(msgds, &mymsg1, sizeof(mymsg1.cmd), idg, 0) == -1)
        {
        	perror("1) msgrcv");
        	exit(1);
        }
        
        if(mymsg1.cmd == FINE)
        {
            printf("Partita terminata, fine!\n");
            exit(1);
        }
        
        mymsg2.type = idg + num_gioc;
        mymsg2.cmd = (rand() * (idg+1)) % 10;
        if(msgsnd(msgds, &mymsg2, sizeof(mymsg2.cmd), 0) == -1)
        {
        	perror("2) msgsnd");
        	exit(1);
        }
    }
}



//argv[1] è n, il numero di giocatori, argv[2] è m, il numero di partite
int main(int argc, char** argv)
{
    int msgds;    
    
    if(argc != 3)
    {
        printf("Numero di parametri errato!\n");
        exit(1);
    }
    
    if(atoi(argv[1]) < 1 || atoi(argv[2]) < 2 || atoi(argv[2]) > 6)
    {
        printf("Il numero di giocatori e/o di partite non è quello che ci si aspetta! Esci...\n");
        exit(1);
    }
    
    if((msgds = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1)
    {
        perror("msgget");
        exit(1);
    }
    
    for(int i=1; i<=atoi(argv[1]); i++)
        if(fork() == 0)
            giocatore(i, msgds, atoi(argv[1]));
    
    
    
    //qui parte il codice di J, che è anche il processo padre
    //x è il numero di partite giocate e non patte
    //codice di J
    int x = 0;
    int punteggio = 0;
    int vector[atoi(argv[1])];
    int punteggi[atoi(argv[1])];
    
    for(int i=0; i<atoi(argv[1]); i++)
        	punteggi[i] = 0;
    
    while(x < atoi(argv[2]))
    {
        for(int i=1; i<=atoi(argv[1]); i++)
        {
            struct msg mymsg;
            mymsg.type = i;
            mymsg.cmd = GIOCA;
            if(msgsnd(msgds, &mymsg, sizeof(mymsg.cmd), 0) == -1)
            {
                perror("1) msgsnd ");
                exit(1);
            }  
        }
        
        
        for(int i=1; i<=atoi(argv[1]); i++)
        {
            struct msg mymsg;
            if(msgrcv(msgds, &mymsg, sizeof(mymsg.cmd), i+atoi(argv[1]), 0) == -1)
            {
            	perror("2) msgrcv");
            }
            
            vector[i-1] = mymsg.cmd;
        }
        
        
        //cerca i duplicati
        for(int i=0; i<atoi(argv[1]); i++)
            for(int j=i+1; j<atoi(argv[1]); j++)
                if(vector[i] == vector[j])
                {
                    printf("Ci sono dei duplicati, partita patta\n");
                    continue;
                }        
        
        
        for(int i=0; i<atoi(argv[1]); i++)
            punteggio += vector[i];
        
        
        punteggio = abs(punteggio % atoi(argv[1]));
        
        punteggi[punteggio]++;
        x++;
        
        printf("Partita vinta dal giocatore %d\n", punteggio);
    }
    
    
    for(int i=1; i<=atoi(argv[1]); i++)
    {
        struct msg mymsg;
        mymsg.type = i;
        mymsg.cmd = FINE;
        if(msgsnd(msgds, &mymsg, sizeof(mymsg.cmd), 0) == -1)
        {
            printf("errore nell'invio del messaggio da parte di J\n");
            exit(1);
        }  
    }
    
    
    for(int i=0; i<atoi(argv[1]); i++)
        printf("Le partite vinte dal giocatore %d sono %d\n", i, punteggi[i]);
    
    
    int max = -1;
    int max_i = 0;
    for(int i=0; i<atoi(argv[1]); i++)
        if(punteggi[i] > max)
        {
            max = punteggi[i];
            max_i = i;
        }
    
    
    for(int i=0; i<atoi(argv[1]); i++)
    	if(punteggi[i] == max && i != max_i)
    		printf("Non ci sono vincitori assoluti\n");
    
    printf("Il vincitore è il giocatore %d\n", max_i);
    
    msgctl(msgds, IPC_RMID, NULL);
    
    return 0;
}








