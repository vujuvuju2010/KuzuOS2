#include "service.h"
#include "process.h"
#include "filesystem.h"

// Local printf wrapper using kernel print functions
static void svc_printf(const char* fmt) {
    // Use simple print for now
    extern void print(const char* s);
    // Simple string print - fmt is just a string for now
    print(fmt);
}

// Local memset implementation
static void* svc_memset(void* s, int c, unsigned long n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

// Local strncpy implementation
static char* svc_strncpy(char* dest, const char* src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    while (i < n - 1) {
        dest[i++] = '\0';
    }
    dest[n - 1] = '\0';
    return dest;
}

// Local strncat implementation
static char* svc_strncat(char* dest, const char* src, int n) {
    int dest_len = 0;
    while (dest[dest_len]) dest_len++;
    int i = 0;
    while (src[i] && i < n) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
    return dest;
}

// Local strcmp implementation
static int svc_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if ((unsigned char)*s1 != (unsigned char)*s2)
            return (unsigned char)*s1 - (unsigned char)*s2;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

// Local strchr implementation
static char* svc_strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    if (c == '\0') return (char*)s;
    return 0;
}

// Global service manager instance
struct service_manager g_service_manager = {0};

// Service state names for display
static const char* service_state_names[] = {
    "disabled",
    "stopped",
    "starting",
    "running",
    "stopping",
    "failed"
};

const char* service_state_name(int state) {
    if (state < 0 || state > SERVICE_FAILED) {
        return "unknown";
    }
    return service_state_names[state];
}

// Helper: skip whitespace
static char* skip_whitespace(char* s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
        s++;
    }
    return s;
}

// Helper: get string length
static int svc_strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Helper: trim trailing whitespace/newlines
static void trim_trailing(char* s) {
    int len = svc_strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' ||
                       s[len-1] == '\r' || s[len-1] == '\n')) {
        s[--len] = '\0';
    }
}

// Helper: parse a line in format "key=value"
static int parse_key_value(char* line, char* key, int key_max, char* value, int value_max) {
    char* eq = 0;
    int i = 0;
    while (line[i]) {
        if (line[i] == '=') {
            eq = &line[i];
            break;
        }
        i++;
    }
    
    if (!eq) {
        return 0;  // No '=' found
    }
    
    // Extract key
    int key_len = eq - line;
    if (key_len >= key_max) key_len = key_max - 1;
    svc_strncpy(key, line, key_len + 1);
    key[key_len] = '\0';
    
    // Extract value
    char* val_start = skip_whitespace(eq + 1);
    svc_strncpy(value, val_start, value_max);
    value[value_max - 1] = '\0';
    trim_trailing(value);
    
    return 1;
}

// Read a line from file content
static int read_line(char** content, char* line, int max_len) {
    int i = 0;
    while (**content && **content != '\n' && i < max_len - 1) {
        line[i++] = **content;
        (*content)++;
    }
    line[i] = '\0';
    
    // Skip newline
    if (**content == '\n') {
        (*content)++;
    }
    
    return i > 0 || line[0] != '\0';
}

// Load a service configuration from a .conf file
int service_load_config(const char* path) {
    if (!path || g_service_manager.service_count >= MAX_SERVICES) {
        return -1;
    }
    
    struct service* svc = &g_service_manager.services[g_service_manager.service_count];
    svc_memset(svc, 0, sizeof(struct service));
    
    // Default values
    svc->state = SERVICE_STOPPED;
    svc->config.auto_start = 0;
    svc->config.restart_on_fail = 0;
    svc->config.type = SERVICE_TYPE_USER;
    svc_strncpy(svc->config.name, "unknown", SERVICE_NAME_MAX);
    
    // Read the config file content
    char content[4096];
    int file_size = fs_read_file((char*)path, content, sizeof(content));
    if (file_size <= 0) {
        svc_printf("service: failed to open ");
        svc_printf(path);
        svc_printf("\n");
        return -1;
    }
    content[file_size] = '\0';
    
    char* content_ptr = content;
    char line[256];
    char key[64], value[256];
    
    while (read_line(&content_ptr, line, sizeof(line))) {
        line[255] = '\0';  // Ensure null termination
        
        // Skip comments and empty lines
        char* trimmed = skip_whitespace(line);
        if (trimmed[0] == '#' || trimmed[0] == '\0' || trimmed[0] == '\n') {
            continue;
        }
        
        if (parse_key_value(trimmed, key, sizeof(key), value, sizeof(value))) {
            if (svc_strcmp(key, "name") == 0) {
                svc_strncpy(svc->config.name, value, SERVICE_NAME_MAX);
            } else if (svc_strcmp(key, "exec") == 0) {
                svc_strncpy(svc->config.exec_path, value, SERVICE_EXEC_MAX);
            } else if (svc_strcmp(key, "args") == 0) {
                svc_strncpy(svc->config.exec_args, value, SERVICE_ARGS_MAX);
            } else if (svc_strcmp(key, "description") == 0) {
                svc_strncpy(svc->config.description, value, SERVICE_DESC_MAX);
            } else if (svc_strcmp(key, "type") == 0) {
                if (svc_strcmp(value, "system") == 0) {
                    svc->config.type = SERVICE_TYPE_SYSTEM;
                } else {
                    svc->config.type = SERVICE_TYPE_USER;
                }
            } else if (svc_strcmp(key, "auto_start") == 0) {
                svc->config.auto_start = (svc_strcmp(value, "true") == 0 ||
                                         svc_strcmp(value, "1") == 0 ||
                                         svc_strcmp(value, "yes") == 0);
            } else if (svc_strcmp(key, "restart_on_fail") == 0) {
                svc->config.restart_on_fail = (svc_strcmp(value, "true") == 0 ||
                                              svc_strcmp(value, "1") == 0 ||
                                              svc_strcmp(value, "yes") == 0);
            }
        }
    }
    
    // Validate required fields
    if (svc_strcmp(svc->config.name, "unknown") == 0 ||
        svc->config.exec_path[0] == '\0') {
        svc_printf("service: invalid config ");
        svc_printf(path);
        svc_printf("\n");
        return -1;
    }
    
    g_service_manager.service_count++;
    svc_printf("service: loaded '");
    svc_printf(svc->config.name);
    svc_printf("' from ");
    svc_printf(path);
    svc_printf("\n");
    return 0;
}

