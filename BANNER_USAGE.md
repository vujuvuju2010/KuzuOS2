# Banner System Usage Guide

The banner system allows you to display animated sequences of images (like a GIF converted to frames) with proper timing.

## File Format

Banner frames are stored in a custom binary format:
- Magic: "BANN" (4 bytes, little-endian: 0x4E4E4142)
- Width: uint32_t (4 bytes)
- Height: uint32_t (4 bytes)
- Delay: uint32_t (4 bytes, milliseconds)
- Pixel data: width * height * 4 bytes (RGBA32 format, BGRA in memory)

## Converting GIF to Banner Frames

To convert a GIF to banner frames:

1. Extract frames from GIF (using ImageMagick, ffmpeg, or similar):
   ```bash
   convert input.gif -coalesce frame_%03d.png
   ```

2. Convert each PNG to the banner format (you'll need a simple converter tool):
   ```bash
   # Example Python script to convert PNG to banner format
   python3 convert_png_to_banner.py frame_000.png banner_frame_000.bin 100
   # where 100 is the delay in milliseconds
   ```

3. Store the banner files in your filesystem and load them using `banner_load_frame()`.

## Usage Example

```c
#include "banner.h"

// Initialize a banner at position (100, 50)
struct banner my_banner;
banner_init(&my_banner, 100, 50);

// Load frames (assuming files are named banner_frame_000.bin, etc.)
banner_load_frame(&my_banner, 0, "/banner_frame_000.bin");
banner_load_frame(&my_banner, 1, "/banner_frame_001.bin");
banner_load_frame(&my_banner, 2, "/banner_frame_002.bin");
// ... load all frames

// Activate the banner
banner_set_active(&my_banner, 1);

// In your main loop:
while (1) {
    // Update animation timing
    banner_update(&my_banner);
    
    // Draw the current frame
    banner_draw(&my_banner);
    
    // ... other rendering code ...
}
```

## Converting PNG to Banner Format

A simple converter tool (convert_png_to_banner.c) would need to:
1. Read PNG file (or use raw RGBA32 data)
2. Write magic "BANN"
3. Write width, height, delay
4. Write pixel data

Since full PNG parsing is complex in a kernel environment, you can:
- Use an external tool to convert PNG to raw RGBA32
- Then use `banner_load_frame_data()` to load raw pixel data directly

## Notes

- The timing system uses a simple counter approximation. For accurate timing, integrate with PIT or RTC.
- Banner frames are stored in RGBA32 format (BGRA byte order in memory).
- Transparent pixels (alpha < 128) are skipped during drawing.
- Maximum 100 frames per banner by default (can be adjusted in banner.c).

