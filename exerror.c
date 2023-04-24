#include "exerror.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s", msg, strerror(errno));
    exit(1);
}

void posix_error(char *msg, int errcode)
{
    fprintf(stderr, "%s: %s", msg, strerror(errcode));
    exit(1);
}

void *Malloc(size_t size)
{
    void *res;
    if ((res = malloc(size)) == NULL){
        unix_error("malloc error");
    }
    return res;
}

void Close (int fd)
{
    if (close(fd) != 0){
        unix_error("close error");
    }
}

void Sigaction(int signum, const struct sigaction *act, 
                                 struct sigaction *oldact)
{
    if (sigaction(signum, act, oldact) != 0){
        unix_error("sigaction error");
    }
}

void Pthread_create(pthread_t *tid, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *args)
{
    pthread_t errcode;
    if ((errcode = pthread_create(tid, attr, start_routine, args)) != 0){
        posix_error("pthread_create error:", errcode);
    }
}

void Pthread_detach(pthread_t tid)
{
    pthread_t errcode;
    if ((errcode = pthread_detach(tid)) != 0){
        posix_error("pthread_detach error:", errcode);
    }
}

void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (sem_init(sem, pshared, value) == -1){
        unix_error("sem_init error");
    }
}

void Sem_wait(sem_t *sem)
{
    while(sem_wait(sem) != 0){
        if (errno !=  EINTR){
            unix_error("sem_wait error");
        }
    }
}

void Sem_post(sem_t *sem)
{
    if (sem_post(sem) == -1){
        unix_error("sem_post error");
    }
}