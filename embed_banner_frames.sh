#!/bin/bash
# Embed banner frames into the ISO filesystem

BANNER_DIR="banner_frames"
ISO_DIR="iso"

# Copy banner frames to ISO directory
mkdir -p "$ISO_DIR/banner_frames"
cp "$BANNER_DIR"/*.bin "$ISO_DIR/banner_frames/" 2>/dev/null

# Also copy to root of ISO
cp "$BANNER_DIR"/*.bin "$ISO_DIR/" 2>/dev/null

echo "Banner frames embedded into ISO directory"

