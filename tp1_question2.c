#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <stdio.h>

#define NB_CAMIONS 4
#define NB_VOITURES 4
#define NB_VEHICULES (NB_CAMIONS + NB_VOITURES)

#define ATTENDRE 1
#define RIEN 2
#define TRAVERSER 3

pthread_mutex_t sc; // mutex sur le pont
sem_t sempriv[NB_VEHICULES];// la semaphore sempriv[i] sur etat[i]
int etat[NB_VEHICULES]; // etat des vehicules : attendre | rien | traverser
int nb_camions_bloques=0;
int seuil=0; 

void attendre(double max);
int tirage_aleatoire(double max);

void *voiture(void *args);
void *camion(void *args);

// suspend l'exécution du thread appelant jusqu'à ce que soit le temps ait expiré
void attendre(double max)
{
    struct timespec delai;
    delai.tv_sec = tirage_aleatoire(max);
    delai.tv_nsec = 0;
    nanosleep(&delai, NULL);
}

int tirage_aleatoire(double max)
{
    int j = (int)(max * rand() / (RAND_MAX + 1.0));
    if (j < 1)
        return 1;
    return j;
}


void acceder_au_pont(int tonnes,int id)
{    //  permettant à une tâche de prendre le verrou
    pthread_mutex_lock(&sc);
    if (tonnes+seuil <= 15){
        seuil += tonnes;
        etat[id]=TRAVERSER;
        //  permettant à une tâche de signaler l'événement evt
        sem_post(&sempriv[id]);
    }
    else {
        etat[id]=ATTENDRE;
        if(tonnes==15) // la vehicule est une camion 
            nb_camions_bloques++;
    }
    pthread_mutex_unlock(&sc);
    sem_wait(&sempriv[id]);
}

void quitter_le_pont(int tonnes,int pid)
{
    int i;
    
    pthread_mutex_lock(&sc);
    etat[pid]=RIEN;
    seuil -= tonnes;
    for(i=0;i<NB_CAMIONS;i++){
        if(etat[i]==ATTENDRE && seuil==0){
            sem_post(&sempriv[i]);
            seuil=15;
            nb_camions_bloques--;
        }
    }
    for(i=NB_CAMIONS;i<NB_VEHICULES;i++){
        if( seuil<15 && nb_camions_bloques==0 && etat[i]==ATTENDRE){
            seuil+=5;
            sem_post(&sempriv[i]);
        }
    }
    pthread_mutex_unlock(&sc);

}
void *voiture(void *args)
{
    int pid = *((int *)args);
    attendre(5.0);
    acceder_au_pont(5,pid);
    printf("Voiture %d traverse le pont \n", pid);
    attendre(5.0);
    printf("Voiture %d quitte le pont\n", pid);
    quitter_le_pont(5,pid);
    pthread_exit(NULL);
}

void *camion(void *args)
{
    int pid = *((int *)args);
    attendre(5.0);
    acceder_au_pont(15,pid);
    printf("Camion %d traverse le pont \n", pid);
    attendre(5.0);
    printf("Camion %d quitte le pont\n", pid);
    quitter_le_pont(15,pid);
    pthread_exit(NULL);
}



int main()
{
    int i;
    pthread_t id;
    // initialiser le tableau etat et les semaphores sur les elementss des tableaux
    for(i=0;i<NB_VEHICULES;i++){
        etat[i]=RIEN;
        sem_init(&sempriv[i],0,0);
    }
    pthread_mutex_init(&sc,0);

    for (i = 0; i < NB_VEHICULES; i++)
    {
        int *j = (int *)malloc(sizeof(int));
        *j = i;
        if (i < NB_CAMIONS)
            pthread_create(&id, NULL, camion, j);
        else
            pthread_create(&id, NULL, voiture, j);
    }
    pthread_exit(NULL);
}