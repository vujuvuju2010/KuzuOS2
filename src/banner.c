#include "banner.h"
#include "filesystem.h"
#include "memory.h"

// Simple memory allocation for banner frames
#define MAX_BANNER_FRAMES 100
static struct banner_frame allocated_frames[MAX_BANNER_FRAMES];
static uint32_t frame_allocated_count = 0;

// Simple PNG-like format parser (raw RGBA32 data with header)
// Format: "BANN" magic (4 bytes), width (4 bytes), height (4 bytes), delay_ms (4 bytes), pixel data (width*height*4 bytes)
#define BANNER_MAGIC 0x4E4E4142  // "BANN" in little-endian

// Allocate a banner frame
static struct banner_frame* allocate_frame() {
    if (frame_allocated_count >= MAX_BANNER_FRAMES) {
        return 0;
    }
    return &allocated_frames[frame_allocated_count++];
}

// Get current time (simple counter-based)
// Note: This is a simple approximation. For accurate timing, use PIT or RTC
static uint32_t get_time_ms() {
    // Simple counter - in real implementation, use PIT or RTC
    static uint32_t time_counter = 0;
    time_counter++;
    // Rough approximation: each call increments by ~1/7 ms
    return time_counter / 7;
}

// Load a banner frame from file
// File format: "BANN" magic, width (4 bytes), height (4 bytes), delay_ms (4 bytes), pixel data
void banner_load_frame(struct banner* b, uint32_t frame_index, const char* filename) {
    if (!b || frame_index >= MAX_BANNER_FRAMES) return;
    
    // Read header first (16 bytes) to check dimensions before allocating
    char header[16];
    int header_size = fs_read_file((char*)filename, header, 16);
    
    if (header_size < 16) return; // Too small to be valid or file not found
    
    // Check magic
    uint32_t magic = *(uint32_t*)header;
    if (magic != BANNER_MAGIC) return;
    
    // Read header values
    uint32_t width = *(uint32_t*)(header + 4);
    uint32_t height = *(uint32_t*)(header + 8);
    uint32_t delay_ms = *(uint32_t*)(header + 12);
    
    // Limit dimensions for low-end systems (max 640x480 to support larger banners)
    if (width > 640 || height > 480) return;
    
    // Check if frame would be too large (max 1MB to prevent OOM)
    uint32_t pixel_count = width * height;
    uint32_t expected_size = 16 + (pixel_count * 4);
    if (expected_size > 1048576) return; // 1MB limit
    
    // Allocate frame structure first
    if (frame_index >= b->num_frames) {
        b->num_frames = frame_index + 1;
    }
    
    if (!b->frames) {
        // Allocate frames array
        b->frames = allocated_frames;
    }
    
    struct banner_frame* frame = &b->frames[frame_index];
    frame->width = width;
    frame->height = height;
    frame->delay_ms = delay_ms;
    
    // Allocate pixel data first (before reading to avoid OOM)
    frame->pixels = (uint32_t*)kmalloc(pixel_count * sizeof(uint32_t));
    
    if (!frame->pixels) return;
    
    // Allocate a buffer to read the file (on heap to avoid stack overflow)
    char* file_buffer = (char*)kmalloc(expected_size);
    if (!file_buffer) {
        kfree(frame->pixels);
        frame->pixels = 0;
        return;
    }
    
    // Read the entire file
    int size = fs_read_file((char*)filename, file_buffer, expected_size);
    
    if (size < (int)expected_size) {
        // Failed to read - cleanup
        kfree(file_buffer);
        kfree(frame->pixels);
        frame->pixels = 0;
        return;
    }
    
    // Copy pixel data (skip 16-byte header)
    uint32_t* src_pixels = (uint32_t*)(file_buffer + 16);
    uint32_t* dst_pixels = frame->pixels;
    // Use word-sized copies for speed
    for (uint32_t i = 0; i < pixel_count; i++) {
        dst_pixels[i] = src_pixels[i];
    }
    
    // Free the temporary buffer
    kfree(file_buffer);
}

// Load frame data directly (for programmatic use)
void banner_load_frame_data(struct banner* b, uint32_t frame_index, uint32_t width, uint32_t height, uint32_t* pixels, uint32_t delay_ms) {
    if (!b || !pixels || frame_index >= MAX_BANNER_FRAMES) return;
    
    if (frame_index >= b->num_frames) {
        b->num_frames = frame_index + 1;
    }
    
    if (!b->frames) {
        b->frames = allocated_frames;
    }
    
    struct banner_frame* frame = &b->frames[frame_index];
    frame->width = width;
    frame->height = height;
    frame->delay_ms = delay_ms;
    
    // Allocate and copy pixel data
    uint32_t pixel_count = width * height;
    frame->pixels = (uint32_t*)kmalloc(pixel_count * sizeof(uint32_t));
    
    if (!frame->pixels) return;
    
    for (uint32_t i = 0; i < pixel_count; i++) {
        frame->pixels[i] = pixels[i];
    }
}

// Initialize banner
void banner_init(struct banner* b, int x, int y) {
    if (!b) return;
    
    b->frames = allocated_frames;
    b->num_frames = 0;
    b->current_frame = 0;
    b->last_frame_time = 0;
    b->x = x;
    b->y = y;
    b->active = 0;
}

// Update banner animation (call this periodically)
void banner_update(struct banner* b) {
    if (!b || !b->active || b->num_frames == 0) return;
    
    uint32_t current_time = get_time_ms();
    
    // Check if it's time to advance to next frame
    if (b->current_frame < b->num_frames) {
        struct banner_frame* frame = &b->frames[b->current_frame];
        
        if (current_time - b->last_frame_time >= frame->delay_ms) {
            b->current_frame = (b->current_frame + 1) % b->num_frames;
            b->last_frame_time = current_time;
        }
    }
}

// Draw current banner frame
void banner_draw(struct banner* b) {
    if (!b || !b->active || b->num_frames == 0) return;
    
    if (b->current_frame >= b->num_frames) {
        b->current_frame = 0;
    }
    
    struct banner_frame* frame = &b->frames[b->current_frame];
    
    if (frame->pixels) {
        vga_draw_bitmap(b->x, b->y, frame->width, frame->height, frame->pixels);
    }
}

// Cleanup banner resources
void banner_cleanup(struct banner* b) {
    if (!b) return;
    
    // Free pixel data
    for (uint32_t i = 0; i < b->num_frames; i++) {
        if (b->frames[i].pixels) {
            kfree(b->frames[i].pixels);
            b->frames[i].pixels = 0;
        }
    }
    
    b->num_frames = 0;
    b->current_frame = 0;
    b->active = 0;
}

// Set banner active state
void banner_set_active(struct banner* b, int active) {
    if (!b) return;
    b->active = active;
    if (active) {
        b->last_frame_time = get_time_ms();
    }
}
