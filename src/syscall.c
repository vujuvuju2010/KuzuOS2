#include "syscall.h"
#include "vga.h"
#include "filesystem.h"
#include "process.h"
#include "memory.h"
#include "elf.h"
#include "interrupts.h"

// External variables from elf.c
extern int program_exit_requested;

// File descriptor tracking
#define MAX_FDS 256
#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

static struct {
    int used;
    char* path;
    uint32_t offset;
    uint32_t mode;  // O_RDONLY, O_WRONLY, etc
} fd_table[MAX_FDS];

static int next_fd = 3;  // Start after stdin/stdout/stderr

void syscall_init() {
    // Initialize file descriptor table
    for (int i = 0; i < MAX_FDS; i++) {
        fd_table[i].used = 0;
        fd_table[i].path = 0;
        fd_table[i].offset = 0;
        fd_table[i].mode = 0;
    }
    
    // Setup stdin/stdout/stderr
    fd_table[FD_STDIN].used = 1;
    fd_table[FD_STDIN].mode = 0;  // O_RDONLY
    
    fd_table[FD_STDOUT].used = 1;
    fd_table[FD_STDOUT].mode = 1;  // O_WRONLY
    
    fd_table[FD_STDERR].used = 1;
    fd_table[FD_STDERR].mode = 1;  // O_WRONLY
}

