#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h>
#include "constantes.h"
#include "types.h"
#include "sema.h"

int main( int argc, char **argv)
{
	if(argc < 2) { printf("Usage %s numIPC\n", argv[0]); exit(EXIT_FAILURE); }
	MEMP *memoireP; 
	int mutex_data, mutex_tpa, sem_global, continuer = 0, i=0;
	char c;
	MSG message;
	key_t sem_key_data = MUTEX_DATA;
	key_t sem_key_glob = MUTEX_GLOB;
	key_t sem_key_tpa = MUTEX_TPA;
	int shmid; 
	int shm_key = atoi(argv[1]);

	shmid = shmget(shm_key, sizeof(MEMP), 0766 | IPC_CREAT); 
	if (shmid == -1) { perror("shmget"); exit(EXIT_FAILURE); }

	if((memoireP = (MEMP *) shmat(shmid, 0 , 0766)) ==(void *) -1)	{ perror("shmat"); exit(EXIT_FAILURE); }
	if((mutex_data = open_sem( sem_key_data)) == -1)	{ perror("open_sem"); exit(EXIT_FAILURE); }
	if((mutex_tpa = open_sem( sem_key_tpa)) == -1)		{ perror("open_sem"); exit(EXIT_FAILURE); }
	if((sem_global = open_sem( sem_key_glob)) == -1)	{ perror("open_sem"); exit(EXIT_FAILURE); }

	// sem_global est un sémaphore à MAX_PROD entrants.
	P(sem_global);
	P(mutex_tpa);
		for(i = 0; i < MAX_PROD && memoireP->tpa[i] != -1 ; i++);
	V(mutex_tpa);
	
	if(memoireP->tpa[i] != -1) { exit(EXIT_FAILURE); }
	memoireP->tpa[i] = 0;

	// On quitte le producteur en tappant 0 (zéro)
	while((c = getc(stdin)) != '0')
	{
		P(mutex_data);
			if(((memoireP->queue -1) % MAX_BUF) != (memoireP->tete % MAX_BUF) )
			{
				memoireP->f[memoireP->tete].c = c;
				memoireP->f[memoireP->tete].idp = i;
				memoireP->tete = memoireP->tete +1;
			}
			fprintf(stdout, "Voici le caractère écrit : %c\n", memoireP->f[memoireP->tete].c);
		V(mutex_data);
	}
	// On a fini, on remet la valeur de l'indice du processus à -1 dans le tableau des producteurs
	P(mutex_tpa);
		memoireP->tpa[i] = -1;	
	V(mutex_tpa);

	// On a fini, on libère l'accès à un autre producteur
	V(sem_global);
	return (EXIT_SUCCESS); 

}