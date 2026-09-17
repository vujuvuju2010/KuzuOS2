/*
soo okay this is the EHCI implementation for a USB
written by vujuvuju


copyright of nothing n noone lol
*/



#include <stdint.h>
#include "vga.h"        // print_color since its ring 0
#include "usb.h"        
#include "z_utils.h"     // z_printf
#define MOUSE_EVENT_BUFFER_SIZE 64

typedef struct {
    int8_t  dx;
    int8_t  dy;
    uint8_t buttons;   
} mouse_event_t;

static mouse_event_t mouse_buffer[MOUSE_EVENT_BUFFER_SIZE];
static int mouse_buffer_head = 0;
static int mouse_buffer_tail = 0;
static uint8_t mouse_last_buttons = 0;

// Kernel cursor rendering state
static int kernel_cursor_enabled = 0;
static int cursor_x = 960;
static int cursor_y = 540;
#define CURSOR_SIZE 10

// Framebuffer access (extern from kernel)
extern uint32_t* framebuffer;
extern uint32_t fb_width;
extern uint32_t fb_height;

// Saved pixels under cursor
static uint32_t saved_cursor[CURSOR_SIZE * CURSOR_SIZE];
static int cursor_saved = 0;

// last known mmouse event
static void mouse_push_event(int8_t dx, int8_t dy, uint8_t buttons)
{
    int next = (mouse_buffer_head + 1) % MOUSE_EVENT_BUFFER_SIZE;

    if(next != mouse_buffer_tail)
    {
        mouse_buffer[mouse_buffer_head].dx = dx;
        mouse_buffer[mouse_buffer_head].dy = dy;
        mouse_buffer[mouse_buffer_head].buttons = buttons;
        mouse_buffer_head = next;
    }
   }

int mouse_pop_event(mouse_event_t* out)
{
    if(mouse_buffer_tail == mouse_buffer_head)
        return 0;

    *out = mouse_buffer[mouse_buffer_tail];
    mouse_buffer_tail = (mouse_buffer_tail + 1) % MOUSE_EVENT_BUFFER_SIZE;
    return 1;
}

void usbmouse_poll_device(usb_device_t* dev)
{
    static unsigned char report[8];

    for(int i = 0; i < 8; i++)
        report[i] = 0;

    int r = ehci_interrupt_in(
        dev->addr,
        dev->endpoint_interrupt_in,
        report,
        8
    );

    if(r != 0)
        return;
    uint8_t buttons = report[0];
    int8_t  dx      = (int8_t)report[1];
    int8_t  dy      = (int8_t)report[2];

    if(dx != 0 || dy != 0 || buttons != mouse_last_buttons)
    {
        mouse_push_event(dx, dy, buttons);
        mouse_last_buttons = buttons;
    }
}

void usbmouse_attach(usb_device_t* dev)
{
    dev->driver = USB_DRIVER_MOUSE;
    print_color("its a mouseee\n", VGA_COLOR_GREEN);
}

// Kernel cursor rendering functions
static void save_cursor_pixels(void) {
    if (!framebuffer || !kernel_cursor_enabled) return;
    
    int idx = 0;
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx < CURSOR_SIZE; dx++) {
            int px = cursor_x + dx;
            int py = cursor_y + dy;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                saved_cursor[idx] = framebuffer[py * fb_width + px];
            } else {
                saved_cursor[idx] = 0;
            }
            idx++;
        }
    }
    cursor_saved = 1;
}

static void restore_cursor_pixels(void) {
    if (!framebuffer || !cursor_saved) return;
    
    int idx = 0;
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx < CURSOR_SIZE; dx++) {
            int px = cursor_x + dx;
            int py = cursor_y + dy;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                framebuffer[py * fb_width + px] = saved_cursor[idx];
            }
            idx++;
        }
    }
}

static void draw_kernel_cursor(void) {
    if (!framebuffer || !kernel_cursor_enabled) return;
    
    uint32_t red = 0xFFAA0000;  // BGR: Red cursor
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx < CURSOR_SIZE; dx++) {
            int px = cursor_x + dx;
            int py = cursor_y + dy;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                framebuffer[py * fb_width + px] = red;
            }
        }
    }
}

void mouse_update_kernel_cursor(void) {
    if (!kernel_cursor_enabled) return;
    
    mouse_event_t event;
    while (mouse_pop_event(&event)) {
        // Restore old position
        restore_cursor_pixels();
        
        // Update position
        cursor_x += event.dx;
        cursor_y += event.dy;
        
        // Clamp to screen
        if (cursor_x < 0) cursor_x = 0;
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_x > (int)fb_width - CURSOR_SIZE) 
            cursor_x = (int)fb_width - CURSOR_SIZE;
        if (cursor_y > (int)fb_height - CURSOR_SIZE) 
            cursor_y = (int)fb_height - CURSOR_SIZE;
        
        // Save and draw at new position
        save_cursor_pixels();
        draw_kernel_cursor();
    }
}

void mouse_enable_kernel_cursor(int enable) {
    if (enable && !kernel_cursor_enabled) {
        // Enabling: save initial position and draw
        kernel_cursor_enabled = 1;
        cursor_x = fb_width / 2;
        cursor_y = fb_height / 2;
        save_cursor_pixels();
        draw_kernel_cursor();
        z_printf("[MOUSE] Kernel cursor enabled at %d,%d\n", cursor_x, cursor_y);
    } else if (!enable && kernel_cursor_enabled) {
        // Disabling: restore pixels
        restore_cursor_pixels();
        kernel_cursor_enabled = 0;
        cursor_saved = 0;
        z_printf("[MOUSE] Kernel cursor disabled\n");
    }
}
