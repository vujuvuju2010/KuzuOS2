// mouse_app.c - Simple mouse cursor demo for KuzuOS2
// Displays a red block cursor that follows the mouse on VESA framebuffer
// Click to turn screen red, release to restore

typedef char int8_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;

// Mouse event structure (matches kernel)
typedef struct {
    int8_t  dx;
    int8_t  dy;
    uint8_t buttons;
} mouse_event_t;

// Syscalls
static inline int32_t syscall1(int num, uint32_t arg1) {
    int32_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
    return ret;
}

static inline int32_t syscall3(int num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    int32_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

static inline int32_t syscall6(int num, uint32_t arg1, uint32_t arg2, uint32_t arg3, 
                                uint32_t arg4, uint32_t arg5, uint32_t arg6) {
    int32_t ret;
    register uint32_t r_arg4 __asm__("r10") = arg4;
    register uint32_t r_arg5 __asm__("r8") = arg5;
    register uint32_t r_arg6 __asm__("r9") = arg6;
    __asm__ volatile("int $0x80" 
                     : "=a"(ret) 
                     : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3), "r"(r_arg4), "r"(r_arg5), "r"(r_arg6));
    return ret;
}

#define SYS_EXIT          1
#define SYS_WRITE         4
#define SYS_BRK           45   // Memory allocation
#define SYS_MMAP2         192  // Memory mapping
#define SYS_MOUSE_POLL    310
#define SYS_USB_POLL      311  // Trigger USB polling
#define SYS_GET_FB        312  // Get framebuffer info
#define SYS_KERNEL_CURSOR 313  // Enable/disable kernel cursor

// Simple helpers
static void exit(int code) {
    syscall1(SYS_EXIT, code);
    while(1);  // should never reach
}

static void write_str(const char* str) {
    int len = 0;
    while (str[len]) len++;
    syscall3(SYS_WRITE, 1, (uint32_t)str, len);
}

// Simple integer to string for printing
static void print_num(const char* prefix, int num) {
    char buf[32];
    int i = 0;
    
    // Print prefix
    write_str(prefix);
    
    // Handle negative
    int is_neg = 0;
    if (num < 0) {
        write_str("-");
        num = -num;
        is_neg = 1;
    }
    
    // Convert to string (reverse)
    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    // Reverse and print
    for (int j = i - 1; j >= 0; j--) {
        char c = buf[j];
        syscall3(SYS_WRITE, 1, (uint32_t)&c, 1);
    }
    write_str("\n");
}

// Print byte in binary
static void print_binary(const char* prefix, uint8_t byte) {
    char buf[16];
    
    write_str(prefix);
    
    // Format: "0b00000000"
    buf[0] = '0';
    buf[1] = 'b';
    for (int i = 7; i >= 0; i--) {
        buf[2 + (7-i)] = (byte & (1 << i)) ? '1' : '0';
    }
    buf[10] = '\n';
    buf[11] = '\0';
    
    syscall3(SYS_WRITE, 1, (uint32_t)buf, 11);
}

static int mouse_poll(mouse_event_t* event) {
    return syscall1(SYS_MOUSE_POLL, (uint32_t)event);
}

static void usb_poll(void) {
    syscall1(SYS_USB_POLL, 0);
}

// Framebuffer info structure
typedef struct {
    uint32_t* addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} fb_info_t;

static int get_framebuffer(fb_info_t* info) {
    return syscall1(SYS_GET_FB, (uint32_t)info);
}

static void enable_kernel_cursor(int enable) {
    syscall1(SYS_KERNEL_CURSOR, enable);
}

// Simple memory allocation using mmap  
static void* alloc_memory(uint32_t size) {
    // Try mmap first: mmap(addr=0, length, prot=3 (RW), flags=0x22 (PRIVATE|ANON), fd=-1, offset=0)
    void* result = (void*)syscall6(SYS_MMAP2, 0, size, 3, 0x22, (uint32_t)-1, 0);
    if ((int32_t)result < 0 && (int32_t)result > -4096) {
        // mmap failed, try brk
        write_str("[ALLOC] mmap failed, trying brk\n");
        
        // Get current brk
        uint32_t current_brk = syscall1(SYS_BRK, 0);
        if (current_brk == 0) {
            return 0;
        }
        
        // Try to extend by size
        uint32_t new_brk = syscall1(SYS_BRK, current_brk + size);
        if (new_brk >= current_brk + size) {
            return (void*)current_brk;
        }
        
        return 0;
    }
    return result;
}

// Framebuffer info - will be filled by syscall
static uint32_t* framebuffer = 0;  // This is the FRONT buffer (visible)
static uint32_t* backbuffer = 0;    // This is the BACK buffer (we draw here)
static uint32_t fb_width = 1920;
static uint32_t fb_height = 1080;

