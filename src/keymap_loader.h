#ifndef KEYMAP_LOADER_H
#define KEYMAP_LOADER_H

#include <stdint.h>

#define MAX_KEYCODE 256
#define MAX_KEYMAP_FN 256

typedef struct {
    uint16_t fn_map[MAX_KEYMAP_FN][MAX_KEYCODE];
} keymap_t;

extern keymap_t current_keymap;

int keymap_load(const char* path);
uint16_t keymap_get_char(uint8_t keycode, uint8_t shift, uint8_t ctrl, uint8_t alt, uint8_t altgr);
void load_us_keymap(void);

#endif
