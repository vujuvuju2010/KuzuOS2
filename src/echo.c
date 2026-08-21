#include "z_syscalls.h"

static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void print(const char* s) {
    z_write(1, s, strlen(s));
}

void _start(void) {
    print("Echo Program - Type something and press Enter (Ctrl+C to exit)\n");
    print("> ");
    
    char buffer[256];
    
    while (1) {
        // Read a line from stdin
        int bytes_read = z_read(0, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            break;  // EOF or error
        }
        
        // Null terminate
        buffer[bytes_read] = '\0';
        
        // Check for exit command
        if (buffer[0] == 'e' && buffer[1] == 'x' && buffer[2] == 'i' && buffer[3] == 't') {
            print("Goodbye!\n");
            break;
        }
        
        // Echo it back
        print("You typed: ");
        z_write(1, buffer, bytes_read);
        
        // Prompt again
        print("> ");
    }
    
    z_exit(0);
}
