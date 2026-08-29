// syscall_router.c - Route and handle all Linux-compatible syscalls for KuzuOS5
// This is the main syscall dispatcher that routes to appropriate handlers

#include "syscall.h"
#include "syscall_linux_compat.h"
#include "vga.h"
#include "filesystem.h"
#include "memory.h"
#include "process.h"
#include "service.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Standard type definitions
typedef unsigned int size_t;

// Undefined in syscall.h - define locally
#ifndef SYS_OPENDIR
#define SYS_OPENDIR  0xFFF1  // Not a real Linux syscall, placeholder
#endif
#ifndef SYS_READDIR
// SYS_READDIR is 89 in syscall.h, already defined
#endif
#ifndef SYS_CLOSEDIR
#define SYS_CLOSEDIR 0xFFF2  // Not a real Linux syscall, placeholder
#endif
#ifndef SYS_ARCH_PRCTL
#define SYS_ARCH_PRCTL 384
#endif
typedef int off_t;
typedef unsigned int mode_t;
typedef long time_t;

// Helper to print syscall names for debugging
static const char* syscall_name(uint64_t num) {
    switch (num) {
        case SYS_READ: return "read";
        case SYS_WRITE: return "write";
        case SYS_OPEN: return "open";
        case SYS_CLOSE: return "close";
        case SYS_LSEEK: return "lseek";
        case SYS_EXIT: return "exit";
        case SYS_EXIT_GROUP: return "exit_group";
        case SYS_FORK: return "fork";
        case SYS_EXECVE: return "execve";
        case SYS_GETPID: return "getpid";
        case SYS_MKDIR: return "mkdir";
        case SYS_STAT: return "stat";
        case SYS_UNAME: return "uname";
        case SYS_GETUID: return "getuid";
        case SYS_TIME: return "time";
        case SYS_IOCTL_SCREEN: return "ioctl_screen";
        default: return "unknown";
    }
}

// ================== Filesystem Wrapper Functions ==================
// These wrap existing filesystem functions with syscall-compatible signatures

static int fs_delete_file_wrapper(char* path) {
    return fs_delete_file(path, 0);  // non-recursive delete
}

static int fs_stat_wrapper(char* path, void* stat_buf) {
    // Just check existence
    return fs_any_exists(path) ? 0 : -1;
}

static int fs_fstat_wrapper(int fd, void* stat_buf) {
    // TODO: lookup fd and stat it
    // For now, just return 0
    return 0;
}

static int fs_chdir_wrapper(char* path) {
    // Check if path is a directory
    if (fs_any_is_directory(path)) {
        // TODO: track current directory
        return 0;
    }
    return -1;
}

static int fs_getcwd_wrapper(char* buf, size_t size) {
    // TODO: return actual current working directory
    // For now, return "/"
    if (size < 2) return -1;
    buf[0] = '/';
    buf[1] = 0;
    return 0;
}

static int fs_mkdir_wrapper(char* path) {
    return fs_create_directory(path);
}

static int fs_rmdir_wrapper(char* path) {
    // i just wanna be part of your SYMPHHOOONYYYYY 
    return fs_delete_file(path, 0);
}

static int fs_truncate_wrapper(char* path, off_t length) {
    // TODO: truncate file at path to length bytes
    return -1;
}

static int fs_ftruncate_wrapper(int fd, off_t length) {
    // TODO: truncate fd to length bytes
    return -1;
}

static int fs_rename_wrapper(char* oldpath, char* newpath) {
    // TODO: implement rename - requires deleting oldpath after copying
    return -1;
}

// ================== Process Management Wrappers ==================

static int process_fork_wrapper(void) {
    // TODO: implement fork - clone current process
    return -1;
}

static int get_current_pid_wrapper(void) {
    // TODO: return actual current PID
    return 1;
}

static int get_parent_pid_wrapper(void) {
    // TODO: return actual parent PID
    return 0;
}

// ================== Memory Management Wrappers ==================

static int memory_brk_wrapper(void* addr) {
    // TODO: extend/shrink heap to addr
    return 0;
}

static void* memory_mmap_wrapper(void* addr, size_t length, int prot, int flags, int fd, int pgoffset) {
    // TODO: map memory region
    return (void*)-1;
}

