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

#define RDV1 0 //Sémaphore de Rendez vous 1
#define RDV2 1 //Sémaphore de Rendez vous 2
#define N 3 //6 étant la taille du tableau

int *pMemoirePartagee;

void printTab(int* pMemoirePartagee){
	int i;
	for(i=0; i<2*N; i++){ //6 c'est la taille du tableau
		printf("%d ",pMemoirePartagee[i]);
	}
	printf("\n\n");
}

void handler(int sig){
	printf("\nTableau Final\n");
	printTab(pMemoirePartagee);
	printf("\n");
	exit(0);

}

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

int minTab(int tab[], int x, int y){
	int i;
	int min = x;
	for(i=x; i<=y; i++){
		if(tab[i] < tab[min]){
			min = i;
		}  
	}
	return min;
}
int maxTab(int tab[], int x, int y){
	int i;
	int max = x;
	for(i=x; i<=y; i++){
		if(tab[i] > tab[max]){
			max = i;
		}  
	}
	return max;
}

void swapTab(int tab[], int i1, int i2){ //remplace i1 par i2
	int tmp;
	tmp = tab[i1];
	tab[i1] = tab[i2];
	tab[i2] = tmp;
}


int main(int argc, char **argv){

	int i;
	int pid;
	semval initsem;
	int shmid = shmget(IPC_PRIVATE, 6*sizeof(int), IPC_CREAT|0660);
	pMemoirePartagee = (int*) shmat(shmid, NULL,0);
	int semid;

	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&(sa.sa_mask));
	/*Installation effective du handler*/
	sigaction(SIGCHLD, &sa, NULL); //Pour savoir quand le fils meure

	if(shmid == -1){
		perror("Erreur\n");
		exit(1);
	}

	if(pMemoirePartagee == NULL){
		perror("Erreur attachement");
		exit(2);
	}

	srand(time(NULL));
	for(i=0; i<2*6; i++){ //6 c'est la taille du tableau dans l'exemple
		pMemoirePartagee[i] = rand()%100;
	}
	printf("\nTableau aléatoire : \n");

	printTab(pMemoirePartagee);

	semid = semget(IPC_PRIVATE, 2, IPC_CREAT|0660|IPC_EXCL);
	if(semid == -1){
		perror("Erreur lors de la creation des sémaphores");
		exit(1);
	}
	initsem.val = 0;// le nombre de jeton
	semctl(semid, RDV1, SETVAL, initsem);
	semctl(semid, RDV2, SETVAL, initsem);


	switch(pid = fork()) {
		case(pid_t) -1 : perror("Création impossible");
				 exit(1);

		case(pid_t) 0 : /*Création du processus fils*/
				// printf("Fils %d\n", getpid());
				 fflush(stdout);
				 int echange = 1;
				 while(echange){

					 int petit = minTab(pMemoirePartagee, 3, 5);
					 P(semid, RDV1);

					 if(pMemoirePartagee[petit] > pMemoirePartagee[0]){
						 echange = 0;
					 }
					 else{
						 swapTab(pMemoirePartagee,0,petit);
						 printf("L'indice du plus petit du 2ème sous tableau : %d \n",petit);
						 printf("Echange effectué par le fils \n");
						 printTab(pMemoirePartagee);	
					 }
					 V(semid, RDV2);
				 }
				 exit(2);
	}

	/*Processus père*/
	while(1){ //Pour attendre la fin du traitement par le fils

		int grand = maxTab(pMemoirePartagee, 0,2);
		printf("L'indice du plus grand du 1er sous tableau : %d \n", grand);
		swapTab(pMemoirePartagee, 0, grand);
		printf("Echange effectué par le père %d\n", getppid());
		printTab(pMemoirePartagee);

		V(semid, RDV1);
		P(semid, RDV2);
	}

	shmctl(shmid, IPC_RMID,0);
	semctl(semid, IPC_RMID,0);

	return 0;

}
