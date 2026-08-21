#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

typedef int sem_t;

int sem_init(sem_t *, int, unsigned int);
int sem_destroy(sem_t *);
int sem_wait(sem_t *);
int sem_post(sem_t *);

#endif
