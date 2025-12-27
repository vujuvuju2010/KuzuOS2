






















#include "shell.h"
#include "keyboard.h"
#include "filesystem.h"
#include "process.h"
#include "vga.h"
#include "elf.h"
#include "banner.h"

// Donanım reboot fonksiyonu
static void hw_reboot() {
    __asm__ volatile("cli");
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"(0x64));
    while (1) { __asm__ volatile("hlt"); }
}

// String fonksiyonları
static void strcat(char* dest, const char* src) {
    int dest_len = strlen(dest);
    int i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

static char* strrchr(const char* str, char c) {
    char* last = 0;
    while (*str != '\0') {
        if (*str == c) {
            last = (char*)str;
        }
        str++;
    }
    return last;
}

// Shell variables
static char command_buffer[MAX_COMMAND_LENGTH];
static int command_pos = 0;

// Global variables
char input[256];
int input_pos = 0;
char current_directory[64] = "/";  // Current directory

#define HISTORY_MAX 16
static char history[HISTORY_MAX][256];
static int history_count = 0;
static int history_index = -1;

static void erase_last_char_visual() {
    // Move back, blank, move back
    putchar('\b');
    putchar(' ');
    putchar('\b');
}

static void clear_current_line_visual(int* pos) {
    while (*pos > 0) { erase_last_char_visual(); (*pos)--; }
}

void shell_init() {
    clear_screen();
    print("KuzuOS Shell v1.0\n");
    print("Type 'help' for available commands\n\n");
}

// Global banner reference for shell
static struct banner* shell_banner = 0;

void shell_set_banner(struct banner* banner) {
    shell_banner = banner;
}

void shell_print_prompt() {
    print("kuzuos> ");
}

void shell_run() {
    char input[256];
    int input_pos = 0;

    print("KuzuOS Shell v1.0\n");
    print("Type 'help' for commands\n");
    shell_print_prompt();

    // Test için basit bir komut çalıştır
    shell_execute_command("help");
    shell_print_prompt();
    
    // Banner updates disabled in shell to reduce overhead
    // while (1) {
    while (1) {
        int len = shell_readline(input, sizeof(input));
        if (len == 0) {
            shell_print_prompt();
            continue;
        }
        if (strcmp(input, "help") == 0) {
            cmd_help();
        } else if (strncmp(input, "run ", 4) == 0) {
            const char* filename = input + 4;
            // Auto-prepend / if not absolute path
            char full_path[64];
            if (filename[0] == '/') {
                strcpy(full_path, filename);
            } else {
                full_path[0] = '/';
                strcpy(full_path + 1, filename);
            }
            elf_load_and_run(full_path);
        } else if (strcmp(input, "clear") == 0) {
            cmd_clear();
        } else if (strncmp(input, "ls ", 3) == 0) {
            cmd_ls_param(input + 3);
        } else if (strcmp(input, "ls") == 0) {
            cmd_ls();
        } else if (strcmp(input, "pwd") == 0) {
            cmd_pwd();
        } else if (strcmp(input, "lsall") == 0) {
            fs_list_all();
        } else if (strcmp(input, "whoami") == 0) {
            cmd_whoami();
        } else if (strcmp(input, "date") == 0) {
            cmd_date();
        } else if (strcmp(input, "uname") == 0) {
            cmd_uname();
        } else if (strcmp(input, "save") == 0) {
            cmd_save();
        } else if (strcmp(input, "disktest") == 0) {
            cmd_disktest();
        } else if (strncmp(input, "echo ", 5) == 0) {
            cmd_echo(input + 5);
        } else if (strncmp(input, "cat ", 4) == 0) {
            cmd_cat(input + 4);
        } else if (strncmp(input, "mkdir ", 6) == 0) {
            cmd_mkdir(input + 6);
        } else if (strncmp(input, "cd ", 3) == 0) {
            cmd_cd(input + 3);
        } else if (strncmp(input, "rm ", 3) == 0) {
            cmd_rm(input + 3);
        } else if (strncmp(input, "touch ", 6) == 0) {
            cmd_touch(input + 6);
        } else if (strncmp(input, "cp ", 3) == 0) {
            cmd_cp(input + 3);
        } else if (strncmp(input, "mv ", 3) == 0) {
            cmd_mv(input + 3);
        } else if (strcmp(input, "exit") == 0) {
            print_color("Exiting shell.\n", VGA_COLOR_LIGHT_GREY);
            break;
        } else if (strcmp(input, "reboot") == 0) {
            print_color("System rebooting...\n", VGA_COLOR_LIGHT_GREY);
            hw_reboot();
        } else if (strcmp(input, "banner") == 0) {
            cmd_banner();
        } else {
            print("Unknown command: ");
            print(input);
            print("\n");
        }
        shell_print_prompt();
    }
}

void shell_execute_command(char* command) {
    if (strcmp(command, "help") == 0) {
        cmd_help();
    } else if (strcmp(command, "clear") == 0) {
        cmd_clear();
    } else if (strncmp(command, "ls ", 3) == 0) {
        cmd_ls_param(command + 3);
    } else if (strcmp(command, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(command, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(command, "whoami") == 0) {
        cmd_whoami();
    } else if (strcmp(command, "date") == 0) {
        cmd_date();
    } else if (strcmp(command, "uname") == 0) {
        cmd_uname();
    } else if (strcmp(command, "save") == 0) {
        cmd_save();
    } else if (strcmp(command, "disktest") == 0) {
        cmd_disktest();
    } else if (strncmp(command, "echo ", 5) == 0) {
        cmd_echo(command + 5);
    } else if (strncmp(command, "cat ", 4) == 0) {
        cmd_cat(command + 4);
    } else if (strncmp(command, "mkdir ", 6) == 0) {
        cmd_mkdir(command + 6);
    } else if (strncmp(command, "cd ", 3) == 0) {
        cmd_cd(command + 3);
    } else if (strncmp(command, "rm ", 3) == 0) {
        cmd_rm(command + 3);
    } else if (strncmp(command, "touch ", 6) == 0) {
        cmd_touch(command + 6);
    } else if (strncmp(command, "cp ", 3) == 0) {
        cmd_cp(command + 3);
    } else if (strncmp(command, "mv ", 3) == 0) {
        cmd_mv(command + 3);
    } else if (strcmp(command, "banner") == 0) {
        cmd_banner();
    } else {
        print("Unknown command: ");
        print(command);
        print("\n");
    }
}

void cmd_help() {
    print("Available commands:\n");
    print("  help - Show this help\n");
    print("  clear - Clear screen\n");
    print("  echo <text> - Print text\n");
    print("  ls [dir] - List files in directory\n");
    print("  lsall - List every file detected in RAM (ISO + TinyFS)\n");
    print("  cat <file> - Show file contents\n");
    print("  pwd - Print working directory\n");
    print("  whoami - Show current user\n");
    print("  date - Show current date\n");
    print("  uname - Show system info\n");
    print("  save - Manually save filesystem to disk\n");
    print("  disktest - Test disk I/O operations\n");
    print("  mkdir <dir> - Create directory\n");
    print("  cd <dir> - Change directory\n");
    print("  rm [-r|-f|-rf] <file/dir> - Remove file or directory\n");
    print("  touch <file> - Create empty file\n");
    print("  cp <src> <dst> - Copy file\n");
    print("  mv <src> <dst> - Move file\n");
    print("  run <file> - Run ELF binary\n");
    print("  exit - Exit shell\n");
    print("  reboot - Reboot system\n");
    print("  banner - Display animated banner\n");
}

void cmd_clear() {
    clear_screen();
}

void cmd_echo(char* args) {
    print(args);
    print("\n");
}

void cmd_ls_param(char* param) {
    // Her ls'de RAM'i diskten güncelle
    char path[64];
    if (param == 0 || param[0] == 0) {
        // Parametre yoksa mevcut dizin
        strcpy(path, current_directory);
    } else if (param[0] == '/') {
        // Absolute path
        strcpy(path, param);
    } else {
        // Relative path
        strcpy(path, current_directory);
        int len = strlen(current_directory);
        if (len > 0 && current_directory[len-1] != '/') {
            strcat(path, "/");
        }
        strcat(path, param);
    }
    fs_list_files(path);
}

void cmd_ls() {
    cmd_ls_param(0);
}

void cmd_cat(char* filename) {
    // Path oluştur
    char full_path[64];
    if (filename[0] == '/') {
        // Absolute path
        strcpy(full_path, filename);
    } else {
        // Relative path
        strcpy(full_path, current_directory);
        if (current_directory[strlen(current_directory) - 1] != '/') {
            strcat(full_path, "/");
        }
        strcat(full_path, filename);
    }
    
    char buffer[256];
    int result = fs_read_file(full_path, buffer, 256);
    if (result > 0) {
        for (int i = 0; i < result; i++) {
            putchar(buffer[i]);
        }
        print("\n");
    } else {
        print("File not found: ");
        print(full_path);
        print("\n");
    }
}

void cmd_pwd() {
    print(current_directory);
    print("\n");
}

void cmd_whoami() {
    print("root\n");
}

void cmd_date() {
    print("2024-01-01 12:00:00\n");
}

void cmd_uname() {
    print("KuzuOS 1.0 x86_32\n");
}

void cmd_mkdir(char* dirname) {
    // Path oluştur
    char full_path[64];
    if (dirname[0] == '/') { strcpy(full_path, dirname); } else { strcpy(full_path, current_directory); if (current_directory[strlen(current_directory) - 1] != '/') { strcat(full_path, "/"); } strcat(full_path, dirname); }
    // If already exists anywhere, report
    if (fs_any_exists(full_path)) {
        print_color("mkdir: already exists\n", VGA_COLOR_LIGHT_RED);
        return;
    }
    int res = fs_create_directory(full_path);
    if (res == 0) { print("Created directory: "); print(full_path); print("\n"); }
    else { print_color("Failed to create directory: ", VGA_COLOR_LIGHT_RED); print_color(full_path, VGA_COLOR_LIGHT_RED); print_color("\n", VGA_COLOR_LIGHT_RED); if (res == -4) { print_color("[ERROR] Directory could not be saved to disk!\n", VGA_COLOR_LIGHT_RED); } }
}

void cmd_cd(char* dirname) {
    if (strcmp(dirname, "/") == 0) { strcpy(current_directory, "/"); print("Changed directory to: /\n"); return; }
    if (strcmp(dirname, "..") == 0) {
        if (strcmp(current_directory, "/") != 0) { char* last_slash = strrchr(current_directory, '/'); if (last_slash == current_directory) { strcpy(current_directory, "/"); } else if (last_slash) { *last_slash = '\0'; } }
        print("Changed directory to: "); print(current_directory); print("\n"); return;
    }
    char full_path[64]; if (dirname[0] == '/') { strcpy(full_path, dirname); } else { strcpy(full_path, current_directory); if (current_directory[strlen(current_directory) - 1] != '/') { strcat(full_path, "/"); } strcat(full_path, dirname); }
    if (fs_any_exists(full_path) && fs_any_is_directory(full_path)) { strcpy(current_directory, full_path); print("Changed directory to: "); print(current_directory); print("\n"); }
    else { print("Directory not found: "); print(full_path); print("\n"); }
}

void cmd_rm(char* args) {
    int recursive = 0, force = 0;
    char* filename = args;
    while (*filename == ' ') filename++;
    if (strncmp(filename, "-rf", 3) == 0 || strncmp(filename, "-fr", 3) == 0) {
        recursive = 1; force = 1; filename += 3;
    } else if (strncmp(filename, "-r", 2) == 0) {
        recursive = 1; filename += 2;
    } else if (strncmp(filename, "-f", 2) == 0) {
        force = 1; filename += 2;
    }
    while (*filename == ' ') filename++;
    if (*filename == 0) {
        print_color("rm: missing operand\n", VGA_COLOR_LIGHT_RED);
        return;
    }
    // Path oluştur
    char full_path[64];
    if (filename[0] == '/') {
        strcpy(full_path, filename);
    } else {
        strcpy(full_path, current_directory);
        if (current_directory[strlen(current_directory) - 1] != '/')
            strcat(full_path, "/");
        strcat(full_path, filename);
    }
    int res = fs_delete_file(full_path, recursive);
    if (res == 0) {
        print("Removed: "); print(full_path); print("\n");
    } else if (res == -3) {
        print_color("rm: cannot remove directory: not empty (use -r)\n", VGA_COLOR_LIGHT_RED);
    } else if (!force) {
        print_color("rm: failed to remove: ", VGA_COLOR_LIGHT_RED);
        print_color(full_path, VGA_COLOR_LIGHT_RED);
        print_color("\n", VGA_COLOR_LIGHT_RED);
    }
}

void cmd_touch(char* filename) {
    char full_path[64]; if (filename[0] == '/') { strcpy(full_path, filename); } else { strcpy(full_path, current_directory); if (current_directory[strlen(current_directory) - 1] != '/') { strcat(full_path, "/"); } strcat(full_path, filename); }
    if (fs_any_exists(full_path)) { print("touch: file exists: "); print(full_path); print("\n"); return; }
    if (fs_create_file(full_path, "", 0) == 0) { print("Created file: "); print(full_path); print("\n"); }
    else { print("Failed to create file: "); print(full_path); print("\n"); }
}

void cmd_cp(char* args) {
    print("Copy: ");
    print(args);
    print("\n");
}

void cmd_mv(char* args) {
    print("Move: ");
    print(args);
    print("\n");
}

void cmd_save() {
    print("Saving filesystem to disk...\n");
    print("(Not needed: all operations are now direct to disk)\n");
}

void cmd_disktest() {
    print("Running disk I/O test...\n");
    if (fs_disk_test() == 0) {
        print("Disk test completed successfully\n");
    } else {
        print("Disk test failed\n");
    }
}

// Simple delay function for banner animation
static void banner_delay(int ms) {
    for (volatile int i = 0; i < ms * 7000; i++) {
        __asm__ volatile("nop");
    }
}

#define MAX_BANNER_FRAMES_CHECK 100  // Should match banner.c

void cmd_banner() {
    // Get cursor position BEFORE printing anything - this is where the command was entered
    int start_cursor_x = get_cursor_x();
    int start_cursor_y = get_cursor_y();
    
    print("Banner: Starting...\n");
    
    struct banner anim_banner;
    extern uint32_t fb_width;
    extern uint32_t fb_height;
    
    // Safety check - make sure framebuffer is initialized
    if (fb_width == 0 || fb_height == 0) {
        print_color("Banner: Framebuffer not initialized\n", VGA_COLOR_LIGHT_RED);
        return;
    }
    
    // Calculate position - place banner below where command was entered + status messages
    // Each line is 16 pixels tall
    int banner_x = 0;  // Start at left edge (like text output)
    int banner_y = 0;  // Will be calculated after we print status messages
    
    // Initialize banner
    banner_init(&anim_banner, banner_x, banner_y);
    
    // Safety check - make sure frames array is initialized
    if (!anim_banner.frames) {
        print_color("Banner: Failed to initialize frames array\n", VGA_COLOR_LIGHT_RED);
        return;
    }
    
    print("Banner: Loading frames...\n");
    
    // Try to load banner frames - support multiple naming patterns
    int frames_loaded = 0;
    char frame_path[64];
    
    // Try loading frames one at a time, very carefully
    for (int i = 0; i < 10; i++) {
        int loaded_this_frame = 0;
        uint32_t num_frames_before = anim_banner.num_frames;
        
        // Try pattern 1: /BANNER_0.BIN, /BANNER_1.BIN, etc. (uppercase, simple)
        // Build path safely - ensure null termination
        frame_path[0] = '/';
        frame_path[1] = 'B';
        frame_path[2] = 'A';
        frame_path[3] = 'N';
        frame_path[4] = 'N';
        frame_path[5] = 'E';
        frame_path[6] = 'R';
        frame_path[7] = '_';
        frame_path[8] = '0' + i;  // i is 0-9, so single digit
        frame_path[9] = '.';
        frame_path[10] = 'B';
        frame_path[11] = 'I';
        frame_path[12] = 'N';
        frame_path[13] = '\0';
        
        // Try to load frame with first pattern - wrap in safety
        // Check if file exists first to avoid crashes
        if (i < MAX_BANNER_FRAMES_CHECK && anim_banner.frames) {
            // Try to load frame - this might crash if file is invalid
            // Wrap in as much safety as possible
            print("Banner: Trying to load frame ");
            putchar('0' + i);
            print("...\n");
            
            banner_load_frame(&anim_banner, i, frame_path);
            
            // Check if frame was loaded - be very careful
            if (anim_banner.num_frames > num_frames_before && 
                anim_banner.num_frames > (uint32_t)i &&
                i < MAX_BANNER_FRAMES_CHECK &&
                anim_banner.frames) {
                // Double-check bounds before accessing
                if ((uint32_t)i < anim_banner.num_frames) {
                    if (anim_banner.frames[i].pixels != 0) {
                        frames_loaded++;
                        loaded_this_frame = 1;
                        print("Banner: Frame ");
                        putchar('0' + i);
                        print(" loaded successfully\n");
                    }
                }
            }
        }
        
        // If pattern 1 failed, try pattern 2
        if (!loaded_this_frame && i < MAX_BANNER_FRAMES_CHECK && anim_banner.frames) {
            // Try pattern 2: /banner_frame_000.bin, etc.
            frame_path[0] = '/';
            frame_path[1] = 'b';
            frame_path[2] = 'a';
            frame_path[3] = 'n';
            frame_path[4] = 'n';
            frame_path[5] = 'e';
            frame_path[6] = 'r';
            frame_path[7] = '_';
            frame_path[8] = 'f';
            frame_path[9] = 'r';
            frame_path[10] = 'a';
            frame_path[11] = 'm';
            frame_path[12] = 'e';
            frame_path[13] = '_';
            frame_path[14] = '0' + (i / 100);
            frame_path[15] = '0' + ((i / 10) % 10);
            frame_path[16] = '0' + (i % 10);
            frame_path[17] = '.';
            frame_path[18] = 'b';
            frame_path[19] = 'i';
            frame_path[20] = 'n';
            frame_path[21] = '\0';
            
            // Try to load frame with second pattern
            banner_load_frame(&anim_banner, i, frame_path);
            
            // Check safely
            if (anim_banner.num_frames > num_frames_before && 
                anim_banner.num_frames > (uint32_t)i &&
                anim_banner.frames) {
                if ((uint32_t)i < anim_banner.num_frames) {
                    if (anim_banner.frames[i].pixels != 0) {
                        frames_loaded++;
                        loaded_this_frame = 1;
                    }
                }
            }
        }
        
        // If this frame failed, assume we've reached the end
        if (!loaded_this_frame) {
            break;
        }
    }
    
    // Final safety check
    if (frames_loaded == 0) {
        print_color("Banner: No frames found.\n", VGA_COLOR_LIGHT_RED);
        print("Expected: /BANNER_0.BIN, /BANNER_1.BIN, etc. or /banner_frame_000.bin, /banner_frame_001.bin, etc.\n");
        print("Make sure banner frames are in the filesystem.\n");
        banner_cleanup(&anim_banner);
        return;
    }
    
    // Double check frames are valid
    if (!anim_banner.frames || anim_banner.num_frames == 0) {
        print_color("Banner: Frames array invalid\n", VGA_COLOR_LIGHT_RED);
        banner_cleanup(&anim_banner);
        return;
    }
    
    print("Banner: Loaded ");
    // Simple integer to string for frame count
    int temp = frames_loaded;
    if (temp == 0) {
        print("0");
    } else {
        char digits[16];
        int dpos = 0;
        while (temp > 0 && dpos < 15) {
            digits[dpos++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = dpos - 1; j >= 0; j--) {
            putchar(digits[j]);
        }
    }
    print(" frames. Press any key to stop...\n");
    
    // Get current cursor position after all status messages
    int end_cursor_y = get_cursor_y();
    
    // Position banner right below where we finished printing (after status messages)
    // Add 1 line spacing, then convert to pixels (16 pixels per line)
    // This ensures banner appears below the command output, not overlapping
    banner_y = (end_cursor_y + 1) * 16;
    
    // Note: Each banner instance is independent - old banners stay on screen
    // when new ones are created, because we're drawing to different positions
    
    // Safety bounds - make sure banner fits on screen
    if (banner_y + 98 > (int)fb_height) {
        // If banner would go off screen, put it higher
        banner_y = (int)fb_height - 98 - 20;
        if (banner_y < 50) banner_y = 50;
    }
    
    // Update banner position (it was initialized with 0, now set actual position)
    anim_banner.x = banner_x;
    anim_banner.y = banner_y;
    
    // Get actual banner height from first frame (if available)
    int banner_height = 98;  // Default
    if (anim_banner.frames && anim_banner.frames[0].pixels != 0) {
        banner_height = (int)anim_banner.frames[0].height;
    }
    
    // Move cursor BELOW the banner so text doesn't overlap it
    // Convert banner bottom (pixels) to character lines (16 pixels per line)
    int banner_bottom_line = (banner_y + banner_height) / 16;
    // Add 1 line spacing below banner
    move_cursor(0, banner_bottom_line + 1);
    
    // Activate banner
    banner_set_active(&anim_banner, 1);
    
    // Simple animation loop - runs forever until keypress
    uint32_t last_frame = 0xFFFFFFFF;
    
    while (1) {  // Run forever until keypress
        // Poll keyboard for input (non-blocking) - check every iteration for responsiveness
        keyboard_poll();
        char key = keyboard_get_char();
        if (key != 0 && key != '\n' && key != '\r') {
            break;  // User pressed a key, exit animation
        }
        
        // Safety check before updating
        if (!anim_banner.frames || anim_banner.num_frames == 0) {
            break;  // Banner became invalid
        }
        
        // Update banner animation (timing is counter-based, so frequent calls are needed)
        banner_update(&anim_banner);
        
        // Only redraw if frame changed (efficiency optimization - most important!)
        if (anim_banner.current_frame != last_frame && 
            anim_banner.current_frame < anim_banner.num_frames &&
            anim_banner.frames[anim_banner.current_frame].pixels != 0) {
            banner_draw(&anim_banner);
            last_frame = anim_banner.current_frame;
        }
        
        // Very small delay to prevent CPU spinning (banner timing is counter-based, not real-time)
        banner_delay(1);  // 1ms delay - minimal CPU usage
    }
    
    // Save banner position and height before cleanup (for cursor positioning)
    int final_banner_y = anim_banner.y;
    int final_banner_height = 98;  // Default
    if (anim_banner.frames && anim_banner.frames[0].pixels != 0) {
        final_banner_height = (int)anim_banner.frames[0].height;
    }
    
    // Cleanup
    banner_cleanup(&anim_banner);
    
    // Move cursor below the banner area so prompt appears in correct place
    int final_banner_bottom_line = (final_banner_y + final_banner_height) / 16;
    move_cursor(0, final_banner_bottom_line + 1);
    
    print("Banner animation stopped.\n");
}

int shell_readline(char* buf, int maxlen) {
    int pos = 0;
    history_index = history_count; // virtual index after last entry
    while (1) {
        char c = keyboard_get_char();
        if (!c) { keyboard_poll(); continue; }
        if (c == '\n' || c == '\r') {
            if (pos < maxlen) buf[pos] = '\0'; else buf[maxlen-1] = '\0';
            putchar('\n');
            if (pos > 0) {
                if (history_count < HISTORY_MAX) {
                    int idx = history_count++;
                    int i=0; while (i < 255 && buf[i]) { history[idx][i] = buf[i]; i++; } history[idx][i] = 0;
                } else {
                    for (int r=1; r<HISTORY_MAX; r++) { int i=0; while (i<256) { history[r-1][i] = history[r][i]; i++; } }
                    int i=0; while (i < 255 && buf[i]) { history[HISTORY_MAX-1][i] = buf[i]; i++; } history[HISTORY_MAX-1][i] = 0;
                }
            }
            return pos;
        } else if (c == '\b') { // backspace
            if (pos > 0) { pos--; buf[pos] = '\0'; erase_last_char_visual(); }
        } else if ((unsigned char)c == 0x7F) { // Delete key -> erase previous char visually
            if (pos > 0) { pos--; buf[pos] = '\0'; erase_last_char_visual(); }
        } else if ((unsigned char)c == 0x80) { // UP
            if (history_count > 0 && history_index > 0) {
                history_index--;
                clear_current_line_visual(&pos);
                int i=0; while (history[history_index][i] && i < maxlen-1) { buf[i] = history[history_index][i]; putchar(buf[i]); i++; }
                buf[i] = 0; pos = i;
            }
        } else if ((unsigned char)c == 0x81) { // DOWN
            if (history_index < history_count-1) {
                history_index++;
                clear_current_line_visual(&pos);
                int i=0; while (history[history_index][i] && i < maxlen-1) { buf[i] = history[history_index][i]; putchar(buf[i]); i++; }
                buf[i] = 0; pos = i;
            } else if (history_index == history_count-1) {
                history_index = history_count;
                clear_current_line_visual(&pos);
                buf[0] = 0; pos = 0;
            }
        } else if (pos < maxlen - 1) {
            buf[pos++] = c; buf[pos] = '\0'; putchar(c);
        }
    }
} 