// Helper: convert int to string
static int svc_itoa(int n, char* buf, int buf_size) {
    if (buf_size <= 0) return 0;
    int i = 0;
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    char tmp[16];
    int tp = 0;
    int neg = 0;
    if (n < 0) {
        neg = 1;
        n = -n;
    }
    while (n > 0 && tp < 15) {
        tmp[tp++] = '0' + (n % 10);
        n /= 10;
    }
    if (neg && i < buf_size - 1) buf[i++] = '-';
    while (tp > 0 && i < buf_size - 1) {
        buf[i++] = tmp[--tp];
    }
    buf[i] = '\0';
    return i;
}

// Load all service configs from /etc/services/ directory
int service_load_all_configs(void) {
    if (!g_service_manager.initialized) {
        return -1;
    }
    
    // Check if /etc/services directory exists using fs_any_is_directory
    if (!fs_any_is_directory("/etc/services")) {
        svc_printf("service: /etc/services directory not found\n");
        return -1;
    }
    
    int loaded = 0;
    int index = 0;
    char entry_name[64];
    int is_dir;
    
    while (fs_readdir_index("/etc/services", index, entry_name, &is_dir) &&
           g_service_manager.service_count < MAX_SERVICES) {
        index++;
        
        // Check if it's a .conf file
        int name_len = svc_strlen(entry_name);
        if (name_len > 5 && svc_strcmp(entry_name + name_len - 5, ".conf") == 0) {
            char path[256];
            svc_strncpy(path, "/etc/services/", sizeof(path));
            svc_strncat(path, entry_name, sizeof(path) - svc_strlen(path) - 1);
            if (service_load_config(path) == 0) {
                loaded++;
            }
        }
    }
    
    svc_printf("service: loaded ");
    char lbuf[16];
    svc_itoa(loaded, lbuf, sizeof(lbuf));
    svc_printf(lbuf);
    svc_printf(" service configurations\n");
    return loaded;
}

// Find a service by name
struct service* service_find(const char* name) {
    if (!name) {
        return 0;
    }
    
    for (int i = 0; i < g_service_manager.service_count; i++) {
        if (svc_strcmp(g_service_manager.services[i].config.name, name) == 0) {
            return &g_service_manager.services[i];
        }
    }
    return 0;
}

// Find a service by PID
struct service* service_find_by_pid(int pid) {
    for (int i = 0; i < g_service_manager.service_count; i++) {
        if (g_service_manager.services[i].config.pid == pid) {
            return &g_service_manager.services[i];
        }
    }
    return 0;
}

// Start a service by name
int service_start(const char* name) {
    if (!name) {
        return -1;
    }
    
    struct service* svc = service_find(name);
    if (!svc) {
        svc_printf("service: '");
        svc_printf(name);
        svc_printf("' not found\n");
        return -1;
    }
    
    if (svc->state == SERVICE_RUNNING) {
        svc_printf("service: '");
        svc_printf(name);
        svc_printf("' is already running\n");
        return 0;
    }
    
    if (svc->config.exec_path[0] == '\0') {
        svc_printf("service: '");
        svc_printf(name);
        svc_printf("' has no executable path\n");
        return -1;
    }
    
    svc->state = SERVICE_STARTING;
    svc_printf("service: starting '");
    svc_printf(svc->config.name);
    svc_printf("' (");
    svc_printf(svc->config.exec_path);
    svc_printf(")\n");
    
    // Store service PID marker (service execution uses execve from shell context)
    // Services are started by calling execve which creates a new process
    // The service manager tracks the service state but doesn't directly manage the process
    
    // For now, mark as running - actual execution happens via execve syscall
    // which is called when user runs the service from shell or via auto-start
    svc->state = SERVICE_RUNNING;
    svc->config.pid = 0;  // Will be set when process is created
    
    svc_printf("service: '");
    svc_printf(name);
    svc_printf("' started\n");
    return 0;
}

