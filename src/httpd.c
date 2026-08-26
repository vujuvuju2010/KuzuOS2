// httpd - HTTP web server for KuzuOS
// Supports: file serving, keep-alive, multiple connections

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
#define SYS_LSEEK          19
#define SYS_FSTAT          108
#define SYS_NET_POLL       406
#define SYS_NET_SEND       402
#define SYS_NET_RECV       403
#define SYS_NET_CLOSE      404
#define SYS_NET_LISTEN     413
#define SYS_NET_ACCEPT     414

#define O_RDONLY 0

#define MAX_CONNECTIONS 4096
#define BUFFER_SIZE 4096
#define WWW_ROOT "/www"

// Connection state
typedef struct {
    int sock;
    int active;
    int keep_alive;
} connection_t;

static connection_t connections[MAX_CONNECTIONS];
static char buffer[BUFFER_SIZE];

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

static int str_cmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

static int str_starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

static void print(const char* s) {
    syscall3(SYS_WRITE, 1, (int)s, str_len(s));
}

// Get MIME type from file extension
static const char* get_mime_type(const char* path) {
    const char* ext = path;
    const char* p = path;
    
    // Find last dot
    while (*p) {
        if (*p == '.') ext = p;
        p++;
    }
    
    if (str_cmp(ext, ".html") == 0 || str_cmp(ext, ".htm") == 0)
        return "text/html";
    if (str_cmp(ext, ".css") == 0)
        return "text/css";
    if (str_cmp(ext, ".js") == 0)
        return "application/javascript";
    if (str_cmp(ext, ".json") == 0)
        return "application/json";
    if (str_cmp(ext, ".png") == 0)
        return "image/png";
    if (str_cmp(ext, ".jpg") == 0 || str_cmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (str_cmp(ext, ".gif") == 0)
        return "image/gif";
    if (str_cmp(ext, ".txt") == 0)
        return "text/plain";
    
    return "application/octet-stream";
}

// Simple integer to string
static void int_to_str(char* buf, int n) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    
    char tmp[12];
    int i = 0;
    while (n > 0) {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

// Send HTTP response header
static void send_header(int sock, int status_code, const char* status_msg,
                       const char* content_type, int content_length, int keep_alive) {
    char resp[512];
    char len_str[16];
    int pos = 0;
    
    // Status line
    const char* status_line = "HTTP/1.1 ";
    for (int i = 0; status_line[i]; i++) resp[pos++] = status_line[i];
    
    int_to_str(len_str, status_code);
    for (int i = 0; len_str[i]; i++) resp[pos++] = len_str[i];
    resp[pos++] = ' ';
    
    for (int i = 0; status_msg[i]; i++) resp[pos++] = status_msg[i];
    resp[pos++] = '\r'; resp[pos++] = '\n';
    
    // Content-Type
    const char* ct = "Content-Type: ";
    for (int i = 0; ct[i]; i++) resp[pos++] = ct[i];
    for (int i = 0; content_type[i]; i++) resp[pos++] = content_type[i];
    resp[pos++] = '\r'; resp[pos++] = '\n';
    
    // Content-Length
    const char* cl = "Content-Length: ";
    for (int i = 0; cl[i]; i++) resp[pos++] = cl[i];
    int_to_str(len_str, content_length);
    for (int i = 0; len_str[i]; i++) resp[pos++] = len_str[i];
    resp[pos++] = '\r'; resp[pos++] = '\n';
    
    // Connection header
    if (keep_alive) {
        const char* ka = "Connection: keep-alive\r\n";
        for (int i = 0; ka[i]; i++) resp[pos++] = ka[i];
    } else {
        const char* close = "Connection: close\r\n";
        for (int i = 0; close[i]; i++) resp[pos++] = close[i];
    }
    
    // Server header
    const char* srv = "Server: KuzuOS/2.0\r\n";
    for (int i = 0; srv[i]; i++) resp[pos++] = srv[i];
    
    // End of headers
    resp[pos++] = '\r'; resp[pos++] = '\n';
    
    syscall3(SYS_NET_SEND, sock, (int)resp, pos);
    
    // Poll to ensure it gets sent
    for (int i = 0; i < 1000; i++) {
        syscall1(SYS_NET_POLL, 0);
    }
}

// Send 404 response
static void send_404(int sock, int keep_alive) {
    const char* body = "<html><body><h1>404 Not Found</h1></body></html>";
    int len = str_len(body);
    
    send_header(sock, 404, "Not Found", "text/html", len, keep_alive);
    syscall3(SYS_NET_SEND, sock, (int)body, len);
    
    // Poll to ensure it gets sent
    for (int i = 0; i < 10000; i++) { // delete 1 zero if necesarry
        syscall1(SYS_NET_POLL, 0);
    }
}

// Send 500 response
static void send_500(int sock, int keep_alive) {
    const char* body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    int len = str_len(body);
    
    send_header(sock, 500, "Internal Server Error", "text/html", len, keep_alive);
    syscall3(SYS_NET_SEND, sock, (int)body, len);
}

// Serve a file
static void serve_file(int sock, const char* path, int keep_alive) {
    char full_path[256];
    str_copy(full_path, WWW_ROOT);
    
    // Prevent directory traversal
    if (path[0] != '/' || str_starts_with(path, "/../") || str_starts_with(path, "/..")) {
        send_404(sock, keep_alive);
        return;
    }
    
    // Build full path
    int i = str_len(full_path);
    for (int j = 0; path[j] && i < 255; j++, i++) {
        full_path[i] = path[j];
    }
    full_path[i] = 0;
    
    // If path ends with /, serve index.html
    if (full_path[i-1] == '/') {
        const char* index = "index.html";
        for (int j = 0; index[j] && i < 255; j++, i++) {
            full_path[i] = index[j];
        }
        full_path[i] = 0;
    }
    
    // Open file
    int fd = syscall3(SYS_OPEN, (int)full_path, O_RDONLY, 0);
    if (fd < 0) {
        print("Failed to open file: ");
        print(full_path);
        print("\n");
        send_404(sock, keep_alive);
        return;
    }
    
    print("Opened file: ");
    print(full_path);
    print("\n");
    
    // Read file into buffer
    int file_size = 0;
    int n;
    char file_buf[4096];
    while ((n = syscall3(SYS_READ, fd, (int)file_buf + file_size, 4096 - file_size)) > 0) {
        file_size += n;
        if (file_size >= 4096) break;
    }
    syscall1(SYS_CLOSE, fd);
    
    if (file_size < 0) {
        send_500(sock, keep_alive);
        return;
    }
    
    // Send response
    const char* mime = get_mime_type(full_path);
    send_header(sock, 200, "OK", mime, file_size, keep_alive);
    syscall3(SYS_NET_SEND, sock, (int)file_buf, file_size);
    
    // Poll longer to ensure it gets sent and received
    for (int i = 0; i < 5000; i++) {
        syscall1(SYS_NET_POLL, 0);
    }
}

// Parse and handle HTTP request
static void handle_request(int sock, char* req, int req_len, int* keep_alive) {
    print("Parsing request, length: ");
    char len_buf[16];
    int_to_str(len_buf, req_len);
    print(len_buf);
    print("\n");
    
    if (req_len < 14) {
        print("Request too short\n");
        send_404(sock, 0);
        *keep_alive = 0;
        return;
    }
    
    // Determine method
    int is_get = str_starts_with(req, "GET ");
    int is_head = str_starts_with(req, "HEAD ");
    int is_post = str_starts_with(req, "POST ");
    
    if (!is_get && !is_head && !is_post) {
        print("Unknown method\n");
        send_404(sock, 0);
        *keep_alive = 0;
        return;
    }
    
    // Extract path
    char path[256];
    int i = is_head ? 5 : (is_get ? 4 : 5); // skip "HEAD " or "GET " or "POST "
    int j = 0;
    while (i < req_len && req[i] != ' ' && req[i] != '\r' && req[i] != '\n' && j < 255) {
        path[j++] = req[i++];
    }
    path[j] = 0;
    
    // Check for Connection: keep-alive (disabled for now - always close)
    *keep_alive = 0;
    /*
    for (int k = 0; k < req_len - 22; k++) {
        if (str_starts_with(req + k, "Connection: keep-alive")) {
            *keep_alive = 1;
            break;
        }
    }
    */    
    print("Method: ");
    if (is_head) print("HEAD");
    else if (is_get) print("GET");
    else print("POST");
    print(" Path: ");
    print(path);
    print("\n");
    
    if (is_head) {
        // HEAD request - just send headers, no body
        send_header(sock, 200, "OK", "text/html", 100, 0);
    } else if (is_post) {
        // Handle POST - for now just return 200 OK with JSON
        const char* body = "{\"status\":\"ok\",\"message\":\"POST received\"}";
        int len = str_len(body);
        send_header(sock, 200, "OK", "application/json", len, *keep_alive);
        syscall3(SYS_NET_SEND, sock, (int)body, len);
        
        // Poll to ensure it gets sent
        for (int p = 0; p < 1000; p++) {
            syscall1(SYS_NET_POLL, 0);
        }
    } else {
        // Handle GET - serve file
        serve_file(sock, path, *keep_alive);
    }
}

void _start(void) {
    print("KuzuOS HTTP Server starting on port 80...\n");
    
    // Initialize connections
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i].sock = -1;
        connections[i].active = 0;
        connections[i].keep_alive = 0;
    }
    
    // Create listening socket
    int listen_sock = syscall1(SYS_NET_LISTEN, 80);
    if (listen_sock < 0) {
        print("Failed to listen on port 80\n");
        syscall1(SYS_EXIT, 1);
    }
    
    print("Server ready! Waiting for connections...\n");
    
    // Main server loop
    while (1) {
        // Poll for network activity
        syscall1(SYS_NET_POLL, 0);
        
        // Try to accept new connections
        int new_sock = syscall1(SYS_NET_ACCEPT, listen_sock);
        if (new_sock >= 0) {
            print("New connection accepted\n");
            
            // Find free slot
            for (int i = 0; i < MAX_CONNECTIONS; i++) {
                if (!connections[i].active) {
                    connections[i].sock = new_sock;
                    connections[i].active = 1;
                    connections[i].keep_alive = 0;
                    break;
                }
            }
        }
        
        // Handle existing connections
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (!connections[i].active) continue;
            
            int sock = connections[i].sock;
            int n = syscall3(SYS_NET_RECV, sock, (int)buffer, BUFFER_SIZE - 1);
            
            if (n > 0) {
                buffer[n] = 0;
                print("Got HTTP request\n");
                
                int keep_alive = 0;
                handle_request(sock, buffer, n, &keep_alive);
                
                if (!keep_alive) {
                    // Give TCP time to send all data before closing
                    for (int delay = 0; delay < 10000; delay++) {
                        syscall1(SYS_NET_POLL, 0);
                    }
                    
                    syscall1(SYS_NET_CLOSE, sock);
                    connections[i].active = 0;
                    print("Connection closed\n");
                } else {
                    connections[i].keep_alive = 1;
                    print("Connection kept alive\n");
                }
            }
        }
        
        // Small delay to avoid spinning
        for (volatile int d = 0; d < 1000; d++);
    }
    
    syscall1(SYS_EXIT, 0);
}
