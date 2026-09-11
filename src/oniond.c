// oniond.c - Onion service daemon for KuzuOS
// Serves as a bridge between Tor network and local HTTP server

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// Syscalls
static inline int syscall0(int n) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n)); return r;
}
static inline int syscall1(int n, int a) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a)); return r;
}
static inline int syscall2(int n, int a, int b) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b)); return r;
}
static inline int syscall3(int n, int a, int b, int c) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c)); return r;
}

#define SYS_EXIT           1
#define SYS_READ           3
#define SYS_WRITE          4
#define SYS_OPEN           5
#define SYS_CLOSE          6
#define SYS_NET_POLL       406
#define SYS_NET_SEND       402
#define SYS_NET_RECV       403
#define SYS_NET_CLOSE      404
#define SYS_NET_LISTEN     413
#define SYS_NET_ACCEPT     414
#define SYS_NET_CONNECT    415

#define O_RDONLY 0

#define MAX_CONNECTIONS 16
#define BUFFER_SIZE 4096
#define ONION_PORT 80
#define LOCAL_HTTP_PORT 80

// Connection types
#define CONN_TYPE_TOR     1
#define CONN_TYPE_LOCAL   2

// Connection state
typedef struct {
    int sock;
    int type;
    int active;
    int paired_conn;  // Index of paired connection (for forwarding)
    uint8_t buffer[BUFFER_SIZE];
    uint16_t buf_len;
    uint16_t buf_pos;
} connection_t;

static connection_t connections[MAX_CONNECTIONS];
static char print_buf[256];
static int onion_service_idx = -1;

// Simple string functions
static int str_len(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void str_copy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = 0;
}

static void print(const char* s) {
    syscall3(SYS_WRITE, 1, (int)s, str_len(s));
}

static void print_hex(uint8_t* data, int len) {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < len; i++) {
        print_buf[0] = hex[(data[i] >> 4) & 0xF];
        print_buf[1] = hex[data[i] & 0xF];
        print_buf[2] = ' ';
        print_buf[3] = 0;
        print(print_buf);
    }
    print("\n");
}

// Forward data from one connection to its paired connection
static int forward_data(int from_conn, int to_conn) {
    if (from_conn < 0 || from_conn >= MAX_CONNECTIONS) return -1;
    if (to_conn < 0 || to_conn >= MAX_CONNECTIONS) return -1;
    
    connection_t* src = &connections[from_conn];
    connection_t* dst = &connections[to_conn];
    
    if (!src->active || !dst->active) return -1;
    if (src->buf_len == 0) return 0;
    
    // Send buffered data
    int to_send = src->buf_len - src->buf_pos;
    if (to_send > 0) {
        int sent = syscall3(SYS_NET_SEND, dst->sock, 
                           (int)(src->buffer + src->buf_pos), to_send);
        if (sent > 0) {
            src->buf_pos += sent;
            if (src->buf_pos >= src->buf_len) {
                src->buf_len = 0;
                src->buf_pos = 0;
            }
            return sent;
        }
    }
    return 0;
}

// Receive data from a connection
static int receive_data(int conn_idx) {
    if (conn_idx < 0 || conn_idx >= MAX_CONNECTIONS) return -1;
    connection_t* conn = &connections[conn_idx];
    if (!conn->active) return -1;
    
    int n = syscall3(SYS_NET_RECV, conn->sock, (int)conn->buffer, BUFFER_SIZE);
    if (n > 0) {
        conn->buf_len = n;
        conn->buf_pos = 0;
        return n;
    }
    return n;
}

// Initialize Tor connection (simplified - just connects to local Tor daemon)
static int init_tor_connection(void) {
    print("[oniond] Initializing Tor connection...\n");
    
    // In a full implementation, this would connect to the Tor network
    // For now, we simulate by just marking the service as active
    
    // Create a listening socket for the onion service
    int listen_sock = syscall1(SYS_NET_LISTEN, ONION_PORT + 1);  // Port 81 for onion
    if (listen_sock < 0) {
        print("[oniond] Failed to create onion listener\n");
        return -1;
    }
    
    print("[oniond] Onion service listening on port ");
    // Simple int to string
    char port_str[8];
    int p = ONION_PORT + 1;
    int i = 0;
    if (p >= 10) {
        port_str[i++] = '0' + (p / 10);
        port_str[i++] = '0' + (p % 10);
    } else {
        port_str[i++] = '0' + p;
    }
    port_str[i] = 0;
    print(port_str);
    print("\n");
    
    return listen_sock;
}

// Connect to local HTTP server
static int connect_local_http(void) {
    // Connect to 127.0.0.1:80
    uint32_t localhost = 127 * 16777216 + 1;  // 127.0.0.1
    int sock = syscall3(SYS_NET_CONNECT, localhost, LOCAL_HTTP_PORT, 50000);
    return sock;
}