// Stop a service by name
int service_stop(const char* name) {
    if (!name) {
        return -1;
    }
    
    struct service* svc = service_find(name);
    if (!svc) {
        svc_printf("service: '");
        svc_printf(name);
        svc_printf("' not found\n");
        return -1;
    }
    
    if (svc->state != SERVICE_RUNNING) {
        svc_printf("service: '");
        svc_printf(name);
        svc_printf("' is not running\n");
        return 0;
    }
    
    svc->state = SERVICE_STOPPING;
    svc_printf("service: stopping '");
    svc_printf(svc->config.name);
    svc_printf("' (PID ");
    char pbuf[16];
    svc_itoa(svc->config.pid, pbuf, sizeof(pbuf));
    svc_printf(pbuf);
    svc_printf(")\n");
    
    // Kill the process
    extern int process_kill(uint32_t pid);
    process_kill(svc->config.pid);
    
    svc->config.pid = 0;
    svc->state = SERVICE_STOPPED;
    svc->stop_time = 0;  // Would use actual time if available
    
    svc_printf("service: '");
    svc_printf(name);
    svc_printf("' stopped\n");
    return 0;
}

// Restart a service by name
int service_restart(const char* name) {
    if (!name) {
        return -1;
    }
    
    struct service* svc = service_find(name);
    if (!svc) {
        svc_printf("service: '");
        svc_printf(name);
        svc_printf("' not found\n");
        return -1;
    }
    
    if (svc->state == SERVICE_RUNNING) {
        service_stop(name);
    }
    
    svc->restart_count++;
    return service_start(name);
}

// Simple string copy for status output
static char* simple_strcpy(char* dest, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

// Get status of a service - simplified
int service_status(const char* name, char* output, int max_len) {
    if (!name || !output || max_len <= 0) {
        return -1;
    }
    
    struct service* svc = service_find(name);
    if (!svc) {
        simple_strcpy(output, "Service '", max_len);
        int len = svc_strlen(output);
        simple_strcpy(output + len, name, max_len - len);
        len = svc_strlen(output);
        simple_strcpy(output + len, "' not found", max_len - len);
        return -1;
    }
    
    int offset = 0;
    simple_strcpy(output + offset, "Service: ", max_len - offset);
    offset = svc_strlen(output);
    simple_strcpy(output + offset, svc->config.name, max_len - offset);
    offset = svc_strlen(output);
    simple_strcpy(output + offset, "\n  State: ", max_len - offset);
    offset = svc_strlen(output);
    simple_strcpy(output + offset, service_state_name(svc->state), max_len - offset);
    offset = svc_strlen(output);
    simple_strcpy(output + offset, "\n  PID: ", max_len - offset);
    offset = svc_strlen(output);
    char pbuf[16];
    svc_itoa(svc->config.pid, pbuf, sizeof(pbuf));
    simple_strcpy(output + offset, pbuf, max_len - offset);
    offset = svc_strlen(output);
    simple_strcpy(output + offset, "\n", max_len - offset);
    
    return 0;
}

// List all services - simplified
int service_list(char* output, int max_len) {
    if (!output || max_len <= 0) {
        return -1;
    }
    
    int offset = 0;
    simple_strcpy(output + offset, "Services (", max_len - offset);
    offset = svc_strlen(output);
    char cbuf[16];
    svc_itoa(g_service_manager.service_count, cbuf, sizeof(cbuf));
    simple_strcpy(output + offset, cbuf, max_len - offset);
    offset = svc_strlen(output);
    simple_strcpy(output + offset, " registered):\n", max_len - offset);
    offset = svc_strlen(output);
    
    for (int i = 0; i < g_service_manager.service_count && offset < max_len - 20; i++) {
        struct service* svc = &g_service_manager.services[i];
        simple_strcpy(output + offset, "  ", max_len - offset);
        offset = svc_strlen(output);
        simple_strcpy(output + offset, svc->config.name, max_len - offset);
        offset = svc_strlen(output);
        simple_strcpy(output + offset, " [", max_len - offset);
        offset = svc_strlen(output);
        simple_strcpy(output + offset, service_state_name(svc->state), max_len - offset);
        offset = svc_strlen(output);
        simple_strcpy(output + offset, "]\n", max_len - offset);
        offset = svc_strlen(output);
    }
    
    return 0;
}

// Run all auto-start services
void service_run_autostart(void) {
    if (!g_service_manager.initialized) {
        return;
    }
    
    svc_printf("service: starting auto-start services...\n");
    
    for (int i = 0; i < g_service_manager.service_count; i++) {
        struct service* svc = &g_service_manager.services[i];
        if (svc->config.auto_start && svc->state == SERVICE_STOPPED) {
            service_start(svc->config.name);
        }
    }
}

// Initialize the service manager
void service_manager_init(void) {
    svc_memset(&g_service_manager, 0, sizeof(struct service_manager));
    g_service_manager.initialized = 1;
    g_service_manager.service_count = 0;
    svc_printf("service: service manager initialized\n");
}
