// cat.c - Concatenate and print files
#include "z_syscalls.h"

static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void write_str(const char* str) {
    z_write(1, str, strlen(str));
}

void _start(void) {
    char args[256];
    char buffer[1024];
    int bytes_read;
    int fd;
    
    // Get command arguments (returns args starting from argv[1])
    int arg_len = z_getcmdargs(args, sizeof(args));
    if (arg_len < 0) {
        write_str("cat: failed to get arguments\n");
        z_exit(1);
    }
    
    // Check if filename was provided
    if (arg_len == 0 || args[0] == '\0') {
        write_str("Usage: cat <filename>\n");
        z_exit(1);
    }
    
    // The first argument is the filename
    char* filename = args;
    
    // Open the file
    fd = z_openat(AT_FDCWD, filename, O_RDONLY);
    if (fd < 0) {
        write_str("cat: ");
        write_str(filename);
        write_str(": cannot open file\n");
        z_exit(1);
    }
    
    // Read and print file contents
    while ((bytes_read = z_read(fd, buffer, sizeof(buffer))) > 0) {
        z_write(1, buffer, bytes_read);
    }
    
    // Close the file
    z_close(fd);
    
    z_exit(0);
}
