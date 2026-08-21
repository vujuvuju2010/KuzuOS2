
#include "shell.h"
#include "keyboard.h"
#include "filesystem.h"
#include "process.h"
#include "memory.h"
#include "vga.h"
#include "elf.h"
#include "keyboardusb.h"
#include "usb.h"

#ifndef NULL
#define NULL ((void*)0)
#endif
// Command search paths (in order of priority)
static const char* search_paths[] = {
    "/dev/",
   
    0
};

// String utilities
static void strcat(char* dest, const char* src) {
    int dest_len = strlen(dest);
    int i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

// Find command in search paths
static int find_command(const char* cmd, char* full_path, int max_len) {
    // First check if it's an absolute path
    if (cmd[0] == '/') {
        if (fs_any_exists((char*)cmd)) {
            strcpy(full_path, cmd);
            return 1;
        }
        return 0;
    }
    
    // Search in all paths
    for (int i = 0; search_paths[i] != NULL; i++) {
        int path_len = strlen(search_paths[i]);
        int cmd_len = strlen(cmd);
        
        if (path_len + cmd_len + 1 > max_len) {
            continue;
        }
        
        strcpy(full_path, search_paths[i]);
        strcat(full_path, cmd);
        
        // Check if file exists
        if (fs_any_exists(full_path)) {
            return 1;  // Found
        }
    }
    
    return 0;  // Not found
}

static void execute_command(char* input) {
    if (input[0] == '\0') {
        return;  // Empty command
    }
    
    // Parse command and arguments
    char cmd[128];
    char full_input[256];
    int i = 0;
    
    // Extract command name
    while (input[i] != '\0' && input[i] != ' ' && i < 127) {
        cmd[i] = input[i];
        i++;
    }
    cmd[i] = '\0';
    
    // Store full input line for argv parsing
    strcpy(full_input, input);
    
    // Find the command ELF binary
    char elf_path[256];
    if (find_command(cmd, elf_path, sizeof(elf_path))) {
        // Parse arguments into argv array for execve
        // Split input by spaces: "cd /lib" -> argv[0]="cd", argv[1]="/lib"
        char* argv[32];  // Support up to 32 arguments
        int argc = 0;
        
        // Allocate space for command copies
        char argv_storage[256];
        int storage_idx = 0;
        
        // First argument is the command itself
        int cmd_len = 0;
        while (cmd[cmd_len]) cmd_len++;
        
        argv[argc] = &argv_storage[storage_idx];
        for (int j = 0; j <= cmd_len; j++) {
            argv_storage[storage_idx++] = cmd[j];
        }
        argc++;
        
        // Parse remaining arguments
        i = 0;
        while (full_input[i] != '\0' && full_input[i] != ' ') {
            i++;  // Skip command name
        }
        
        while (full_input[i] != '\0' && argc < 31) {
            // Skip spaces
            while (full_input[i] == ' ') i++;
            
            if (full_input[i] == '\0') break;
            
            // Start of next argument
            argv[argc] = &argv_storage[storage_idx];
            
            // Copy argument until space or end
            while (full_input[i] != '\0' && full_input[i] != ' ' && storage_idx < 255) {
                argv_storage[storage_idx++] = full_input[i];
                i++;
            }
            
            // Null terminate argument
            argv_storage[storage_idx++] = '\0';
            argc++;
        }
        
        // Null terminate argv array
        argv[argc] = NULL;
        
        // Debug: show command and argv being passed to ELF loader
        print_color("[shell] exec: ", VGA_COLOR_LIGHT_GREY);
        print_color(elf_path, VGA_COLOR_LIGHT_GREY);
        print_color(" argc=", VGA_COLOR_LIGHT_GREY);
        {
            // Simple int to string
            int tmp_argc = argc;
            char nbuf[16];
            int pos = 0;
            if (tmp_argc == 0) {
                nbuf[pos++] = '0';
            } else {
                if (tmp_argc < 0) {
                    nbuf[pos++] = '-';
                    tmp_argc = -tmp_argc;
                }
                char rev[16];
                int rp = 0;
                while (tmp_argc > 0 && rp < 15) {
                    rev[rp++] = '0' + (tmp_argc % 10);
                    tmp_argc /= 10;
                }
                while (rp--) {
                    nbuf[pos++] = rev[rp];
                }
            }
            nbuf[pos] = '\0';
            print_color(nbuf, VGA_COLOR_LIGHT_GREY);
        }
        print_color(" argv: ", VGA_COLOR_LIGHT_GREY);
        for (int ai = 0; ai < argc; ai++) {
            print_color("[", VGA_COLOR_LIGHT_GREY);
            print_color(argv[ai], VGA_COLOR_LIGHT_CYAN);
            print_color("]", VGA_COLOR_LIGHT_GREY);
        }
        print_color("\n", VGA_COLOR_LIGHT_GREY);

        // Execute with proper Linux-style execve
        extern int elf_load_and_execve(const char* filename, char* const argv[], char* const envp[]);
        elf_load_and_execve(elf_path, argv, NULL);
        
    } else {
        // Command not found
        print_color(cmd, VGA_COLOR_LIGHT_RED);
        print_color(": command not found\n", VGA_COLOR_LIGHT_RED);
    }
}

void shell_run() {
    char input[256];

    
    shell_print_prompt();
    
    while (1) {
        int len = shell_readline(input, sizeof(input));
        
        // Execute command
        if (len > 0) {
            execute_command(input);
        }
        
        shell_print_prompt();
    }
}

void shell_execute_command(char* command) {
    execute_command(command);
}

void shell_init() {
    clear_screen();

}

void shell_print_prompt() {
    print("kuzuos> ");
}

// History and readline
char current_directory[64] = "/";

#define HISTORY_MAX 16
static char history[HISTORY_MAX][256];
static int history_count = 0;
static int history_index = -1;

static void erase_last_char_visual() {
    putchar('\b');
    putchar(' ');
    putchar('\b');
}

static void clear_current_line_visual(int* pos) {
    while (*pos > 0) { erase_last_char_visual(); (*pos)--; }
}

int shell_readline(char* buf, int maxlen) {
    int pos = 0;
    history_index = history_count;
    
    while (1) {
        char c = keyboard_get_char();
if (!c)
{
    keyboard_poll();
    extern void usb_poll(void);
    usb_poll();
    continue;
}        
        if (c == '\n' || c == '\r') {
            if (pos < maxlen) buf[pos] = '\0';
            else buf[maxlen-1] = '\0';
            putchar('\n');
            
            if (pos > 0) {
                if (history_count < HISTORY_MAX) {
                    int idx = history_count++;
                    int i=0;
                    while (i < 255 && buf[i]) {
                        history[idx][i] = buf[i];
                        i++;
                    }
                    history[idx][i] = 0;
                } else {
                    for (int r=1; r<HISTORY_MAX; r++) {
                        int i=0;
                        while (i<256) {
                            history[r-1][i] = history[r][i];
                            i++;
                        }
                    }
                    int i=0;
                    while (i < 255 && buf[i]) {
                        history[HISTORY_MAX-1][i] = buf[i];
                        i++;
                    }
                    history[HISTORY_MAX-1][i] = 0;
                }
            }
            return pos;
        }
        else if (c == '\b') {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                erase_last_char_visual();
            }
        }
        else if ((unsigned char)c == 0x7F) {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                erase_last_char_visual();
            }
        }
        else if ((unsigned char)c == 0x80) { // UP
            if (history_count > 0 && history_index > 0) {
                history_index--;
                clear_current_line_visual(&pos);
                int i=0;
                while (history[history_index][i] && i < maxlen-1) {
                    buf[i] = history[history_index][i];
                    putchar(buf[i]);
                    i++;
                }
                buf[i] = 0;
                pos = i;
            }
        }
        else if ((unsigned char)c == 0x81) { // DOWN
            if (history_index < history_count-1) {
                history_index++;
                clear_current_line_visual(&pos);
                int i=0;
                while (history[history_index][i] && i < maxlen-1) {
                    buf[i] = history[history_index][i];
                    putchar(buf[i]);
                    i++;
                }
                buf[i] = 0;
                pos = i;
            } else if (history_index == history_count-1) {
                history_index = history_count;
                clear_current_line_visual(&pos);
                buf[0] = 0;
                pos = 0;
            }
        }
        else if (pos < maxlen - 1) {
            buf[pos++] = c;
            buf[pos] = '\0';
            putchar(c);
        }
    }
}