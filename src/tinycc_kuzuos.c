// sooo ts is a bad compiler and it sücks ass 

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* Include libtcc header for API */
#include "libtcc.h"

/* Permanent storage for args — declared before tcc_main so both functions see them */
static char _tcc_infile[256];
static char _tcc_outfile[256];
static int  _tcc_output_type;

/* Read entire file into memory */
static char* read_file(const char *filename, size_t *out_size) {
    write(1, "[read_file] filename='", 22);
    if (filename) {
        int len = 0;
        while (filename[len]) len++;
        write(1, filename, len);
    } else {
        write(1, "(null)", 6);
    }
    write(1, "'\n", 2);
    
    int fd = open(filename, 0);
    if (fd < 0) return NULL;
    
    /* Get file size */
    int pos = lseek(fd, 0, 2);  /* SEEK_END = 2 */
    lseek(fd, 0, 0);             /* SEEK_SET = 0 */
    
    char *buf = malloc(pos + 1);
    if (!buf) {
        close(fd);
        return NULL;
    }
    
    int bytes_read = read(fd, buf, pos);
    close(fd);
    
    buf[bytes_read] = '\0';
    if (out_size) *out_size = bytes_read;
    return buf;
}
// if broken delete
/* Error callback for TCC - must be a regular function, not nested */
static void tcc_error_callback(void *opaque, const char *msg) {
    (void)opaque;
    write(1, "[TCC ERROR] ", 12);
    int len = 0;
    while (msg[len]) len++;
    write(1, msg, len);
    write(1, "\n", 1);
}

/* Main TinyCC compiler function */
int tcc_main(int argc, char **argv) {
    (void)argc; (void)argv;

    // Debug: show what we see
    write(1, "[tcc_main] infile='", 19);
    int inlen = 0;
    while (_tcc_infile[inlen]) inlen++;
    write(1, _tcc_infile, inlen);
    write(1, "'\n", 2);

    if (_tcc_infile[0] == '\0') {
        printf("tcc: no input file\n");
        return 1;
    }

    // test malloc before tcc_new touches it
    write(1, "[tcc_main] testing malloc...\n", 29);
    void* test = malloc(64);
    if (!test) {
        write(1, "[tcc_main] MALLOC RETURNED NULL!\n", 33);
        extern __attribute__((noreturn)) void z_exit(int code);
        z_exit(1);
    }
    write(1, "[tcc_main] malloc ok!\n", 22);
    free(test);
    write(1, "[tcc_main] free ok!\n", 20);

    TCCState *s = tcc_new();
    if (!s) {
        printf("tcc: memory allocation failed\n");
        return 1;
    }
    
    // Set up error callback to see what's failing
    /*
    // add these and delete the one i said if broken
        void error_callback(void *opaque, const char *msg) {
        write(1, "[TCC ERROR] ", 12);
        int len = 0;
        while (msg[len]) len++;
        write(1, msg, len);
        write(1, "\n", 1);
    }
    tcc_set_error_func(s, NULL, error_callback);
    
    
    */
    
    
    tcc_set_error_func(s, NULL, tcc_error_callback); // the line in question
    
    // Debug: check if tcc_new() corrupted our statics
    write(1, "[tcc_main] after tcc_new, infile='", 34);
    inlen = 0;
    while (_tcc_infile[inlen]) inlen++;
    write(1, _tcc_infile, inlen);
    write(1, "'\n", 2);
    if (tcc_set_output_type(s, _tcc_output_type) < 0) {
        printf("tcc: invalid output type\n");
        tcc_delete(s);
        return 1;
    }
    
    // Configure TCC for freestanding/nostdlib mode
    write(1, "[tcc_main] configuring for freestanding mode\n", 45);
    tcc_define_symbol(s, "__i386__", "1");
    tcc_define_symbol(s, "__linux__", "1");
    
    /* Disable any fancy CPU features that might generate 0x0F opcodes */
    tcc_define_symbol(s, "__NO_SSE__", "1");
    tcc_define_symbol(s, "__NO_MMX__", "1");
    tcc_define_symbol(s, "__NO_SSE2__", "1");
    
    tcc_set_options(s, "-nostdlib");
    tcc_set_options(s, "-nostdinc");
    tcc_set_options(s, "-m32");
    tcc_set_options(s, "-static");  /* no dynamic linking - avoids GOT/PLT/interp code paths */
    
    /* CRITICAL: Disable include path searching to prevent infinite loops */
    write(1, "[tcc_main] disabling include path search\n", 41);
    
    /* Add library path for -l flags */
    tcc_add_library_path(s, "/lib");
    tcc_add_library_path(s, "/dev/lib");

    write(1, "[tcc_main] reading file into memory\n", 36);
    size_t size;
    char *code = read_file(_tcc_infile, &size);
    if (!code) {
        printf("tcc: could not open file '%s'\n", _tcc_infile);
        tcc_delete(s);
        return 1;
    }
    
    write(1, "[tcc_main] file read, size=", 27);
    char sizebuf[16];
    int spos = 0;
    int tmpsize = (int)size;
    if (tmpsize == 0) sizebuf[spos++] = '0';
    else {
        char rev[16];
        int rp = 0;
        while (tmpsize > 0) { rev[rp++] = '0' + (tmpsize % 10); tmpsize /= 10; }
        while (rp > 0) sizebuf[spos++] = rev[--rp];
    }
    write(1, sizebuf, spos);
    write(1, "\n", 1);
    
    write(1, "[tcc_main] file contents (first 200 bytes):\n", 44);
    write(1, code, size > 200 ? 200 : size);
    write(1, "\n", 1);
    
    write(1, "[tcc_main] compiling string\n", 28);
    write(1, "[tcc_main] about to call tcc_compile_string...\n", 48);
    write(1, "[tcc_main] NOTE: If it hangs here, TinyCC is stuck in an infinite loop\n", 72);
    write(1, "[tcc_main] This is likely due to:\n", 34);
    write(1, "[tcc_main]   1. Missing include files\n", 38);
    write(1, "[tcc_main]   2. TinyCC parser bug\n", 34);
    write(1, "[tcc_main]   3. Malloc failures causing retry loops\n", 52);
    
    int compile_result = tcc_compile_string(s, code);
    
    write(1, "[tcc_main] tcc_compile_string returned: ", 40);
    if (compile_result < 0) {
        write(1, "FAILED\n", 7);
        printf("tcc: compilation failed\n");
        free(code);
        tcc_delete(s);
        return 1;
    }
    write(1, "SUCCESS\n", 8);
    free(code);

    write(1, "[tcc_main] outputting to '", 27);
    int outlen = 0;
    while (_tcc_outfile[outlen]) outlen++;
    write(1, _tcc_outfile, outlen);
    write(1, "'\n", 2);
    
    if (tcc_output_file(s, _tcc_outfile) < 0) {
        printf("tcc: output file failed\n");
        tcc_delete(s);
        return 1;
    }

    write(1, "[tcc_main] compilation successful!\n", 35);
    tcc_delete(s);
    return 0;
}