// Cursor state
static int cursor_x = 960;  // Start at center
static int cursor_y = 540;

// Cursor size in pixels
#define CURSOR_SIZE 10

// Saved screen data under cursor (not used in double buffer approach)
static uint32_t saved_block[CURSOR_SIZE * CURSOR_SIZE];

// Track dirty region for optimized presenting
static int dirty_x_min = 0;
static int dirty_y_min = 0;  
static int dirty_x_max = 0;
static int dirty_y_max = 0;
static int screen_fully_dirty = 1;

// Mark region as dirty
static void mark_dirty(int x, int y, int w, int h) {
    if (screen_fully_dirty) {
        return;  // Already fully dirty
    }
    
    if (dirty_x_min == 0 && dirty_y_min == 0 && dirty_x_max == 0 && dirty_y_max == 0) {
        // First dirty region
        dirty_x_min = x;
        dirty_y_min = y;
        dirty_x_max = x + w;
        dirty_y_max = y + h;
    } else {
        // Expand dirty region
        if (x < dirty_x_min) dirty_x_min = x;
        if (y < dirty_y_min) dirty_y_min = y;
        if (x + w > dirty_x_max) dirty_x_max = x + w;
        if (y + h > dirty_y_max) dirty_y_max = y + h;
    }
}

// Copy backbuffer to framebuffer (present/flip) - optimized version
static void present_buffer(void) {
    if (screen_fully_dirty) {
        // Copy entire screen (needed on first frame or after full screen change)
        uint32_t total_pixels = fb_width * fb_height;
        for (uint32_t i = 0; i < total_pixels; i++) {
            framebuffer[i] = backbuffer[i];
        }
        screen_fully_dirty = 0;
        dirty_x_min = dirty_y_min = dirty_x_max = dirty_y_max = 0;
    } else if (dirty_x_max > 0 || dirty_y_max > 0) {
        // Only copy dirty region
        for (int y = dirty_y_min; y < dirty_y_max && y < (int)fb_height; y++) {
            for (int x = dirty_x_min; x < dirty_x_max && x < (int)fb_width; x++) {
                framebuffer[y * fb_width + x] = backbuffer[y * fb_width + x];
            }
        }
        dirty_x_min = dirty_y_min = dirty_x_max = dirty_y_max = 0;
    }
}

// Save pixels under cursor from BACKBUFFER
static void save_cursor_area(void) {
    int idx = 0;
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx < CURSOR_SIZE; dx++) {
            int px = cursor_x + dx;
            int py = cursor_y + dy;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                saved_block[idx] = backbuffer[py * fb_width + px];
            } else {
                saved_block[idx] = 0;
            }
            idx++;
        }
    }
}

// Restore pixels under cursor to BACKBUFFER
static void restore_cursor_area(void) {
    int idx = 0;
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx < CURSOR_SIZE; dx++) {
            int px = cursor_x + dx;
            int py = cursor_y + dy;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                backbuffer[py * fb_width + px] = saved_block[idx];
            }
            idx++;
        }
    }
    // Mark old cursor area as dirty
    mark_dirty(cursor_x, cursor_y, CURSOR_SIZE, CURSOR_SIZE);
}

// Draw red cursor block to BACKBUFFER
static void draw_cursor(void) {
    uint32_t red = 0xFFAA0000;  // BGR format: 00=blue, 00=green, AA=red, FF=alpha
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx < CURSOR_SIZE; dx++) {
            int px = cursor_x + dx;
            int py = cursor_y + dy;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                backbuffer[py * fb_width + px] = red;
            }
        }
    }
    // Mark new cursor area as dirty
    mark_dirty(cursor_x, cursor_y, CURSOR_SIZE, CURSOR_SIZE);
}

// Turn entire BACKBUFFER red
static void screen_red(void) {
    uint32_t red = 0xFFAA0000;  // BGR format: Red
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            backbuffer[y * fb_width + x] = red;
        }
    }
}

// Draw screen border in red (less invasive than full screen flash)
static void draw_red_border(void) {
    uint32_t red = 0xFFAA0000;  // Red in BGR (00=B, 00=G, AA=R)
    int border = 8;
    
    // Top and bottom borders
    for (uint32_t y = 0; y < (uint32_t)border; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            backbuffer[y * fb_width + x] = red;
            backbuffer[(fb_height - 1 - y) * fb_width + x] = red;
        }
    }
    
    // Left and right borders
    for (uint32_t y = (uint32_t)border; y < fb_height - (uint32_t)border; y++) {
        for (uint32_t x = 0; x < (uint32_t)border; x++) {
            backbuffer[y * fb_width + x] = red;
            backbuffer[y * fb_width + (fb_width - 1 - x)] = red;
        }
    }
}

