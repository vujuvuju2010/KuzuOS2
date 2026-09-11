/* Tor Daemon Main Entry Point for KuzuOS
 * This is the actual Tor 0.4.8.9 port with KuzuOS syscall integration
 */
#include "orconfig.h"

/* String function for length calculation */
static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

/* Syscall interface */
static inline int syscall0(int n) {
    int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n)); return r;
}
static inline int syscall1(int n, int a) {
    int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a)); return r;
}
static inline int syscall2(int n, int a, int b) {
    int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b)); return r;
}
static inline int syscall3(int n, int a, int b, int c) {
    int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c)); return r;
}

/* Syscall numbers */
#define SYS_EXIT        1
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_NET_SOCKET  401
#define SYS_NET_BIND    405
#define SYS_NET_POLL    406
#define SYS_NET_SEND    402
#define SYS_NET_RECV    403
#define SYS_NET_CLOSE   404
#define SYS_NET_LISTEN  413
#define SYS_NET_ACCEPT  414
#define SYS_NET_CONNECT 415

/* Standard file descriptors */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* Simple output for early boot */
static void tor_log(const char* msg) {
    syscall3(SYS_WRITE, STDOUT_FILENO, (long)msg, strlen(msg));
}

/* Main Tor entry point - calls into the actual Tor code */
extern int tor_run_main(int argc, char** argv);

/*
 * KuzuOS convention: the shell execs "/dev/tor".  That is fine as long as
 * "/dev/tor" is an ELF file.  We do not care what argv[] the kernel
 * constructed; we just want to start Tor with sane defaults.
 *
 * The real process entrypoint is z_entry() in the loader, which jumps to
 * the ELF header's e_entry.  For this binary, e_entry points at _start.
 */
void _start(void) {
    char* argv[8];
    int argc = 0;

    argv[argc++] = "tor"; /* argv[0] as Tor expects */
    argv[argc++] = "--defaults-torrc";
    argv[argc++] = "/etc/services/torrc";
    argv[argc++] = "--DataDirectory";
    argv[argc++] = "/tor/data";
    argv[argc] = NULL;

    tor_log("=== Tor Daemon for KuzuOS ===\n");
    tor_log("Based on Tor 0.4.8.9 source code\n");
    tor_log("Starting hidden service...\n\n");

    int result = tor_run_main(argc, argv);

    syscall1(SYS_EXIT, result);
}