/* Entry point for KuzuOS5 - fetch args via SYS_GETARGC/SYS_GETARGV */
extern __attribute__((noreturn)) void z_exit(int code);

static inline int tcc_syscall1(int num, int arg1) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1) : "memory");
    return ret;
}

static inline int tcc_syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3) : "memory");
    return ret;
}

void _start(void) {
    write(1, "[_start] TCC entry reached\n", 27);
    /* fetch raw args */
    static char argbufs[16][256];
    static char* argv[17];

    int argc = tcc_syscall1(260, 0);
    if (argc > 16) argc = 16;
    for (int i = 0; i < argc; i++) {
        int len = tcc_syscall3(261, i, (int)argbufs[i], 256);
        argbufs[i][len < 256 ? len : 255] = '\0';
        argv[i] = argbufs[i];
    }
    argv[argc] = (char*)0;

    /* parse args into permanent static storage NOW, before any heap alloc */
    _tcc_infile[0]  = '\0';
    _tcc_outfile[0] = 'a'; _tcc_outfile[1] = '.';
    _tcc_outfile[2] = 'o'; _tcc_outfile[3] = '\0';
    _tcc_output_type = TCC_OUTPUT_OBJ;  /* default to object file, not exe - exe triggers linking */

    if (argc < 2) {
        /* let tcc_main handle the error */
        int exit_code = tcc_main(argc, argv);
        z_exit(exit_code);
        return;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'o' && argv[i][2] == '\0' && i + 1 < argc) {
                i++;
                int k = 0;
                while (argv[i][k] && k < 255) { _tcc_outfile[k] = argv[i][k]; k++; }
                _tcc_outfile[k] = '\0';
            } else if (argv[i][1] == 'c' && argv[i][2] == '\0') {
                _tcc_output_type = TCC_OUTPUT_OBJ;
            }
        } else {
            int k = 0;
            while (argv[i][k] && k < 255) { _tcc_infile[k] = argv[i][k]; k++; }
            _tcc_infile[k] = '\0';
        }
    }
    
    // Debug: show what we parsed
    write(1, "[tcc _start] argc=", 18);
    char abuf[4] = {'0' + (argc % 10), '\n', 0, 0};
    write(1, abuf, 2);
    write(1, "[tcc _start] infile='", 21);
    int inlen = 0;
    while (_tcc_infile[inlen]) inlen++;
    write(1, _tcc_infile, inlen);
    write(1, "'\n", 2);

    int exit_code = tcc_main(argc, argv);
    z_exit(exit_code);
}

/* We need to rename the real main so we can provide our _start */
int main(int argc, char **argv) {
    return tcc_main(argc, argv);
}
