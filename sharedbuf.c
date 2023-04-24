#include "sharedbuf.h"
#include "exerror.h"
#include <stdlib.h>
#include <semaphore.h>


struct sharedbuf{
    void **startbuf;
    void **endbuf;
    void **remp;
    void **addp;
    sem_t mutex;
    sem_t slots;
    sem_t items;
};


struct sharedbuf *sharedbuf(long unsigned nitems)
{
    struct sharedbuf *sbuf = (struct sharedbuf *) Malloc(sizeof(struct sharedbuf));
    sbuf->startbuf = (void **)Malloc(nitems * sizeof(void *));
    sbuf->endbuf = sbuf->startbuf + nitems;
    sbuf->addp = sbuf->remp = sbuf->startbuf;
    Sem_init(&sbuf->mutex, 0 , 1);
    Sem_init(&sbuf->slots, 0 , nitems);
    Sem_init(&sbuf->items, 0 , 0);
    return sbuf;
}

int free_sharedbuf(struct sharedbuf *sbuf)
{
    Sem_wait(&sbuf->mutex); /* is this good here? blocking...  */
    free(sbuf->startbuf);
    free(sbuf);
    return 0;
}

int sbuf_add(struct sharedbuf *sbuf, void *itemp)
{
    Sem_wait(&sbuf->slots);
    Sem_wait(&sbuf->mutex);
    *(sbuf->addp) = itemp;
    sbuf->addp++;
    if (sbuf->addp == sbuf->endbuf){
        sbuf->addp = sbuf->startbuf;
    }
    Sem_post(&sbuf->mutex);
    Sem_post(&sbuf->items);
    return 0;
}

void *sbuf_get(struct sharedbuf *sbuf)
{
    void *itemp;
    Sem_wait(&sbuf->items);
    Sem_wait(&sbuf->mutex);
    itemp = *(sbuf->remp);
    sbuf->remp++;
    if (sbuf->remp == sbuf->endbuf){
        sbuf->remp = sbuf->startbuf;
    }
    Sem_post(&sbuf->mutex);
    Sem_post(&sbuf->slots);
    return itemp;
}