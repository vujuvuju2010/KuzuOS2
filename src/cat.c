// cat.c - Concatenate and print files
#include "z_syscalls.h"

static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void _start(void) {
    // Usage: cat <filename>
    // For simplicity, we'll just read from stdin and echo to stdout
    char buffer[1024];
    int bytes_read;
    
    // Read from stdin (fd 0)
    while ((bytes_read = z_read(0, buffer, sizeof(buffer))) > 0) {
        z_write(1, buffer, bytes_read);
    }
    
    z_exit(0);
}
