/* libc/src/time/time.c - Time functions */
#include <time.h>
#include "../../src/z_syscalls.h"

time_t time(time_t *t) {
    /* Stub - return fixed timestamp */
    time_t current = 0;
    if (t) *t = current;
    return current;
}

clock_t clock(void) {
    /* Return time in microseconds as clock ticks */
    return 0;
}

struct tm *localtime(const time_t *timep) {
    /* Stub - return minimal tm structure */
    static struct tm tm_buf = {0};
    if (!timep) return NULL;
    
    tm_buf.tm_sec = 0;
    tm_buf.tm_min = 0;
    tm_buf.tm_hour = 0;
    tm_buf.tm_mday = 1;
    tm_buf.tm_mon = 0;
    tm_buf.tm_year = 70;
    tm_buf.tm_wday = 4;
    tm_buf.tm_yday = 0;
    tm_buf.tm_isdst = 0;
    return &tm_buf;
}

struct tm *gmtime(const time_t *timep) {
    return localtime(timep);
}

char *ctime(const time_t *timep) {
    static char buf[26] = "Wed Jun 30 21:49:08 1993\n";
    return buf;
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
    /* Minimal stub */
    if (max > 0) s[0] = '\0';
    return 0;
}
