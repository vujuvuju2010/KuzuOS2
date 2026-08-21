#include <z_syscalls.h>

// Forward declare vim's real main
extern int main(int argc, char **argv);

// This will be our KuzuOS5 entry point
int vim_main_wrapper(int argc, char **argv) {
    // Just forward to Vim's main - the libc will handle syscalls
    return main(argc, argv);
}

