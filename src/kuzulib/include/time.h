/* KuzuOS C Library - time.h for Tor compatibility */
#ifndef _TIME_H
#define _TIME_H

#include <stdint.h>
#include <stddef.h>

/* Time types */
typedef long time_t;
typedef long suseconds_t;
typedef long clock_t;

/* Timeval structure */
struct timeval {
    time_t      tv_sec;   /* seconds */
    suseconds_t tv_usec;  /* microseconds */
};

/* Timespec structure */
struct timespec {
    time_t tv_sec;   /* seconds */
    long   tv_nsec;  /* nanoseconds */
};

/* Broken-down time structure */
struct tm {
    int tm_sec;    /* seconds after the minute [0-60] */
    int tm_min;    /* minutes after the hour [0-59] */
    int tm_hour;   /* hours since midnight [0-23] */
    int tm_mday;   /* day of the month [1-31] */
    int tm_mon;    /* months since January [0-11] */
    int tm_year;   /* years since 1900 */
    int tm_wday;   /* days since Sunday [0-6] */
    int tm_yday;   /* days since January 1 [0-365] */
    int tm_isdst;  /* Daylight Saving Time flag */
    long tm_gmtoff; /* offset from UTC in seconds */
    char *tm_zone; /* timezone abbreviation */
};

/* Timezone structure */
struct timezone {
    int tz_minuteswest; /* minutes west of Greenwich */
    int tz_dsttime;     /* type of DST correction */
};

/* Clock ID type */
typedef int clockid_t;

/* Timer ID type */
typedef int timer_t;

/* Constants */
#define CLOCKS_PER_SEC 1000000L
#define CLK_TCK CLOCKS_PER_SEC

/* Time conversion macros */
#define TIMER_ABSTIME 1

/* Functions */
time_t time(time_t *tloc);
int gettimeofday(struct timeval *tv, struct timezone *tz);
int settimeofday(const struct timeval *tv, const struct timezone *tz);

/* Time conversion */
struct tm *gmtime(const time_t *timep);
struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime(const time_t *timep);
struct tm *localtime_r(const time_t *timep, struct tm *result);
time_t mktime(struct tm *tm);
time_t timegm(struct tm *tm);

/* Format time */
char *asctime(const struct tm *tm);
char *asctime_r(const struct tm *tm, char *buf);
char *ctime(const time_t *timep);
char *ctime_r(const time_t *timep, char *buf);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);

/* Sleep */
unsigned int sleep(unsigned int seconds);
int usleep(suseconds_t usec);
int nanosleep(const struct timespec *req, struct timespec *rem);

/* Clock functions */
int clock_gettime(clockid_t clk_id, struct timespec *tp);
int clock_settime(clockid_t clk_id, const struct timespec *tp);
int clock_getres(clockid_t clk_id, struct timespec *tp);

/* Timer functions */
int timer_create(clockid_t clk_id, struct sigevent *sevp, timer_t *timerid);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
                 struct itimerspec *ovalue);
int timer_gettime(timer_t timerid, struct itimerspec *value);
int timer_getoverrun(timer_t timerid);
int timer_delete(timer_t timerid);

/* Interval timer structure */
struct itimerspec {
    struct timespec it_interval;
    struct timespec it_value;
};

/* Clock IDs */
#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3

/* Timer signal */
#define TIMER_SIG SIGALRM

#endif /* _TIME_H */
