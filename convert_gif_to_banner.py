#!/usr/bin/env python3
"""
Convert GIF to banner frames
Extracts frames from GIF and converts them to the banner format
"""

import sys
from PIL import Image

def convert_gif_to_banner(gif_path, output_dir="banner_frames", delay_ms=100):
    """
    Convert GIF to banner frames
    Args:
        gif_path: Path to input GIF file
        output_dir: Directory to save banner frame files
        delay_ms: Default delay in milliseconds (if not specified in GIF)
    """
    import os
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Open GIF
    try:
        gif = Image.open(gif_path)
    except Exception as e:
        print(f"Error opening GIF: {e}")
        return False
    
    frame_count = 0
    
    # Process each frame
    try:
        while True:
            # Get frame duration (convert to milliseconds)
            try:
                frame_delay = gif.info.get('duration', delay_ms)
            except:
                frame_delay = delay_ms
            
            # Convert to RGBA
            frame = gif.convert('RGBA')
            width, height = frame.size
            
            # Get pixel data
            pixels = list(frame.getdata())
            
            # Convert to BGRA format (for banner)
            bgra_pixels = []
            for pixel in pixels:
                r, g, b, a = pixel
                # Convert to BGRA (little-endian uint32_t)
                bgra = (a << 24) | (r << 16) | (g << 8) | b
                bgra_pixels.append(bgra)
            
            # Create banner file
            output_path = os.path.join(output_dir, f"banner_frame_{frame_count:03d}.bin")
            
            with open(output_path, 'wb') as f:
                # Write magic "BANN"
                f.write(b'BANN')
                
                # Write width (little-endian uint32_t)
                f.write(width.to_bytes(4, 'little'))
                
                # Write height (little-endian uint32_t)
                f.write(height.to_bytes(4, 'little'))
                
                # Write delay (little-endian uint32_t)
                f.write(frame_delay.to_bytes(4, 'little'))
                
                # Write pixel data
                for pixel in bgra_pixels:
                    f.write(pixel.to_bytes(4, 'little'))
            
            print(f"Converted frame {frame_count}: {width}x{height}, delay={frame_delay}ms -> {output_path}")
            frame_count += 1
            
            # Move to next frame
            try:
                gif.seek(gif.tell() + 1)
            except EOFError:
                break
                
    except Exception as e:
        print(f"Error processing frames: {e}")
        return False
    
    print(f"\nConverted {frame_count} frames to {output_dir}/")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 convert_gif_to_banner.py <gif_file> [output_dir] [delay_ms]")
        print("Example: python3 convert_gif_to_banner.py cooltext485324202115251.gif banner_frames 100")
        sys.exit(1)
    
    gif_path = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "banner_frames"
    delay_ms = int(sys.argv[3]) if len(sys.argv) > 3 else 100
    
    convert_gif_to_banner(gif_path, output_dir, delay_ms)

