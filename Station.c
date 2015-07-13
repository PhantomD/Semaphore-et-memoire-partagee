#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <signal.h>

#define POMPE 0
#define CAISSE 1 
#define ACCESMEMOIREPARTAGEE 2

typedef union semun {

	int val ; //valeur pour SETVAL
	struct semid_ds *buf ; // buffer pour IPC_SET et IPC_STAT
	unsigned short *array ;// tableau pour GETALL et SETALL
	struct seminfo *__buf; //buffer pour IPC_INFO
} semval;

void P(int smid, int numero){

	struct sembuf smbf;
	smbf.sem_num = numero;
	smbf.sem_op = -1;
	smbf.sem_flg = 0;
	semop(smid, &smbf, 1);
}

void V(int smid, int numero){

	struct sembuf smbf;
	smbf.sem_num = numero;
	smbf.sem_op = +1; 
	smbf.sem_flg = 0;
	semop(smid, &smbf, 1);
}

void delai(int duree, int precision){

	int random;
	random = (rand()/(float)RAND_MAX)*(2*precision+1);
	random = random-precision; //ça ne marche pas si c'est sur la meme ligne qu'au dessus sans doute un preoblème de parenthèses que je n'arrive pas à régler
	sleep(duree-random);

}

int main(int argc, char** argv){

	int *pMemoirePartagee;
	int i;
	int j =0;
	int pid;
	int nvoitures = 20;
	int npompes = 4;
	semval initsempompes;
	semval initsemcaisse;

	int shmid = shmget(IPC_PRIVATE, 2*sizeof(int), IPC_CREAT|0660);
	//2*sizeof car c'est pour stocker attente caisse et voitures parties
	pMemoirePartagee = (int*) shmat(shmid, NULL,0);
	pMemoirePartagee[0] = 0;
	pMemoirePartagee[1] = 0;

	if(shmid == -1){
		perror("Erreur\n");
		exit(1);
	}

	if(pMemoirePartagee == NULL){
		perror("Erreur attachement");
		exit(2);
	}

	int semid = semget(IPC_PRIVATE, 3, IPC_CREAT|0660|IPC_EXCL);

	if(semid == -1){
		perror("Erreur lors de la creation des sémaphores");
		exit(1);
	}

	initsempompes.val = npompes;// le nombre de jetons pompes
	initsemcaisse.val = 1;// le nombre de jetons caisse

	semctl(semid, POMPE, SETVAL, initsempompes);
	semctl(semid, CAISSE, SETVAL, initsemcaisse);
	semctl(semid, ACCESMEMOIREPARTAGEE, SETVAL, initsemcaisse);

	//for(i=0; i<nvoitures; i++){ 
	  while(pMemoirePartagee[1]<nvoitures){ //tant que le nombre de voitures parties inférieur au nombre de voitures
	    if(j<nvoitures){
		switch(pid = fork()) {
			case(pid_t) -1 : perror("Création impossible");
					 exit(1);

			case(pid_t) 0 : /*Création du processus fils*/

					 srand(getpid());

					 P(semid,POMPE);
					 delai(5,2);
					 V(semid,POMPE); //libère pompe

					 P(semid, ACCESMEMOIREPARTAGEE);
					 pMemoirePartagee[0]++; //incrémente le nombre de voitures qui attendent la caisse
					 V(semid,ACCESMEMOIREPARTAGEE);

					 P(semid,CAISSE);
					 P(semid,ACCESMEMOIREPARTAGEE);
					 pMemoirePartagee[0]--; //Décrémente le nombre de voitures qui attendent la caisse
					 V(semid,ACCESMEMOIREPARTAGEE);
					 delai(2,1); 
					 V(semid, CAISSE);

					 P(semid,ACCESMEMOIREPARTAGEE);
					 pMemoirePartagee[1]++; //incrémente le nombre de voitures parties
					 V(semid,ACCESMEMOIREPARTAGEE);

					 exit(0);
		}
		j++;
        
	}

	 
		printf("\nVoitures crées : %d \nPompes en service : %d\nAttente en Caisse : %d\nVoitures parties : %d\n",j,npompes,pMemoirePartagee[0],pMemoirePartagee[1]);
		sleep(1); //affiche 1 par seconde
	  }

	printf("\nVoitures crées : %d \nPompes en service : %d\nAttente en Caisse : %d\nVoitures parties : %d\n",nvoitures,npompes,pMemoirePartagee[0],pMemoirePartagee[1]);

	shmctl(shmid, IPC_RMID,0);
	semctl(semid, IPC_RMID,0);

	return 0;
}
