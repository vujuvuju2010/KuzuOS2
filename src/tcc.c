/* TinyCC for KuzuOS5 - Main Compiler Command */
#include "compiler.h"
#include <stdlib.h>
#include <string.h>

extern int z_write(int fd, const void* buf, size_t count);
extern int z_read(int fd, void* buf, size_t count);
extern int z_open(const char* path, int flags);
extern int z_close(int fd);
extern int z_lseek(int fd, int offset, int whence);

static void write_error(const char* msg) {
    z_write(2, msg, strlen(msg));
    z_write(2, "\n", 1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        write_error("Usage: tcc [options] file.c [-o output]");
        return 1;
    }
    
    const char* input_file = NULL;
    const char* output_file = "a.out";
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }
    
    if (!input_file) {
        write_error("Error: No input file specified");
        return 1;
    }
    
    /* Read source file */
    int fd = z_open(input_file, 0);
    if (fd < 0) {
        write_error("Error: Cannot open input file");
        return 1;
    }
    
    /* Get file size by seeking to end */
    int file_size = z_lseek(fd, 0, 2);  /* SEEK_END */
    z_lseek(fd, 0, 0);  /* SEEK_SET */
    
    /* Read file contents */
    char* source = (char*)malloc(file_size + 1);
    if (!source) {
        write_error("Error: Out of memory");
        z_close(fd);
        return 1;
    }
    
    int nread = z_read(fd, source, file_size);
    z_close(fd);
    
    if (nread != file_size) {
        write_error("Error: Failed to read source file");
        free(source);
        return 1;
    }
    source[file_size] = '\0';
    
    /* Create compiler state */
    CompilerState* compiler = compiler_new();
    if (!compiler) {
        write_error("Error: Failed to create compiler");
        free(source);
        return 1;
    }
    
    /* Compile source */
    if (compiler_compile(compiler, source) != 0) {
        write_error("Error: Compilation failed");
        compiler_free(compiler);
        free(source);
        return 1;
    }
    
    /* Write output */
    int out_fd = z_open(output_file, 0x0241);  /* O_WRONLY | O_CREAT | O_TRUNC */
    if (out_fd < 0) {
        write_error("Error: Failed to create output file");
        compiler_free(compiler);
        free(source);
        return 1;
    }
    
    z_write(out_fd, compiler->code, compiler->code_pos);
    z_close(out_fd);
    
    /* Write success message */
    z_write(1, "Compiled successfully\n", 21);
    
    compiler_free(compiler);
    free(source);
    
    return 0;
}

