#include "keyboard.h"
#include "vga.h"
#include "keymap_loader.h"

char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
int buffer_head = 0;
int buffer_tail = 0;

static int shift = 0;
static int ctrl = 0;
static int alt = 0;
static int altgr = 0;
static int caps_lock = 0;
static int e0_prefix = 0;

/* PS/2 scancode set 1 -> Linux keycode table */
static const uint8_t set1_keycode[128] = {
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
     64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
     80, 81, 82, 83, 99, 0, 86, 87, 88, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Extended scancode (E0 prefix) mappings */
static const uint8_t set1_keycode_e0[128] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 97, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 55, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 71, 72, 73, 0, 75, 0, 77, 0, 79,
     80, 81, 82, 83, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int is_letter_keycode(uint8_t kc) {
    return (kc >= 16 && kc <= 25) || (kc >= 30 && kc <= 38) || (kc >= 44 && kc <= 50);
}

static void push_char(char c) {
    if (!c) return;
    int next = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != buffer_tail) {
        keyboard_buffer[buffer_head] = c;
        buffer_head = next;
    }
}

static void handle_modifier(uint8_t kc, int pressed) {
    switch (kc) {
        case 42:
        case 54:
            shift = pressed;
            break;
        case 29:
        case 97:
            ctrl = pressed;
            break;
        case 56:
            alt = pressed;
            break;
        case 100:
            altgr = pressed;
            break;
        case 58:
            if (pressed) caps_lock = !caps_lock;
            break;
        default:
            break;
    }
}

static int is_modifier_keycode(uint8_t kc) {
    return kc == 42 || kc == 54 || kc == 29 || kc == 97 || kc == 56 || kc == 100 || kc == 58;
}

void keyboard_init() {
    buffer_head = 0;
    buffer_tail = 0;
    shift = 0;
    ctrl = 0;
    alt = 0;
    altgr = 0;
    caps_lock = 0;
    e0_prefix = 0;

    // Flush keyboard buffer
    for (int i = 0; i < 10; i++) {
        uint8_t status;
        __asm__ volatile("inb $0x64, %0" : "=a"(status));
        if (!(status & 0x01)) break;
        uint8_t dummy;
        __asm__ volatile("inb $0x60, %0" : "=a"(dummy));
    }

    uint8_t mask;
    __asm__ volatile("inb $0x21, %0" : "=a"(mask));
    mask |= 0x02;
    __asm__ volatile("outb %0, $0x21" : : "a"(mask));
}

void keyboard_handler() {
    return;
}

void keyboard_poll() {
    uint8_t status;
    __asm__ volatile("inb $0x64, %0" : "=a"(status));

    if (!(status & 0x01))
        return;

    uint8_t scancode;
    __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

    if (scancode == 0xE0) {
        e0_prefix = 1;
        return;
    }

    if (scancode == 0xE1) {
        // Ignore E1 (Pause key)
        e0_prefix = 0;
        return;
    }

    int release = (scancode & 0x80) != 0;
    uint8_t code = scancode & 0x7F;

    uint8_t keycode;
    if (e0_prefix) {
        keycode = set1_keycode_e0[code];
    } else {
        keycode = set1_keycode[code];
    }
    e0_prefix = 0;

    if (keycode == 0)
        return;

    if (is_modifier_keycode(keycode)) {
        handle_modifier(keycode, !release);
        return;
    }

    if (release)
        return;

    // Apply caps lock only to letters, not to shift state for keymap
    int use_shift = shift;
    if (caps_lock && is_letter_keycode(keycode))
        use_shift = !use_shift;

    uint16_t c = keymap_get_char(keycode, use_shift, ctrl, alt, altgr);
    if (c > 0 && c < 256)
        push_char((char)c);
}

char keyboard_get_char() {
    if (buffer_head == buffer_tail) return 0;
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
