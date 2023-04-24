#include <semaphore.h>

struct sharedbuf;


struct sharedbuf *sharedbuf(long unsigned nitems);

int free_sharedbuf(struct sharedbuf *sbuf);

int sbuf_add(struct sharedbuf *sbuf, void *itemp);

void *sbuf_get(struct sharedbuf *sbuf);