// Handle incoming onion connection
static void handle_onion_connection(int onion_sock) {
    print("[oniond] New onion connection!\n");
    
    // Accept the onion connection
    int tor_sock = syscall1(SYS_NET_ACCEPT, onion_sock);
    if (tor_sock < 0) return;
    
    // Find free slot for Tor connection
    int tor_idx = -1;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!connections[i].active) {
            tor_idx = i;
            break;
        }
    }
    if (tor_idx < 0) {
        syscall1(SYS_NET_CLOSE, tor_sock);
        return;
    }
    
    // Connect to local HTTP server
    int http_sock = connect_local_http();
    if (http_sock < 0) {
        syscall1(SYS_NET_CLOSE, tor_sock);
        print("[oniond] Failed to connect to local HTTP\n");
        return;
    }
    
    // Find free slot for HTTP connection
    int http_idx = -1;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!connections[i].active && i != tor_idx) {
            http_idx = i;
            break;
        }
    }
    if (http_idx < 0) {
        syscall1(SYS_NET_CLOSE, tor_sock);
        syscall1(SYS_NET_CLOSE, http_sock);
        return;
    }
    
    // Set up Tor connection
    connections[tor_idx].sock = tor_sock;
    connections[tor_idx].type = CONN_TYPE_TOR;
    connections[tor_idx].active = 1;
    connections[tor_idx].paired_conn = http_idx;
    connections[tor_idx].buf_len = 0;
    connections[tor_idx].buf_pos = 0;
    
    // Set up HTTP connection
    connections[http_idx].sock = http_sock;
    connections[http_idx].type = CONN_TYPE_LOCAL;
    connections[http_idx].active = 1;
    connections[http_idx].paired_conn = tor_idx;
    connections[http_idx].buf_len = 0;
    connections[http_idx].buf_pos = 0;
    
    print("[oniond] Connection paired: Tor[");
    char idx_str[4];
    idx_str[0] = '0' + tor_idx;
    idx_str[1] = ']';
    idx_str[2] = '<';
    idx_str[3] = '>';
    print(idx_str);
    print("HTTP[");
    idx_str[0] = '0' + http_idx;
    print(idx_str);
    print("]\n");
}

// Close a connection and its pair
static void close_connection(int conn_idx) {
    if (conn_idx < 0 || conn_idx >= MAX_CONNECTIONS) return;
    connection_t* conn = &connections[conn_idx];
    if (!conn->active) return;
    
    int pair_idx = conn->paired_conn;
    
    syscall1(SYS_NET_CLOSE, conn->sock);
    conn->active = 0;
    conn->paired_conn = -1;
    
    if (pair_idx >= 0 && pair_idx < MAX_CONNECTIONS) {
        connections[pair_idx].paired_conn = -1;
        connections[pair_idx].active = 0;
        syscall1(SYS_NET_CLOSE, connections[pair_idx].sock);
    }
    
    print("[oniond] Connection closed\n");
}

void _start(void) {
    print("=== KuzuOS Onion Service Daemon ===\n");
    print("[oniond] Starting...\n");
    
    // Initialize connections
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i].sock = -1;
        connections[i].active = 0;
        connections[i].paired_conn = -1;
        connections[i].buf_len = 0;
        connections[i].buf_pos = 0;
    }
    
    // Initialize Tor/onion service
    int onion_listen_sock = init_tor_connection();
    if (onion_listen_sock < 0) {
        print("[oniond] Failed to initialize onion service\n");
        syscall1(SYS_EXIT, 1);
    }
    
    print("[oniond] Onion service ready!\n");
    print("[oniond] Your onion address: kuzuos2test.onion (simulated)\n");
    print("[oniond] Forwarding to local HTTP server on port 80\n");
    
    // Main loop
    while (1) {
        // Poll network
        syscall1(SYS_NET_POLL, 0);
        
        // Check for new onion connections
        int new_sock = syscall1(SYS_NET_ACCEPT, onion_listen_sock);
        if (new_sock >= 0) {
            handle_onion_connection(onion_listen_sock);
        }
        
        // Handle existing connections
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (!connections[i].active) continue;
            
            // Receive data
            int n = receive_data(i);
            if (n < 0) {
                // Connection error or closed
                close_connection(i);
                continue;
            }
            
            if (n > 0) {
                print("[oniond] Received ");
                char len_str[8];
                int li = 0;
                if (n >= 100) { len_str[li++] = '0' + (n / 100); n %= 100; }
                if (n >= 10) { len_str[li++] = '0' + (n / 10); n %= 10; }
                len_str[li++] = '0' + n;
                len_str[li] = 0;
                print(len_str);
                print(" bytes on conn ");
                char idx_str[4];
                idx_str[0] = '0' + i;
                idx_str[1] = 0;
                print(idx_str);
                print("\n");
                
                // Forward to paired connection
                int pair = connections[i].paired_conn;
                if (pair >= 0) {
                    forward_data(i, pair);
                }
            }
            
            // Forward any buffered data
            int pair = connections[i].paired_conn;
            if (pair >= 0 && connections[i].buf_len > 0) {
                forward_data(i, pair);
            }
        }
        
        // Small delay
        for (volatile int d = 0; d < 1000; d++);
    }
    
    syscall1(SYS_EXIT, 0);
}
