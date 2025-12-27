#include "keyboard.h"
#include "vga.h"

#define KEYBOARD_DATA_PORT 0x60

// Turkish Q Layout scancodes'a göre mapping
static const char scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 0, 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

// Shift ile semboller
static const char shift_chars[] = "!@#$%^&*()";

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;
static int shift = 0;
static int caps_lock = 0;
static int e0_prefix = 0;

void keyboard_init() {
    buffer_head = 0;
    buffer_tail = 0;
    shift = 0;
    caps_lock = 0;
    e0_prefix = 0;
    
    // PIC'te IRQ1'i devre dışı bırak - polling kullanacağız
    uint8_t mask;
    __asm__ volatile("inb $0x21, %0" : "=a"(mask));
    mask |= 0x02; // IRQ1'i devre dışı bırak
    __asm__ volatile("outb %0, $0x21" : : "a"(mask));
}

void keyboard_handler() {
    // why 
    return;
}

// Polling ile keyboard oku
void keyboard_poll() {
    uint8_t status;
    __asm__ volatile("inb $0x64, %0" : "=a"(status));
    
    if (status & 0x01) { // Data available
        uint8_t scancode;
        __asm__ volatile("inb $0x60, %0" : "=a"(scancode));
        
        // Handle E0 prefix for extended keys
        if (scancode == 0xE0) { e0_prefix = 1; return; }
        
        // Key press/release handling
        if (scancode < 0x80) {
            // Key press
            if (e0_prefix) {
                // Extended key press
                e0_prefix = 0;
                char out = 0;
                if (scancode == 0x48) { // Up arrow
                    out = (char)0x80;
                } else if (scancode == 0x50) { // Down arrow
                    out = (char)0x81;
                } else if (scancode == 0x53) { // Delete
                    out = (char)0x7F;
                }
                if (out) {
                    int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
                    if (next_head != buffer_tail) { keyboard_buffer[buffer_head] = out; buffer_head = next_head; }
                }
                return;
            }
            switch (scancode) {
                case 0x2A: // Left Shift
                case 0x36: // Right Shift
                    shift = 1;
                    break;
                case 0x3A: // Caps Lock
                    caps_lock = !caps_lock;
                    break;
                case 0x0E: // Backspace (BACK key)
                {
                    int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
                    if (next_head != buffer_tail) { keyboard_buffer[buffer_head] = '\b'; buffer_head = next_head; }
                    break;
                }
                case 0x66: // Alternative backspace
                {
                    int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
                    if (next_head != buffer_tail) { keyboard_buffer[buffer_head] = '\b'; buffer_head = next_head; }
                    break;
                }
                default:
                {
                    char c = scancode_to_ascii[scancode];
                    if (c) {
                        // Shift ve Caps Lock handling
                        if ((shift != caps_lock) && c >= 'a' && c <= 'z') { c = c - 'a' + 'A'; }
                        // Shift ile sayı sembolleri
                        if (shift) {
                            if (c == '1') c = '!';
                            else if (c == '2') c = '@';
                            else if (c == '3') c = '#';
                            else if (c == '4') c = '$';
                            else if (c == '5') c = '%';
                            else if (c == '6') c = '^';
                            else if (c == '7') c = '&';
                            else if (c == '8') c = '*';
                            else if (c == '9') c = '(';
                            else if (c == '0') c = ')';
                        }
                        int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
                        if (next_head != buffer_tail) { keyboard_buffer[buffer_head] = c; buffer_head = next_head; }
                    }
                    break;
                }
            }
        } else {
            // Key release
            uint8_t release_scancode = scancode - 0x80;
            if (e0_prefix) { e0_prefix = 0; return; }
            switch (release_scancode) {
                case 0x2A: // Left Shift
                case 0x36: // Right Shift
                    shift = 0;
                    break;
            }
        }
    }
}

char keyboard_get_char() {
    if (buffer_head == buffer_tail) return 0;
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
} 
