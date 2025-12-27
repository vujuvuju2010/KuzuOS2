#ifndef VGA_H
#define VGA_H

// Kendi typedef'lerimiz
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// VGA buffer
#define VGA_BUFFER 0xB8000

// Renk sabitleri
#define VGA_COLOR_BLACK         0x0
#define VGA_COLOR_BLUE          0x1
#define VGA_COLOR_GREEN         0x2
#define VGA_COLOR_CYAN          0x3
#define VGA_COLOR_RED           0x4
#define VGA_COLOR_MAGENTA       0x5
#define VGA_COLOR_BROWN         0x6
#define VGA_COLOR_LIGHT_GREY    0x7
#define VGA_COLOR_DARK_GREY     0x8
#define VGA_COLOR_LIGHT_BLUE    0x9
#define VGA_COLOR_LIGHT_GREEN   0xA
#define VGA_COLOR_LIGHT_CYAN    0xB
#define VGA_COLOR_LIGHT_RED     0xC
#define VGA_COLOR_LIGHT_MAGENTA 0xD
#define VGA_COLOR_YELLOW        0xE
#define VGA_COLOR_WHITE         0xF

// VGA fonksiyonları
void vga_init(uint32_t mb_magic, uint32_t mb_addr);
void putchar(char c);
void print(const char* str);
void clear_screen();
void putchar_color(char c, uint8_t color);
void print_color(const char* str, uint8_t color);
void scroll();
void move_cursor(int x, int y);
int get_cursor_x();
int get_cursor_y();
void vga_draw_bitmap(int x, int y, uint32_t width, uint32_t height, uint32_t* pixels);

#endif 