// Draw screen border in green for right-click
static void draw_green_border(void) {
    uint32_t green = 0xFF00AA00;  // Green in BGR
    int border = 8;
    
    // Top and bottom borders
    for (uint32_t y = 0; y < (uint32_t)border; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            backbuffer[y * fb_width + x] = green;
            backbuffer[(fb_height - 1 - y) * fb_width + x] = green;
        }
    }
    
    // Left and right borders
    for (uint32_t y = (uint32_t)border; y < fb_height - (uint32_t)border; y++) {
        for (uint32_t x = 0; x < (uint32_t)border; x++) {
            backbuffer[y * fb_width + x] = green;
            backbuffer[y * fb_width + (fb_width - 1 - x)] = green;
        }
    }
}

// Draw yellow border for middle button
static void draw_yellow_border(void) {
    uint32_t yellow = 0xFF00FFFF;  // Yellow in BGR
    int border = 8;
    
    // Top and bottom borders
    for (uint32_t y = 0; y < (uint32_t)border; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            backbuffer[y * fb_width + x] = yellow;
            backbuffer[(fb_height - 1 - y) * fb_width + x] = yellow;
        }
    }
    
    // Left and right borders
    for (uint32_t y = (uint32_t)border; y < fb_height - (uint32_t)border; y++) {
        for (uint32_t x = 0; x < (uint32_t)border; x++) {
            backbuffer[y * fb_width + x] = yellow;
            backbuffer[y * fb_width + (fb_width - 1 - x)] = yellow;
        }
    }
}

// Decode and print button byte details
static void print_button_info(uint8_t buttons) {
    write_str("\n=== USB Mouse Packet (Byte 0) ===\n");
    print_binary("Raw byte:     ", buttons);
    write_str("Bit breakdown:\n");
    write_str("  [7] Y overflow:  "); write_str((buttons & 0x80) ? "YES\n" : "NO\n");
    write_str("  [6] X overflow:  "); write_str((buttons & 0x40) ? "YES\n" : "NO\n");
    write_str("  [5] Y sign bit:  "); write_str((buttons & 0x20) ? "1 (neg)\n" : "0 (pos)\n");
    write_str("  [4] X sign bit:  "); write_str((buttons & 0x10) ? "1 (neg)\n" : "0 (pos)\n");
    write_str("  [3] Always 1:    "); write_str((buttons & 0x08) ? "1 ✓\n" : "0 ✗\n");
    write_str("  [2] Middle Btn:  "); write_str((buttons & 0x04) ? "PRESSED\n" : "released\n");
    write_str("  [1] Right Btn:   "); write_str((buttons & 0x02) ? "PRESSED\n" : "released\n");
    write_str("  [0] Left Btn:    "); write_str((buttons & 0x01) ? "PRESSED\n" : "released\n");
    write_str("================================\n\n");
}

void _start(void) {
    write_str("\n");
    write_str("========================================\n");
    write_str("  KuzuOS Mouse Demo - Kernel Cursor\n");
    write_str("========================================\n");
    write_str("\n");
    
    write_str("Checking for USB mouse...\n");
    
    // Try to detect mouse
    mouse_event_t test_event;
    int mouse_detected = 0;
    
    for (int i = 0; i < 100; i++) {
        usb_poll();
        if (mouse_poll(&test_event)) {
            mouse_detected = 1;
            write_str("\n✓ USB Mouse DETECTED!\n");
            write_str("\nInitial mouse event:\n");
            print_num("  dx: ", test_event.dx);
            print_num("  dy: ", test_event.dy);
            print_button_info(test_event.buttons);
            break;
        }
        for (volatile int j = 0; j < 10000; j++);
    }
    
    if (!mouse_detected) {
        write_str("\n✗ No USB mouse detected.\n");
        write_str("Make sure a USB mouse is connected.\n");
        write_str("\nEnabling kernel cursor anyway...\n");
    }
    
    write_str("\n");
    write_str("Enabling kernel-side cursor rendering...\n");
    write_str("The red cursor will remain after this program exits!\n");
    write_str("\n");
    
    // Enable kernel cursor
    enable_kernel_cursor(1);
    
    write_str("✓ Kernel cursor enabled!\n");
    write_str("  - Red 10x10 pixel cursor\n");
    write_str("  - Tracks mouse movement automatically\n");
    write_str("  - Persists after program exit\n");
    write_str("\n");
    write_str("Demo: Moving cursor for 5 seconds...\n");
    
    // Let user see the cursor for 5 seconds
    for (int sec = 5; sec > 0; sec--) {
        print_num("  ", sec);
        for (volatile int d = 0; d < 10000000; d++) {
            // Poll USB to keep cursor updating
            if (d % 100000 == 0) {
                usb_poll();
            }
        }
    }
    
    write_str("\n");
    write_str("Exiting to shell...\n");
    write_str("Cursor should remain visible and track mouse!\n");
    write_str("\n");
    
    exit(0);
}
