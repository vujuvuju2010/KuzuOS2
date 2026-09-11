#include <pthread.h>

static int dummy_mutex = 0;
static int dummy_cond = 0;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg) {
    (void)thread; (void)attr; (void)start_routine; (void)arg;
    return -1;
}

void pthread_exit(void *retval) {
    (void)retval;
}

int pthread_join(pthread_t thread, void **retval) {
    (void)thread; (void)retval;
    return -1;
}

int pthread_detach(pthread_t thread) {
    (void)thread;
    return -1;
}

pthread_t pthread_self(void) {
    return 1;
}

int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    (void)mutex; (void)attr;
    *mutex = 0;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    (void)cond; (void)attr;
    *cond = 0;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
    (void)cond;
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    (void)cond; (void)mutex;
    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime) {
    (void)cond; (void)mutex; (void)abstime;
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
    (void)cond;
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
    (void)cond;
    return 0;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    (void)key; (void)destructor;
    *key = 0;
    return 0;
}

int pthread_key_delete(pthread_key_t key) {
    (void)key;
    return 0;
}

void *pthread_getspecific(pthread_key_t key) {
    (void)key;
    return NULL;
}

int pthread_setspecific(pthread_key_t key, const void *value) {
    (void)key; (void)value;
    return 0;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    (void)once_control; (void)init_routine;
    return 0;
}

int pthread_cancel(pthread_t thread) {
    (void)thread;
    return 0;
}

int pthread_setcancelstate(int state, int *oldstate) {
    (void)state; (void)oldstate;
    return 0;
}

int pthread_setcanceltype(int type, int *oldtype) {
    (void)type; (void)oldtype;
    return 0;
}

void pthread_testcancel(void) {
}

int pthread_setschedparam(pthread_t thread, int policy,
                          const struct sched_param *param) {
    (void)thread; (void)policy; (void)param;
    return 0;
}

int pthread_getschedparam(pthread_t thread, int *policy,
                          struct sched_param *param) {
    (void)thread; (void)policy; (void)param;
    return 0;
}

int pthread_setschedprio(pthread_t thread, int priority) {
    (void)thread; (void)priority;
    return 0;
}
