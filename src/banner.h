#ifndef BANNER_H
#define BANNER_H

#include "vga.h"

// Banner frame structure
struct banner_frame {
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;  // RGBA32 pixel data
    uint32_t delay_ms; // Frame delay in milliseconds
};

// Banner animation structure
struct banner {
    struct banner_frame* frames;
    uint32_t num_frames;
    uint32_t current_frame;
    uint32_t last_frame_time;
    int x;  // Position on screen
    int y;
    int active;  // 1 if banner is active, 0 if not
};

// Banner functions
void banner_init(struct banner* b, int x, int y);
void banner_load_frame(struct banner* b, uint32_t frame_index, const char* filename);
void banner_load_frame_data(struct banner* b, uint32_t frame_index, uint32_t width, uint32_t height, uint32_t* pixels, uint32_t delay_ms);
void banner_update(struct banner* b);
void banner_draw(struct banner* b);
void banner_cleanup(struct banner* b);
void banner_set_active(struct banner* b, int active);

// Helper function to draw a bitmap at a position
void vga_draw_bitmap(int x, int y, uint32_t width, uint32_t height, uint32_t* pixels);

#endif
