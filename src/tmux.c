#include "z_syscalls.h"

#define MAX_PANES 2
#define BUFFER_SIZE 256
#define MAX_HISTORY 10

typedef enum {
    SPLIT_NONE,
    SPLIT_HORIZONTAL,
    SPLIT_VERTICAL
} SplitType;

typedef struct {
    int active;
    char input_buffer[BUFFER_SIZE];
    int input_pos;
    char history[MAX_HISTORY][BUFFER_SIZE];
    int history_count;
    int history_index;
} Pane;

typedef struct {
    Pane panes[MAX_PANES];
    int num_panes;
    int active_pane;
    SplitType split_type;
} Multiplexer;

static Multiplexer mux;

// Helper functions
static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void print(const char* s) {
    z_write(1, s, strlen(s));
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void strcpy_safe(char* dst, const char* src, int max) {
    int i = 0;
    while (i < max - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void init_pane(Pane* pane) {
    pane->active = 1;
    pane->input_pos = 0;
    pane->history_count = 0;
    pane->history_index = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        pane->input_buffer[i] = 0;
    }
}

static void init_mux(void) {
    mux.num_panes = 1;
    mux.active_pane = 0;
    mux.split_type = SPLIT_NONE;
    
    for (int i = 0; i < MAX_PANES; i++) {
        mux.panes[i].active = 0;
    }
    
    init_pane(&mux.panes[0]);
}

static void execute_command(Pane* pane, const char* cmd) {
    // Save to history
    if (pane->history_count < MAX_HISTORY) {
        strcpy_safe(pane->history[pane->history_count], cmd, BUFFER_SIZE);
        pane->history_count++;
    } else {
        // Shift history
        for (int i = 1; i < MAX_HISTORY; i++) {
            strcpy_safe(pane->history[i-1], pane->history[i], BUFFER_SIZE);
        }
        strcpy_safe(pane->history[MAX_HISTORY-1], cmd, BUFFER_SIZE);
    }
    
    // Parse command name
    char prog[64] = {0};
    int i = 0;
    while (cmd[i] && cmd[i] != ' ' && i < 63) {
        prog[i] = cmd[i];
        i++;
    }
    prog[i] = '\0';
    
    if (strlen(prog) == 0) {
        return;
    }
    
    // Try to execute - prepend / if not absolute
    char full_path[128];
    if (prog[0] == '/') {
        strcpy_safe(full_path, prog, 128);
    } else {
        full_path[0] = '/';
        strcpy_safe(full_path + 1, prog, 127);
    }
    
    // Try to open the file
    int fd = z_open(full_path, O_RDONLY);
    if (fd < 0) {
        print("Command not found: ");
        print(prog);
        print("\n");
        return;
    }
    z_close(fd);
    
    // Execute it
    print("Executing: ");
    print(full_path);
    print("\n");
    
    // TODO: Add actual exec syscall when available
    // For now just acknowledge the command
}

static void draw_border_h(int count) {
    for (int i = 0; i < count; i++) {
        print("-");
    }
}

static void draw_divider(void) {
    print("\n");
    draw_border_h(78);
    print("\n");
}

static void draw_pane(Pane* pane, int pane_num, int is_active) {
    // Draw pane header
    print("[ Shell ");
    char num = '0' + pane_num;
    z_write(1, &num, 1);
    if (is_active) {
        print(" - ACTIVE ");
    }
    print(" ]");
    
    // Show last few history entries
    int start = pane->history_count > 3 ? pane->history_count - 3 : 0;
    for (int i = start; i < pane->history_count; i++) {
        print("\n> ");
        print(pane->history[i]);
    }
    
    // Show current input
    print("\n> ");
    print(pane->input_buffer);
}

static void draw_screen(void) {
    // Clear screen by printing newlines
    for (int i = 0; i < 30; i++) {
        print("\n");
    }
    
    print("=== TMUX - Terminal Multiplexer ===\n");
    print("Ctrl+A: H-Split | Ctrl+B: V-Split | Ctrl+E: Switch | Ctrl+D: Close\n");
    draw_border_h(78);
    print("\n");
    
    if (mux.num_panes == 1) {
        draw_pane(&mux.panes[0], 1, 1);
    } else if (mux.split_type == SPLIT_HORIZONTAL) {
        draw_pane(&mux.panes[0], 1, mux.active_pane == 0);
        draw_divider();
        draw_pane(&mux.panes[1], 2, mux.active_pane == 1);
    } else if (mux.split_type == SPLIT_VERTICAL) {
        draw_pane(&mux.panes[0], 1, mux.active_pane == 0);
        draw_divider();
        draw_pane(&mux.panes[1], 2, mux.active_pane == 1);
    }
    
    print("\n");
    draw_border_h(78);
    print("\n");
}

static void split_horizontal(void) {
    if (mux.num_panes < 2) {
        mux.num_panes = 2;
        mux.split_type = SPLIT_HORIZONTAL;
        init_pane(&mux.panes[1]);
        draw_screen();
    }
}

static void split_vertical(void) {
    if (mux.num_panes < 2) {
        mux.num_panes = 2;
        mux.split_type = SPLIT_VERTICAL;
        init_pane(&mux.panes[1]);
        draw_screen();
    }
}

static void switch_pane(void) {
    if (mux.num_panes > 1) {
        mux.active_pane = (mux.active_pane + 1) % mux.num_panes;
        draw_screen();
    }
}

static void close_pane(void) {
    if (mux.num_panes > 1) {
        mux.panes[mux.active_pane].active = 0;
        mux.num_panes--;
        mux.split_type = SPLIT_NONE;
        mux.active_pane = 0;
        draw_screen();
    } else {
        // Last pane - exit
        print("\nGoodbye!\n");
        z_exit(0);
    }
}

static void history_up(Pane* pane) {
    if (pane->history_count > 0 && pane->history_index > 0) {
        pane->history_index--;
        strcpy_safe(pane->input_buffer, pane->history[pane->history_index], BUFFER_SIZE);
        pane->input_pos = strlen(pane->input_buffer);
        draw_screen();
    }
}

static void history_down(Pane* pane) {
    if (pane->history_index < pane->history_count - 1) {
        pane->history_index++;
        strcpy_safe(pane->input_buffer, pane->history[pane->history_index], BUFFER_SIZE);
        pane->input_pos = strlen(pane->input_buffer);
        draw_screen();
    } else if (pane->history_index == pane->history_count - 1) {
        pane->history_index = pane->history_count;
        pane->input_buffer[0] = '\0';
        pane->input_pos = 0;
        draw_screen();
    }
}

void _start(void) {
    init_mux();
    draw_screen();
    
    char input_buf[256];
    Pane* active = &mux.panes[mux.active_pane];
    
    while (1) {
        int bytes = z_read(0, input_buf, sizeof(input_buf));
        
        if (bytes <= 0) continue;
        
        for (int i = 0; i < bytes; i++) {
            char c = input_buf[i];
            
            // Control sequences
            if (c == 0x01) {  // Ctrl+A
                split_horizontal();
                active = &mux.panes[mux.active_pane];
                continue;
            } else if (c == 0x02) {  // Ctrl+B
                split_vertical();
                active = &mux.panes[mux.active_pane];
                continue;
            } else if (c == 0x05) {  // Ctrl+E
                switch_pane();
                active = &mux.panes[mux.active_pane];
                continue;
            } else if (c == 0x04) {  // Ctrl+D
                close_pane();
                if (mux.num_panes == 0) return;
                active = &mux.panes[mux.active_pane];
                continue;
            } else if (c == '\n' || c == '\r') {
                // Execute command
                if (strcmp(active->input_buffer, "exit") == 0) {
                    close_pane();
                    if (mux.num_panes == 0) return;
                    active = &mux.panes[mux.active_pane];
                    continue;
                }
                
                if (active->input_pos > 0) {
                    execute_command(active, active->input_buffer);
                }
                
                // Reset history index
                active->history_index = active->history_count;
                
                // Clear input
                active->input_pos = 0;
                active->input_buffer[0] = '\0';
                draw_screen();
            } else if (c == 0x7F || c == 0x08) {  // Backspace
                if (active->input_pos > 0) {
                    active->input_pos--;
                    active->input_buffer[active->input_pos] = '\0';
                    draw_screen();
                }
            } else if ((unsigned char)c == 0x80) {  // Up arrow
                history_up(active);
            } else if ((unsigned char)c == 0x81) {  // Down arrow
                history_down(active);
            } else if (c >= 32 && c < 127) {  // Printable
                if (active->input_pos < BUFFER_SIZE - 1) {
                    active->input_buffer[active->input_pos++] = c;
                    active->input_buffer[active->input_pos] = '\0';
                    draw_screen();
                }
            }
        }
    }
    
    z_exit(0);
}