static void* memory_mremap_wrapper(void* old_addr, size_t old_size, size_t new_size, int flags) {
    // TODO: resize existing mapping
    return (void*)-1;
}

static int memory_munmap_wrapper(void* addr, size_t length) {
    // TODO: unmap memory region
    return -1;
}

// ================== File Descriptor Wrappers ==================

static int fd_dup_wrapper(int oldfd) {
    // TODO: duplicate fd
    return -1;
}

static int fd_dup2_wrapper(int oldfd, int newfd) {
    // TODO: duplicate oldfd onto newfd
    return -1;
}

static int fd_fcntl_wrapper(int fd, int cmd, int arg) {
    // TODO: file control operations
    return -1;
}

static int fd_pipe_wrapper(int* pipefd) {
    // TODO: create pipe
    return -1;
}

static int fd_ioctl_wrapper(int fd, unsigned long request, void* arg) {
    // TODO: I/O control for fd
    return -1;
}

// ================== Time Wrappers ==================

static time_t kernel_time_wrapper(time_t* tloc) {
    // TODO: return system time
    return 0;
}

static int kernel_gettimeofday_wrapper(void* tv, void* tz) {
    // TODO: return current time of day
    return 0;
}

static int kernel_clock_gettime_wrapper(int clockid, void* tp) {
    // TODO: return clock time
    return 0;
}

static int kernel_nanosleep_wrapper(void* req, void* rem) {
    // TODO: sleep for nanoseconds
    return 0;
}

