#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

void *Malloc(size_t size);

void Close (int fd);

void Sigaction(int signum, const struct sigaction *act, 
                                 struct sigaction *oldact);



void Pthread_create(pthread_t *tid, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *args);

void Pthread_detach(pthread_t tid);


void Sem_init(sem_t *sem, int pshared, unsigned int value);

void Sem_wait(sem_t *sem);

void Sem_post(sem_t *sem);
