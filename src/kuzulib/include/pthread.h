/* KuzuOS C Library - pthread.h stub for Tor compatibility */
#ifndef _PTHREAD_H
#define _PTHREAD_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

/* Scheduling parameters */
struct sched_param {
    int sched_priority;
};

/* Thread types - stub implementations for KuzuOS */
typedef unsigned long pthread_t;
typedef int pthread_mutex_t;
typedef int pthread_cond_t;
typedef int pthread_rwlock_t;
typedef int pthread_barrier_t;
typedef int pthread_spinlock_t;

/* Thread key type */
typedef unsigned int pthread_key_t;

/* Mutex attributes */
typedef struct {
    int type;
    int protocol;
    int pshared;
} pthread_mutexattr_t;

/* Condition attributes */
typedef struct {
    int pshared;
} pthread_condattr_t;

/* Thread attributes */
typedef struct {
    void *stackaddr;
    size_t stacksize;
    int detachstate;
    int inheritsched;
    int schedpolicy;
    struct sched_param schedparam;
} pthread_attr_t;

/* Mutex types */
#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_ERRORCHECK 1
#define PTHREAD_MUTEX_RECURSIVE  2
#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL

/* Mutex initializers */
#define PTHREAD_MUTEX_INITIALIZER 0

/* Condition variable initializers */
#define PTHREAD_COND_INITIALIZER 0

/* Thread detach states */
#define PTHREAD_CREATE_JOINABLE  0
#define PTHREAD_CREATE_DETACHED  1

/* Scheduling policies */
#define PTHREAD_INHERIT_SCHED    0
#define PTHREAD_EXPLICIT_SCHED   1

/* Process shared attribute */
#define PTHREAD_PROCESS_PRIVATE  0
#define PTHREAD_PROCESS_SHARED   1

/* Thread functions - stub implementations */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
void pthread_exit(void *retval);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);

/* Mutex functions - stub implementations */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

/* Condition variable functions - stub implementations */
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

/* Thread key functions - stub implementations */
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

/* Once control */
typedef int pthread_once_t;
#define PTHREAD_ONCE_INIT 0
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

/* Cancelability */
#define PTHREAD_CANCEL_ENABLE  0
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_DEFERRED  0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);

/* Scheduling functions */
int pthread_setschedparam(pthread_t thread, int policy,
                          const struct sched_param *param);
int pthread_getschedparam(pthread_t thread, int *policy,
                          struct sched_param *param);
int pthread_setschedprio(pthread_t thread, int priority);

#endif /* _PTHREAD_H */