// Extended syscall handler - implements ALL the syscalls
// Returns: result or -errno on error
int32_t handle_syscall_extended(uint64_t syscall_num,
                                 uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                 uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    extern void z_printf(const char* fmt, ...);
    
    // Debug: log every syscall
    if (syscall_num == 45) {
        z_printf("[SYSCALL_ROUTER] Got syscall %d (BRK) with arg1=0x%x\n", syscall_num, arg1);
    }
    
    // Delegate to original handler for implemented syscalls
    // Then add new ones here
    
    switch (syscall_num) {
        // ==================== Process Exit ====================
        case SYS_EXIT: {
            // void exit(int status)
            extern struct process* current_process;
            if (current_process) {
                extern void process_exit_current(int exit_code);
                process_exit_current((int)arg1);
            }
            return 0;
        }
        
        case SYS_EXIT_GROUP: {
            // void exit_group(int status) - exit all threads in process
            extern struct process* current_process;
            if (current_process) {
                extern void process_exit_current(int exit_code);
                process_exit_current((int)arg1);
            }
            return 0;
        }
        
        // ==================== File Descriptors ====================
        case SYS_DUP: {
            // int dup(int oldfd) - not yet implemented
            return -1;
        }
        
        case SYS_DUP2: {
            // int dup2(int oldfd, int newfd) - not yet implemented
            return -1;
        }
        
        case SYS_FCNTL: {
            // int fcntl(int fd, int cmd, ...) - not yet implemented
            return -1;
        }
        
        case SYS_PIPE: {
            // int pipe(int pipefd[2]) - not yet implemented
            return -1;
        }
        
        case SYS_IOCTL: {
            // int ioctl(int fd, unsigned long request, ...) - not yet implemented
            return -1;
        }
        
        // ==================== File Operations ====================
        case SYS_UNLINK: {
            // int unlink(const char *pathname)
            const char* path = (const char*)arg1;
            return fs_delete_file_wrapper((char*)path);
        }
        
        case SYS_RENAME: {
            // int rename(const char *oldpath, const char *newpath) - not yet implemented
            return -1;
        }
        
        case SYS_LINK: {
            // int link(const char *oldpath, const char *newpath)
            const char* oldpath = (const char*)arg1;
            const char* newpath = (const char*)arg2;
            // TODO: need filesystem support for link (hardlink)
            return -1;
        }
        
        case SYS_SYMLINK: {
            // int symlink(const char *target, const char *linkpath)
            // TODO: need filesystem support for symlink
            return -1;
        }
        
        case SYS_READLINK: {
            // ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
            // TODO: need filesystem support for symlink read
            return -1;
        }
        
        case SYS_CHMOD: {
            // int chmod(const char *pathname, mode_t mode)
            // KuzuOS doesn't have permissions yet - just succeed
            return 0;
        }
        
        case SYS_FCHMOD: {
            // int fchmod(int fd, mode_t mode)
            // KuzuOS doesn't have permissions yet - just succeed
            return 0;
        }
        
        case SYS_CHOWN: {
            // int chown(const char *pathname, uid_t owner, gid_t group)
            // KuzuOS doesn't have ownership yet - just succeed
            return 0;
        }
        
        case SYS_FCHOWNAT: {
            // int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags)
            // KuzuOS doesn't have ownership yet - just succeed
            return 0;
        }
        
        case SYS_TRUNCATE: {
            // int truncate(const char *path, off_t length) - not yet implemented
            return -1;
        }
        
        case SYS_FTRUNCATE: {
            // int ftruncate(int fd, off_t length) - not yet implemented
            return -1;
        }
        
        // ==================== Directory Operations ====================
        case SYS_MKDIR: {
            // int mkdir(const char *pathname, mode_t mode)
            const char* pathname = (const char*)arg1;
            
            // Validate user pointer
            if (!pathname || (uint64_t)pathname < 0x400000) {
                return -1;
            }
            
            extern int vfs_mkdir(const char* path);
            return vfs_mkdir(pathname);
        }
        
        case SYS_RMDIR: {
            // int rmdir(const char *pathname) - not yet implemented
            return -1;
        }
        
        case SYS_CHDIR: {
            // int chdir(const char *path)
            const char* path = (const char*)arg1;
            extern int fs_chdir(char* path);
            return fs_chdir((char*)path);
        }
        
        case SYS_GETCWD: {
            // char *getcwd(char *buf, size_t size)
            char* buf = (char*)arg1;
            size_t size = (size_t)arg2;
            extern char kernel_cwd[256];
            uint32_t i = 0;
            while (i < size - 1 && kernel_cwd[i]) { buf[i] = kernel_cwd[i]; i++; }
            buf[i] = '\0';
            return (int32_t)i;
        }
        
        case SYS_FCHDIR: {
            // int fchdir(int fd)
            int fd = (int)arg1;
            // TODO: need filesystem fd tracking
            return -1;
        }
        
        case SYS_OPENDIR: {
            // DIR *opendir(const char *name)
            // TODO: need directory struct support
            return -1;
        }
        
        case SYS_READDIR: {
            // struct dirent *readdir(DIR *dirp)
            // TODO: need directory struct support
            return -1;
        }
        
        case SYS_CLOSEDIR: {
            // int closedir(DIR *dirp)
            // TODO: need directory struct support
            return -1;
        }
        
        // ==================== File Information ====================
        case SYS_STAT: {
            // int stat(const char *pathname, struct stat *statbuf)
            // Just check existence for now
            const char* path = (const char*)arg1;
            return fs_any_exists((char*)path) ? 0 : -1;
        }
        
        case SYS_FSTAT: {
            // int fstat(int fd, struct stat *statbuf) - return success stub
            return 0;
        }
        
        case SYS_LSTAT: {
            // int lstat - same as stat (no symlinks)
            const char* path = (const char*)arg1;
            return fs_any_exists((char*)path) ? 0 : -1;
        }
        
        // ==================== Process Management ====================
        case SYS_FORK: {
            // pid_t fork(void) - not yet implemented
            return -1;
        }
        
        case SYS_EXECVE: {
            // int execve(const char *pathname, char *const argv[], char *const envp[])
            const char* pathname = (const char*)arg1;
            char** argv = (char**)arg2;
            char** envp = (char**)arg3;
            extern int elf_load_and_execve(const char* filename, char* const argv[], char* const envp[]);
            return elf_load_and_execve(pathname, argv, envp);
        }
        
        case SYS_CLONE: {
            // int clone(int (*fn)(void *), void *child_stack, int flags, void *arg)
            int (*fn)(void *) = (int (*)(void *))arg1;
            void *child_stack = (void*)arg2;
            int flags = (int)arg3;
            void *arg = (void*)arg4;
            // TODO: implement clone properly
            return -1;
        }
        
        case SYS_GETPID: {
            // pid_t getpid(void)
            extern struct process* current_process;
            return current_process ? (int32_t)current_process->pid : 1;
        }
        
        case SYS_GETPPID: {
            // pid_t getppid(void) - shell is always parent
            return 1;
        }
        
        case SYS_WAIT4: {
            // pid_t wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage)
            int pid = (int)arg1;
            int* wstatus = (int*)arg2;
            int options = (int)arg3;
            void* rusage = (void*)arg4;
            // TODO: implement wait4
            return -1;
        }
        
        case SYS_KILL: {
            // int kill(pid_t pid, int sig)
            int pid = (int)arg1;
            int sig = (int)arg2;
            // TODO: implement signal handling
            return -1;
        }
        
        // ==================== Memory Management ====================
        case SYS_BRK: {
            // int brk(void *addr) - set program break (heap end)
            extern struct process* current_process;
            extern void z_printf(const char* fmt, ...);
            
            if (!current_process) {
                z_printf("[BRK] ERROR: current_process is NULL!\n");
                return -1;
            }
            
            uint32_t new_brk = (uint32_t)arg1;
            
            z_printf("[BRK] Called with arg1=0x%x, heap_start=0x%x, heap_end=0x%x\n",
                     new_brk, current_process->heap_start, current_process->heap_end);
            
            // If heap not initialized yet, set up a dedicated user heap region
            if (current_process->heap_start == 0) {
                z_printf("[BRK] First call, allocating user heap region...\n");
                
                // Try progressively smaller sizes until one succeeds
                uint32_t heap_mem_addr = 0;
                uint32_t heap_size = 0;
                uint32_t try_sizes[] = {
                    64 * 1024 * 1024,   // 64MB
                    32 * 1024 * 1024,   // 32MB
                    16 * 1024 * 1024,   // 16MB
                    8  * 1024 * 1024,   // 8MB
                    4  * 1024 * 1024,   // 4MB
                };
                for (int ti = 0; ti < 5; ti++) {
                    void* m = kmalloc(try_sizes[ti]);
                    if (m) {
                        heap_mem_addr = (uint32_t)m;
                        heap_size = try_sizes[ti];
                        break;
                    }
                }
                void* heap_mem = (void*)heap_mem_addr;
                
                if (!heap_mem) {
                    z_printf("[BRK] ERROR: kmalloc failed for user heap!\n");
                    return -1;
                }

                // delete these 2 if something is broken 
                uint8_t* p = (uint8_t*)heap_mem;
                for (uint32_t zi = 0; zi < heap_size; zi++) p[zi] = 0;

                z_printf("[BRK] kmalloc returned address: 0x%x\n", heap_mem_addr);
                z_printf("[BRK] address low 4 bits: %d\n", heap_mem_addr & 0xF);
                
                uint32_t heap_base = (uint32_t)heap_mem;
                
                current_process->heap_start = heap_base;
                current_process->heap_end = heap_base;  // Start at base, will grow
                current_process->heap_max = heap_base + heap_size;
                
                z_printf("[BRK] User heap: 0x%x - 0x%x (%d MB)\n", 
                         heap_base, heap_base + heap_size, heap_size / (1024*1024));
                
                // If querying (addr=0), return current end (which is start initially)
                if (new_brk == 0) {
                    z_printf("[BRK] Returning initial heap_end: 0x%x\n", heap_base);
                    return (int32_t)heap_base;
                }
                
                // If setting brk on first call, allow it if within bounds
                if (new_brk >= heap_base && new_brk <= current_process->heap_max) {
                    current_process->heap_end = new_brk;
                    z_printf("[BRK] First allocation: set heap_end to 0x%x\n", new_brk);
                    return (int32_t)new_brk;
                }
                
                z_printf("[BRK] ERROR: First allocation out of bounds: 0x%x\n", new_brk);
                return (int32_t)heap_base;
            }
            
            // If addr is 0, return current brk
            if (new_brk == 0) {
                z_printf("[BRK] Query: returning current heap_end: 0x%x\n", current_process->heap_end);
                return (int32_t)current_process->heap_end;
            }
            
            // Check if new_brk is valid
            if (new_brk < current_process->heap_start) {
                z_printf("[BRK] ERROR: new_brk 0x%x < heap_start 0x%x\n", new_brk, current_process->heap_start);
                return (int32_t)current_process->heap_end;  // brk shat itself ::(
            }
            
            if (new_brk > current_process->heap_max) {
                z_printf("[BRK] ERROR: new_brk 0x%x > heap_max 0x%x (requested %d MB)\n", 
                         new_brk, current_process->heap_max, 
                         (new_brk - current_process->heap_start) / (1024*1024));
                return (int32_t)current_process->heap_end;  // Can't grow beyond max
            }
            
            // Just update the pointer within our pre-allocated space
            current_process->heap_end = new_brk;
            z_printf("[BRK] SUCCESS: Set heap_end to 0x%x\n", new_brk);
            return (int32_t)new_brk;
        }
        
        case SYS_MMAP2: {
            // void *mmap2(...) - allocate from kernel heap for anonymous maps
            size_t length = (size_t)arg2;
            int flags = (int)arg4;
            if (flags & 0x20) {  // MAP_ANONYMOUS
                void* p = kmalloc(length);
                // also delete the if and for if soöething is broke
                if (p) {
                    uint8_t* b = (uint8_t*)p;
                    for (size_t zi = 0; zi < length; zi++) b[zi] = 0;
                }
                return p ? (int32_t)p : -1;
            }
            return -1;
        }
        
        case SYS_MREMAP: {
            // void *mremap(...) - not yet implemented
            return -1;
        }
        
        case SYS_MUNMAP: {
            // int munmap(void *addr, size_t length) - stub success
            return 0;
        }
        
        // ==================== User/Group ====================
        case SYS_GETEUID: {
            // uid_t geteuid(void)
            return 0;  // root (euid = 0)
        }
        
        case SYS_GETEGID: {
            // gid_t getegid(void)
            return 0;  // root group (egid = 0)
        }
        
        // ==================== Time ====================
        case SYS_TIME: {
            // time_t time(time_t *tloc) - return 0 (no RTC yet)
            time_t* tloc = (time_t*)arg1;
            if (tloc) *tloc = 0;
            return 0;
        }
        
        case SYS_GETTIMEOFDAY: {
            // int gettimeofday(struct timeval *tv, struct timezone *tz) - stub
            return 0;
        }
        
        case SYS_CLOCK_GETTIME: {
            // int clock_gettime(clockid_t clockid, struct timespec *tp) - stub
            return 0;
        }
        
        case SYS_NANOSLEEP: {
            // int nanosleep(...) - stub (no sleep, just return)
            return 0;
        }
        
        // ==================== Signal Handling ====================
        case SYS_SIGNAL: {
            // sighandler_t signal(int signum, sighandler_t handler)
            int signum = (int)arg1;
            void* handler = (void*)arg2;
            // TODO: implement full signal handling
            return 0;
        }
        
        case SYS_SIGACTION: {
            // int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
            int signum = (int)arg1;
            void* act = (void*)arg2;
            void* oldact = (void*)arg3;
            // TODO: implement full signal handling
            return 0;
        }
        
        // ==================== Misc ==============================
        case SYS_UNAME: {
            // int uname(struct utsname *buf)
            // TODO: implement uname - fill in system info
            return 0;
        }
        
        case SYS_GETRLIMIT: {
            // int getrlimit(int resource, struct rlimit *rlim)
            // TODO: implement getrlimit
            return 0;
        }
        
        case SYS_SETRLIMIT: {
            // int setrlimit(int resource, const struct rlimit *rlim)
            // TODO: implement setrlimit
            return 0;
        }
        
        case SYS_PRCTL: {
            // int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5)
            // TODO: implement prctl
            return 0;
        }
        
        case SYS_ARCH_PRCTL: {
            // int arch_prctl(int code, unsigned long addr)
            // TODO: implement arch_prctl
            return 0;
        }
        
        // ==================== Network ====================
        case 300: {  // SYS_NET_GETINFO
            uint8_t* buf = (uint8_t*)arg1;
            if (!buf) return -1;
            extern void net_get_info(uint8_t* mac6, uint32_t* ip, uint32_t* gw, uint32_t* nm);
            uint32_t* w = (uint32_t*)(buf + 8);
            net_get_info(buf, &w[0], &w[1], &w[2]);
            return 0;
        }
        case 301: {  // SYS_NET_TCP_CONNECT
            uint32_t dst_ip   = arg1;
            uint16_t dst_port = (uint16_t)arg2;
            uint16_t src_port = (uint16_t)arg3;
            extern int tcp_connect(uint32_t, uint16_t, uint16_t);
            return tcp_connect(dst_ip, dst_port, src_port);
        }
        case 302: {  // SYS_NET_TCP_SEND
            int sock      = (int)arg1;
            uint8_t* data = (uint8_t*)arg2;
            uint16_t len  = (uint16_t)arg3;
            extern int tcp_send(int, uint8_t*, uint16_t);
            return tcp_send(sock, data, len);
        }
        case 303: {  // SYS_NET_TCP_RECV
            int sock     = (int)arg1;
            uint8_t* buf = (uint8_t*)arg2;
            uint16_t len = (uint16_t)arg3;
            extern int tcp_recv(int, uint8_t*, uint16_t);
            return tcp_recv(sock, buf, len);
        }
        case 304: {  // SYS_NET_TCP_CLOSE
            extern void tcp_close(int);
            tcp_close((int)arg1);
            return 0;
        }
        case 305: {  // SYS_NET_TCP_CONNECTED
            extern int tcp_is_connected(int);
            return tcp_is_connected((int)arg1);
        }
        case 306: {  // SYS_NET_POLL
            extern void net_poll(void);
            net_poll();
            return 0;
        }
        case 307: {  // SYS_NET_ARP_REQUEST
            extern void arp_request(uint32_t);
            arp_request(arg1);
            return 0;
        }

        // ==================== Process Control ====================
        case 1000: {  // SYS_GETPROCESSLIST
            // Get list of all processes
            struct process_info {
                uint32_t pid;
                uint32_t state;
                char name[32];
            };
            
            struct process_info* info_list = (struct process_info*)arg1;
            int max_count = (int)arg2;
            
            if (!info_list || max_count <= 0) {
                return -1;
            }
            
            // Get process pointers
            struct process* temp_list[256];
            extern int process_get_list(struct process** out_list, int max_count);
            int actual_count = process_get_list(temp_list, max_count < 256 ? max_count : 256);
            
            // Copy to userspace-safe struct
            for (int i = 0; i < actual_count; i++) {
                info_list[i].pid = temp_list[i]->pid;
                info_list[i].state = temp_list[i]->state;
                int j = 0;
                while (j < 31 && temp_list[i]->name[j]) {
                    info_list[i].name[j] = temp_list[i]->name[j];
                    j++;
                }
                info_list[i].name[j] = '\0';
            }
            
            return actual_count;
        }
        
        case 1001: {  // SYS_KILLPROCESS
            // Kill process by PID
            uint32_t pid = (uint32_t)arg1;
            extern int process_kill(uint32_t pid);
            return process_kill(pid);
        }
        
        // ==================== Service Management ====================
        case 1010: {  // SYS_SERVICE_START
            // Start a service by name
            const char* name = (const char*)arg1;
            extern int service_start(const char* name);
            return service_start(name);
        }
        
        case 1011: {  // SYS_SERVICE_STOP
            // Stop a service by name
            const char* name = (const char*)arg1;
            extern int service_stop(const char* name);
            return service_stop(name);
        }
        
        case 1012: {  // SYS_SERVICE_RESTART
            // Restart a service by name
            const char* name = (const char*)arg1;
            extern int service_restart(const char* name);
            return service_restart(name);
        }
        
        case 1013: {  // SYS_SERVICE_STATUS
            // Get status of a service
            const char* name = (const char*)arg1;
            char* buffer = (char*)arg2;
            int max_len = (int)arg3;
            extern int service_status(const char* name, char* output, int max_len);
            return service_status(name, buffer, max_len);
        }
        
        case 1014: {  // SYS_SERVICE_LIST
            // List all services
            char* buffer = (char*)arg1;
            int max_len = (int)arg2;
            extern int service_list(char* output, int max_len);
            return service_list(buffer, max_len);
        }
        

        default:
            // Unknown syscall - try original handler
            return handle_syscall(syscall_num, arg1, arg2, arg3, arg4, arg5, arg6);
    }
}
