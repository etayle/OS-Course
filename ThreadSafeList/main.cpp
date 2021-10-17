#include "Barrier.h"
#include <pthread.h>
#include <unistd.h>
#include <iostream>

using namespace std;

pthread_mutex_t m;
Barrier* b = new Barrier(1000);
int count = 0;
void * special_print(void *to_print) {
    int* num = (int*) to_print;
    pthread_mutex_lock(&m);
    //cout << "before barrier " << *num << endl;
    count++;
    pthread_mutex_unlock(&m);
    b->wait();
    pthread_mutex_lock(&m);
    //cout << "after barrier " << *num << endl;
    cout << count<<endl;
    pthread_mutex_unlock(&m);
    return nullptr;
}

int main(){

    pthread_mutex_init(&m, nullptr);
    pthread_t *t = new pthread_t[1000];
    int i;
    for (i = 0; i < 1000; i++) {
        int *a = new int();
        *a = i;
        pthread_create(t + i, nullptr, special_print, a);
    }
    for (i = 0; i < 1000; i++) {
        pthread_join(t[i], nullptr);
    }

    pthread_mutex_destroy(&m);
};

