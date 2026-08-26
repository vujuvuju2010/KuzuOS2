#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define KEYBOARD_BUFFER_SIZE 256

void keyboard_init();
void keyboard_handler();
void keyboard_poll();
char keyboard_get_char();

extern volatile int ctrl_z_pressed;
void poll_input_devices(void);

#endif 