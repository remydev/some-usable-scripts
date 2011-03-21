#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <curses.h>
#include <ctype.h>
#include "consommateur.h"
#include "types.h"
#include "sema.h"
#include "constantes.h"

WINDOW *creation_fenetre(int n,int d,char *t);

int main( int argc, char **argv)
{
	if(argc < 2) { printf("Usage : %s nSHM \n", argv[0]); exit(EXIT_FAILURE); }

	int shmid, shm_key, i = 0;
	int mutex_data, mutex_tpa, mutex_glob;
	key_t sem_key_data = MUTEX_DATA;
	key_t sem_key_tpa = MUTEX_TPA;
	key_t sem_key_glob = MUTEX_GLOB;

	shm_key = (key_t) atoi(argv[1]);
	MEMP * memoireP;

    const char CTRL_D = 4 ;

	int NB_FENETRES = 4;
    WINDOW *f_haut, *f_bas, *f_milieu1, *f_milieu2 ;
    WINDOW *w ;
    char c ;

    initscr() ;			/* initialisation (obligatoire) de curses */
    noecho() ;			/* suppression de l'echo des caracteres tapes*/
    cbreak() ;			/* lecture non bufferisee */

	if((shmid = shmget(shm_key, sizeof(MEMP), IPC_CREAT|IPC_EXCL|0766)) == -1) 
	{ perror("shmget"); exit(EXIT_FAILURE);}
	
	if((memoireP = (MEMP *) shmat(shmid, 0 , 0766)) == (void *) -1)
	{ perror("shmat"); exit(EXIT_FAILURE); }
		
	if((mutex_data = creat_sem( sem_key_data, 1)) == -1)
	{ perror("creat_sem"); exit(EXIT_FAILURE); }

	if((mutex_tpa = creat_sem( sem_key_tpa, 1)) == -1)
	{ perror("creat_sem"); exit(EXIT_FAILURE); }

	if((mutex_glob = creat_sem( sem_key_glob, 1)) == -1)
	{ perror("creat_sem"); exit(EXIT_FAILURE); }

	P(mutex_data);
	
		memoireP->tete = 0;
		memoireP->queue = 0;

	V(mutex_data);
		
	P(mutex_tpa);
		for( i = 0; i < MAX_PROD ; i++)
			memoireP->tpa[i] = -1;
	V(mutex_tpa);


	// Création des fenêtres MARCHE
    f_haut = creation_fenetre(LINES/NB_FENETRES,0,"F_HAUT") ;
    f_milieu1 = creation_fenetre(LINES/NB_FENETRES,LINES - 3 * (LINES/NB_FENETRES),"F_MILIEU1") ;
    f_milieu2 = creation_fenetre(LINES/NB_FENETRES,LINES - 2 * (LINES/NB_FENETRES),"F_MILIEU2") ;
    f_bas = creation_fenetre(LINES/NB_FENETRES, LINES - (LINES/NB_FENETRES),"F_BAS") ;

	sleep(5);
	// aDesProd => est-ce qu'il y a des producteur (si 0 on quitte)
	int aDesProd;

	// numTete est un entier qui détermine la "tête" courante. Si elle a changé, c'est qu'on a ajouté un caractère
	int numTete = 0;

    while (1)
    {
		aDesProd = 0;
		P(mutex_data);
		if(numTete != memoireP->tete)
		{
			numTete = memoireP->tete;
			c = (char) memoireP->f[numTete].c;
			switch(memoireP->f[numTete].idp)
			{
				case 0 : 
					w = f_haut;
					break;
				case 1 : 
					w = f_milieu1;
					break;
				case 2 : 
					w = f_milieu2;
					break;
				default :
					w = f_bas;
			}
			waddch(w,c) ;
			wrefresh(w) ; 
		}
		V(mutex_data);

		// On vérifie qu'il y ait toujours des producteurs MARCHE
		for(i = 0 ; i < MAX_PROD && aDesProd == 0 ; i++ )
			if(memoireP->tpa[i] != -1)
				aDesProd = 1;

		// S'il n'y a plus de producteurs, on quitte
		if(aDesProd == 0)
		{
			if(shmctl(shmid, IPC_RMID, 0) < 0)
			{ perror("shmctl"); exit(EXIT_FAILURE); }
		
			if(mutex_data >= 0) { del_sem(sem_key_data); }
			if(mutex_tpa >= 0) { del_sem(sem_key_tpa); }
			if(mutex_glob >= 0) { del_sem(sem_key_glob); }

			endwin() ;
			exit(EXIT_SUCCESS);
		}
		usleep(10); // Ralentissement volontaire du programme
    }
	exit(EXIT_FAILURE);
}

WINDOW *creation_fenetre(int n,int d,char *t)
{
    WINDOW *cadre ;			/* la fenetre pour le cadre */
    WINDOW *w ;			/* la fenetre de dialogue */
    
    /* creation du cadre */

    cadre= newwin(n,COLS,d,0) ;
    box(cadre,0,0) ;
    mvwprintw(cadre,0,COLS/2-strlen(t)/2,t) ;
    wrefresh(cadre) ;
    
    /* creation de la fenetre de dialogue */

    w= newwin(n-2,COLS-2,d+1,1) ;
    idlok(w,TRUE) ;
    scrollok(w,TRUE) ;	   /* mise en place du defilement (scrolling) */
    wclear(w) ;
    wrefresh(w) ;
    return w ;
}