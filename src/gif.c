// GIF Player for KuzuOS - plays banner frames in VESA graphics mode
#include "z_syscalls.h"
#include <stddef.h>

int main(int argc, char* argv[]);

// VESA framebuffer parameters
#define FB_WIDTH 1920
#define FB_HEIGHT 1080

// Magic number for banner format
#define BANNER_MAGIC 0x4E4E4142  // "BANN"

// Draw a pixel via syscall
static void putpixel(int x, int y, uint32_t color) {
    z_draw_pixel(x, y, color);
}

// Load and display a banner frame
static int load_and_display_frame(const char* filename) {
    z_write(1, "Loading: ", 9);
    z_write(1, filename, 20);
    z_write(1, "\n", 1);
    
    char header[16];
    int fd = z_open(filename, 0);
    if (fd < 0) {
        z_write(1, "Error: open failed\n", 19);
        return -1;
    }
    
    z_write(1, "File opened\n", 12);
    
    int header_size = z_read(fd, header, 16);
    if (header_size < 16) {
        z_close(fd);
        z_write(1, "Error: read header failed\n", 26);
        return -1;
    }
    
    z_write(1, "Header read\n", 12);
    
    // Parse header
    uint32_t magic = *(uint32_t*)header;
    uint32_t width = *(uint32_t*)(header + 4);
    uint32_t height = *(uint32_t*)(header + 8);
    
    if (magic != BANNER_MAGIC) {
        z_close(fd);
        z_write(1, "Error: bad magic\n", 17);
        return -1;
    }
    
    if (width > FB_WIDTH || height > FB_HEIGHT) {
        z_close(fd);
        z_write(1, "Error: frame too large\n", 23);
        return -1;
    }
    
    z_write(1, "Frame: ", 7);
    char buf[16];
    buf[0] = '0' + (width / 100);
    buf[1] = '0' + ((width / 10) % 10);
    buf[2] = '0' + (width % 10);
    buf[3] = 'x';
    buf[4] = '0' + (height / 100);
    buf[5] = '0' + ((height / 10) % 10);
    buf[6] = '0' + (height % 10);
    buf[7] = '\n';
    z_write(1, buf, 8);
    
    // Read pixel data into buffer
    uint32_t pixel_count = width * height;
    uint32_t pixels_size = pixel_count * 4;
    
    // Simple stack-based buffer for small frames
    char pixels_buf[256 * 256 * 4];  // Max 256x256 = 262144 bytes
    if (pixels_size > (int)sizeof(pixels_buf)) {
        z_close(fd);
        z_write(1, "Error: frame too big\n", 21);
        return -1;
    }
    
    int pixels_read = z_read(fd, pixels_buf, pixels_size);
    z_close(fd);
    
    if (pixels_read != (int)pixels_size) {
        z_write(1, "Error: pixel read mismatch\n", 27);
        return -1;
    }
    
    z_write(1, "Drawing pixels\n", 15);
    
    // Display the frame in the center of screen
    int start_x = (FB_WIDTH - width) / 2;
    int start_y = (FB_HEIGHT - height) / 2;
    
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    
    // Draw pixels
    uint32_t* pixels = (uint32_t*)pixels_buf;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t color = pixels[y * width + x];
            putpixel(start_x + x, start_y + y, color);
        }
    }
    
    z_write(1, "Done\n", 5);
    return 0;
}

// Entry point
void _start(void) {
    int argc = 0;
    char* argv[] = { NULL };
    int result = main(argc, argv);
    z_exit(result);
}

// Main entry point
int main(int argc, char* argv[]) {
    // Display the KuzuOS banner frames
    z_write(1, "Attempting to load banner frame...\n", 35);
    load_and_display_frame("/dev/banner_frame_000.bin");
    
    return 0;
}
