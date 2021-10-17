#include "Barrier.h"

#include <semaphore.h>

Barrier::Barrier(unsigned int num_of_threads):count(0), n(num_of_threads){
    sem_init(&mutex, 0,1);
    sem_init(&reuse, 0, num_of_threads);
    sem_init(&barrier, 0, 0);
}

Barrier::~Barrier() {
    sem_destroy(&mutex);
    sem_destroy(&reuse);
    sem_destroy(&barrier);
}

void Barrier::wait() {
    sem_wait(&reuse); //at the start of program reuse = n
    sem_wait(&mutex);
    count++;
    if(count==n){
        for(unsigned int i=0;i<n;i++) {
            sem_post(&barrier);
        }
    }
    sem_post(&mutex);

    sem_wait(&barrier); //here until the last thread start heve reuse = 1, barrier = -(n-1)

    sem_wait(&mutex);
    count--;
    if(count==0){
        for(unsigned int i=0;i<n;i++) {
            sem_post(&reuse);
        }
    }
    sem_post(&mutex);
}
