#ifndef BARRIER_H_
#define BARRIER_H_

#include <semaphore.h>


class Barrier {
public:
    Barrier(unsigned int num_of_threads);
    void wait();
    ~Barrier();

    sem_t mutex;
    sem_t reuse;
    sem_t barrier;
    unsigned int count;
    unsigned int n;
};

#endif // BARRIER_H_

