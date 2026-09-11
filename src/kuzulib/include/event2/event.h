/* KuzuOS C Library - event2/event.h for Tor compatibility */
#ifndef _EVENT2_EVENT_H
#define _EVENT2_EVENT_H

#include <stdint.h>
#include <stddef.h>

/* struct timeval for event timeout */
struct timeval {
    long tv_sec;
    long tv_usec;
};

/* Socket type for libevent - matches int file descriptor */
typedef int evutil_socket_t;

/* Callback function type */
typedef void (*event_callback_fn)(int, short, void *);

struct event_base;
struct event;

/* Event structure */
struct event {
    evutil_socket_t ev_fd;
    short ev_events;
    struct event_base *ev_base;
    event_callback_fn ev_callback;
    void *ev_arg;
    struct event *ev_next;
    struct timeval ev_timeout;
};

struct event_base *event_base_new(void);
void event_base_free(struct event_base *base);
int event_base_dispatch(struct event_base *base);
int event_base_loopexit(struct event_base *base, const struct timeval *tv);

struct event *event_new(struct event_base *base, int fd, short what,
                        event_callback_fn cb, void *arg);
void event_free(struct event *ev);
int event_add(struct event *ev, const struct timeval *tv);
int event_del(struct event *ev);

#define EV_READ     0x01
#define EV_WRITE    0x02
#define EV_PERSIST  0x10
#define EV_ET       0x20
#define EV_SIGNAL   0x08
#define EV_TIMEOUT  0x04

/* Event activation */
void event_active(struct event *ev, int what, short ncalls);

/* Signal events */
struct event *evsignal_new(struct event_base *base, evutil_socket_t fd,
                           event_callback_fn cb, void *arg);
int evsignal_add(struct event *ev, const struct timeval *tv);
int evsignal_del(struct event *ev);

#endif /* _EVENT2_EVENT_H */
