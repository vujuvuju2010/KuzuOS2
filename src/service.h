#ifndef SERVICE_H
#define SERVICE_H

#include "kuzulib/fs/vfs.h"

// Basic type definitions (in case not already defined)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define MAX_SERVICES 32
#define SERVICE_NAME_MAX 64
#define SERVICE_PATH_MAX 128
#define SERVICE_EXEC_MAX 128
#define SERVICE_DESC_MAX 128
#define SERVICE_ARGS_MAX 256

// Service states
#define SERVICE_DISABLED    0
#define SERVICE_STOPPED     1
#define SERVICE_STARTING    2
#define SERVICE_RUNNING     3
#define SERVICE_STOPPING    4
#define SERVICE_FAILED      5

// Service types
#define SERVICE_TYPE_USER   0
#define SERVICE_TYPE_SYSTEM 1

// Service configuration structure (parsed from .conf file)
struct service_config {
    char name[SERVICE_NAME_MAX];           // Service name (e.g., "httpd")
    char exec_path[SERVICE_EXEC_MAX];      // Full path to executable (e.g., "/bin/httpd")
    char exec_args[SERVICE_ARGS_MAX];      // Command line arguments
    char description[SERVICE_DESC_MAX];    // Human-readable description
    int type;                              // SERVICE_TYPE_USER or SERVICE_TYPE_SYSTEM
    int auto_start;                        // 1 = start at boot, 0 = manual
    int restart_on_fail;                   // 1 = auto-restart on failure
    int pid;                               // PID of running service (0 if not running)
    
    // CPU affinity configuration for multi-core systems
    int high_priority;                     // 1 = high priority process (gets dedicated core)
    int cpu_core;                          // Specific CPU core ID (-1 = any, 0-N = specific core)
};

// Service log buffer (circular buffer for last messages)
#define SERVICE_LOG_LINES 8
#define SERVICE_LOG_LINE_LEN 128

struct service_log {
    char lines[SERVICE_LOG_LINES][SERVICE_LOG_LINE_LEN];
    int head;                              // Index of newest entry
    int count;                             // Number of entries (0 to SERVICE_LOG_LINES)
};

// Service runtime structure
struct service {
    struct service_config config;          // Configuration from .conf file
    int state;                             // Current state (SERVICE_*)
    uint64_t start_time;                   // When service was started
    uint64_t stop_time;                    // When service was stopped
    int exit_code;                         // Exit code if stopped/failed
    int restart_count;                     // Number of restarts
    struct service_log log;                // Log buffer for service output
    int log_capture_enabled;               // 1 if we should capture output from this service
};

// Service manager structure
struct service_manager {
    struct service services[MAX_SERVICES]; // Array of services
    int service_count;                     // Number of registered services
    int initialized;                       // 1 if manager is initialized
};

// Global service manager
extern struct service_manager g_service_manager;

// Service manager functions
void service_manager_init(void);
int service_load_config(const char* path);
int service_load_all_configs(void);
int service_start(const char* name);
int service_stop(const char* name);
int service_restart(const char* name);
int service_status(const char* name, char* output, int max_len);
int service_list(char* output, int max_len);
struct service* service_find(const char* name);
struct service* service_find_by_pid(int pid);
void service_run_autostart(void);

// Service state helpers
const char* service_state_name(int state);

#endif // SERVICE_H