// Syscall handler - handles all Linux syscalls
int32_t handle_syscall(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6) {
    (void)arg4; (void)arg5; (void)arg6;  // Unused for now
    
    // Debug: Print syscall number
    print_color("[SYSCALL ", VGA_COLOR_LIGHT_GREY);
    char sc_buf[16];
    int sc_pos = 0;
    uint32_t sc = syscall_num;
    if (sc == 0) {
        sc_buf[sc_pos++] = '0';
    } else {
        char digits[16];
        int dpos = 0;
        while (sc > 0 && dpos < 15) {
            digits[dpos++] = '0' + (sc % 10);
            sc /= 10;
        }
        for (int i = dpos - 1; i >= 0; i--) {
            sc_buf[sc_pos++] = digits[i];
        }
    }
    sc_buf[sc_pos] = '\0';
    print(sc_buf);
    print("]\n");
    
    switch (syscall_num) {
        case SYS_EXIT:
        case SYS_EXIT_GROUP:
            // Exit program
            print_color("\n[Program exited with code ", VGA_COLOR_LIGHT_GREY);
            // Simple int to string
            char code_buf[16];
            int code = (int)arg1;
            int pos = 0;
            if (code < 0) {
                code_buf[pos++] = '-';
                code = -code;
            }
            if (code == 0) {
                code_buf[pos++] = '0';
            } else {
                char digits[16];
                int dpos = 0;
                while (code > 0) {
                    digits[dpos++] = '0' + (code % 10);
                    code /= 10;
                }
                for (int i = dpos - 1; i >= 0; i--) {
                    code_buf[pos++] = digits[i];
                }
            }
            code_buf[pos] = '\0';
            print(code_buf);
            print("]\n");
            
            // Set exit flag - interrupt handler will modify return EIP
            program_exit_requested = 1;
            return 0;
            
        case SYS_WRITE:
            {
                int fd = (int)arg1;
                char* buf = (char*)arg2;
                uint32_t count = arg3;
                
                if (fd == FD_STDOUT || fd == FD_STDERR) {
                    // Write to console
                    for (uint32_t i = 0; i < count; i++) {
                        putchar(buf[i]);
                    }
                    return count;
                } else if (fd_table[fd].used) {
                    // Write to file (simplified)
                    // TODO: Implement file writing
                    return -1;  // ENOSYS
                }
                return -1;  // EBADF
            }
            
        case SYS_READ:
            {
                int fd = (int)arg1;
                char* buf = (char*)arg2;
                uint32_t count = arg3;
                
                if (fd == FD_STDIN) {
                    // Read from keyboard (simplified - read one char at a time)
                    // TODO: Implement proper keyboard reading
                    return 0;  // No input available for now
                } else if (fd_table[fd].used) {
                    // Read from file
                    int read = fs_read_file(fd_table[fd].path, buf, count);
                    if (read > 0) {
                        fd_table[fd].offset += read;
                        return read;
                    }
                    return 0;  // EOF
                }
                return -1;  // EBADF
            }
            
        case SYS_OPEN:
            {
                char* path = (char*)arg1;
                int flags = (int)arg2;
                int mode = (int)arg3;
                
                (void)flags; (void)mode;  // For now
                
                // Find free file descriptor
                int fd = -1;
                for (int i = next_fd; i < MAX_FDS; i++) {
                    if (!fd_table[i].used) {
                        fd = i;
                        next_fd = i + 1;
                        break;
                    }
                }
                
                if (fd == -1) {
                    return -1;  // EMFILE
                }
                
                // Check if file exists
                if (!fs_any_exists(path)) {
                    return -1;  // ENOENT
                }
                
                fd_table[fd].used = 1;
                fd_table[fd].path = path;  // Just store pointer (should copy in real impl)
                fd_table[fd].offset = 0;
                fd_table[fd].mode = flags;
                
                return fd;
            }
            
        case SYS_CLOSE:
            {
                int fd = (int)arg1;
                if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].used) {
                    return -1;  // EBADF
                }
                fd_table[fd].used = 0;
                return 0;
            }
            
        case SYS_GETPID:
            // Return process ID (simplified)
            return 1;
            
        case SYS_GETUID:
            return 0;  // root
            
        case SYS_GETGID:
            return 0;  // root
            
        case SYS_IOCTL:
            // I/O control - mostly unsupported
            return -1;  // ENOSYS
            
        case SYS_FCNTL:
            // File control - simplified
            return 0;
            
        case SYS_LSEEK:
            {
                int fd = (int)arg1;
                int32_t offset = (int32_t)arg2;
                int whence = (int)arg3;
                
                if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].used) {
                    return -1;  // EBADF
                }
                
                if (whence == 0) {  // SEEK_SET
                    fd_table[fd].offset = offset;
                } else if (whence == 1) {  // SEEK_CUR
                    fd_table[fd].offset += offset;
                } else if (whence == 2) {  // SEEK_END
                    // TODO: Get file size
                    return -1;  // ENOSYS
                }
                
                return fd_table[fd].offset;
            }
            
        case SYS_UNLINK:
            {
                char* path = (char*)arg1;
                if (fs_delete_file(path, 0) == 0) {
                    return 0;
                }
                return -1;  // ENOENT
            }
            
        case SYS_MKDIR:
            {
                char* path = (char*)arg1;
                int mode = (int)arg2;
                (void)mode;
                if (fs_create_directory(path) == 0) {
                    return 0;
                }
                return -1;  // EEXIST or error
            }
            
        case SYS_RMDIR:
            {
                char* path = (char*)arg1;
                if (fs_delete_file(path, 0) == 0) {
                    return 0;
                }
                return -1;  // ENOENT
            }
            
        case SYS_CHDIR:
            {
                char* path = (char*)arg1;
                // TODO: Implement chdir
                (void)path;
                return 0;  // Stub
            }
            
        case SYS_GETCWD:
            {
                char* buf = (char*)arg1;
                uint32_t size = arg2;
                // Return current directory
                if (size > 2) {
                    buf[0] = '/';
                    buf[1] = '\0';
                    return 1;
                }
                return -1;
            }
            
        case SYS_STAT:
        case SYS_FSTAT:
        case SYS_LSTAT:
            // File stat - return stub
            {
                // struct stat* st = (struct stat*)arg2;
                // TODO: Fill stat structure
                return 0;  // Stub
            }
            
        case SYS_TIME:
            {
                uint32_t* tloc = (uint32_t*)arg1;
                // Return current time (stub)
                uint32_t time = 1609459200;  // 2021-01-01 00:00:00 UTC
                if (tloc) {
                    *tloc = time;
                }
                return time;
            }
            
        case SYS_GETTIMEOFDAY:
            {
                // struct timeval* tv = (struct timeval*)arg1;
                // TODO: Fill timeval
                return 0;  // Stub
            }
            
        case SYS_UNAME:
            {
                // struct utsname* buf = (struct utsname*)arg1;
                // TODO: Fill utsname with system info
                return 0;  // Stub
            }
            
        case SYS_NANOSLEEP:
            // Sleep - just return immediately for now
            return 0;
            
        case SYS_SCHED_YIELD:  // SYS_YIELD is an alias (same number 158)
            // Yield CPU
            return 0;
            
        case SYS_FORK:
        case SYS_VFORK:
        case SYS_CLONE:
            // Process creation - not supported yet
            return -1;  // ENOSYS
            
        case SYS_EXECVE:
            {
                // Linux execve signature: int execve(const char *pathname, char *const argv[], char *const envp[]);
                // x86 syscall convention: ebx=pathname, ecx=argv, edx=envp
                char* pathname = (char*)arg1;
                char** argv_user = (char**)arg2;
                char** envp_user = (char**)arg3;
                
                // Validate pathname pointer (basic check)
                if (!pathname || (uint32_t)pathname < 0x1000 || (uint32_t)pathname > 0xFFFFFFFF) {
                    return -14;  // EFAULT
                }
                
                // Read pathname from user space (max 256 chars)
                char pathname_buf[256];
                int pathname_len = 0;
                for (int i = 0; i < 255; i++) {
                    char c = pathname[i];
                    pathname_buf[i] = c;
                    if (c == 0) {
                        pathname_len = i;
                        break;
                    }
                }
                pathname_buf[255] = 0;
                
                if (pathname_len == 0) {
                    return -2;  // ENOENT
                }
                
                // Parse argv array from user space
                // Limit to 64 arguments to prevent excessive memory usage
                #define MAX_ARGS 64
                char* argv_kernel[MAX_ARGS + 1];
                int argc = 0;
                
                if (argv_user) {
                    // Validate argv pointer
                    if ((uint32_t)argv_user < 0x1000 || (uint32_t)argv_user > 0xFFFFFFFF) {
                        return -14;  // EFAULT
                    }
                    
                    // Read argv array
                    for (int i = 0; i < MAX_ARGS; i++) {
                        char* arg_ptr = argv_user[i];
                        if (!arg_ptr) {
                            break;  // NULL terminator
                        }
                        
                        // Validate argument pointer
                        if ((uint32_t)arg_ptr < 0x1000 || (uint32_t)arg_ptr > 0xFFFFFFFF) {
                            // Free already allocated args
                            for (int j = 0; j < i; j++) {
                                kfree(argv_kernel[j]);
                            }
                            return -14;  // EFAULT
                        }
                        
                        // Allocate kernel space for argument string
                        char* arg_buf = (char*)kmalloc(256);
                        if (!arg_buf) {
                            // Free already allocated args
                            for (int j = 0; j < i; j++) {
                                kfree(argv_kernel[j]);
                            }
                            return -12;  // ENOMEM
                        }
                        
                        // Copy argument string
                        int arg_len = 0;
                        for (int j = 0; j < 255; j++) {
                            char c = arg_ptr[j];
                            arg_buf[j] = c;
                            if (c == 0) {
                                arg_len = j;
                                break;
                            }
                        }
                        arg_buf[255] = 0;
                        
                        argv_kernel[argc++] = arg_buf;
                    }
                }
                argv_kernel[argc] = 0;  // NULL terminator
                
                // Parse envp array from user space
                // Limit to 64 environment variables
                #define MAX_ENV 64
                char* envp_kernel[MAX_ENV + 1];
                int envc = 0;
                
                if (envp_user) {
                    // Validate envp pointer
                    if ((uint32_t)envp_user < 0x1000 || (uint32_t)envp_user > 0xFFFFFFFF) {
                        // Free argv strings
                        for (int i = 0; i < argc; i++) {
                            kfree(argv_kernel[i]);
                        }
                        return -14;  // EFAULT
                    }
                    
                    // Read envp array
                    for (int i = 0; i < MAX_ENV; i++) {
                        char* env_ptr = envp_user[i];
                        if (!env_ptr) {
                            break;  // NULL terminator
                        }
                        
                        // Validate environment variable pointer
                        if ((uint32_t)env_ptr < 0x1000 || (uint32_t)env_ptr > 0xFFFFFFFF) {
                            // Free argv and envp strings
                            for (int j = 0; j < argc; j++) {
                                kfree(argv_kernel[j]);
                            }
                            for (int j = 0; j < i; j++) {
                                kfree(envp_kernel[j]);
                            }
                            return -14;  // EFAULT
                        }
                        
                        // Allocate kernel space for environment variable string
                        char* env_buf = (char*)kmalloc(512);
                        if (!env_buf) {
                            // Free already allocated strings
                            for (int j = 0; j < argc; j++) {
                                kfree(argv_kernel[j]);
                            }
                            for (int j = 0; j < i; j++) {
                                kfree(envp_kernel[j]);
                            }
                            return -12;  // ENOMEM
                        }
                        
                        // Copy environment variable string
                        int env_len = 0;
                        for (int j = 0; j < 511; j++) {
                            char c = env_ptr[j];
                            env_buf[j] = c;
                            if (c == 0) {
                                env_len = j;
                                break;
                            }
                        }
                        env_buf[511] = 0;
                        
                        envp_kernel[envc++] = env_buf;
                    }
                }
                envp_kernel[envc] = 0;  // NULL terminator
                
                // Execute the program
                int result = elf_load_and_execve(pathname_buf, argv_kernel, envp_kernel);
                
                // Free kernel copies of strings
                for (int i = 0; i < argc; i++) {
                    kfree(argv_kernel[i]);
                }
                for (int i = 0; i < envc; i++) {
                    kfree(envp_kernel[i]);
                }
                
                // If execve succeeds, it never returns (process is replaced)
                // If it fails, return error code
                if (result != 0) {
                    return -2;  // ENOENT (file not found or execution failed)
                }
                
                // Should never reach here if execve succeeds
                return 0;
            }
            
        case SYS_WAITPID:
            // Wait for process - not supported
            return -1;  // ENOSYS
            
        case SYS_KILL:
            // Send signal - not supported
            return -1;  // ENOSYS
            
        case SYS_MMAP:
        case SYS_MMAP2:
        case SYS_MUNMAP:
            // Memory mapping - simplified
            if (syscall_num == SYS_MMAP || syscall_num == SYS_MMAP2) {
                void* addr = (void*)arg1;
                uint32_t len = arg2;
                int prot = (int)arg3;
                int flags = (int)arg4;
                int fd = (int)arg5;
                uint32_t offset = arg6;
                
                (void)prot; (void)flags; (void)fd; (void)offset;
                
                // Simple allocation
                if (addr == 0) {
                    void* ptr = kmalloc(len);
                    return (int32_t)ptr;
                }
                return (int32_t)addr;
            } else {
                // munmap
                kfree((void*)arg1);
                return 0;
            }
            
        case SYS_BRK:
            // Memory allocation - use our kmalloc
            {
                static uint32_t program_break = 0x500000;  // Start at 5MB
                if (arg1 == 0) {
                    return program_break;  // Get current break
                }
                // Set new break (simplified)
                program_break = arg1;
                return program_break;
            }
            
        default:
            // Unsupported syscall - return ENOSYS (errno 38)
            return -38;  // ENOSYS
    }
}

