# -*- coding: utf-8 -*-
"""Convert existing images in ImageData.c to 76x284 RGB565 C arrays for cycling display."""
import re, struct, os
from PIL import Image

# Parse ImageData.c
with open(r'C:\Users\Administrator\Desktop\2.25\examples\ImageData.c', 'r') as f:
    content = f.read()

pattern = r'const unsigned char (gImage_\w+)\[(\d+)\]\s*=\s*\{([^}]+)\}'
images = {}
for m in re.finditer(pattern, content):
    name = m.group(1)
    hex_bytes = re.findall(r'0X([0-9A-Fa-f]{2})', m.group(3))
    data = bytes(int(h, 16) for h in hex_bytes)
    images[name] = data

# Image headers: 8 bytes [cmd1, cmd2, W_hi, W_lo, H_hi, H_lo, ?, ?]
img_info = {
    'gImage_11': (240, 64),
    'gImage_13': (240, 240),
    'gImage_14': (180, 240),
}

TARGET_W = 76
TARGET_H = 284

out_dir = r'C:\Users\Administrator\Desktop\2.25\examples'

# Generate scaled C arrays
scaled_images = []
for name, (orig_w, orig_h) in img_info.items():
    data = images[name]
    pixel_data = data[8:]  # skip 8-byte header
    # Create PIL image from RGB565 data (little-endian: lo, hi)
    # Convert RGB565 LE to RGB888
    pixels = []
    for i in range(0, len(pixel_data), 2):
        lo = pixel_data[i]
        hi = pixel_data[i+1]
        val = (hi << 8) | lo
        r5 = (val >> 11) & 0x1F
        g6 = (val >> 5) & 0x3F
        b5 = val & 0x1F
        r = (r5 << 3) | (r5 >> 2)
        g = (g6 << 2) | (g6 >> 4)
        b = (b5 << 3) | (b5 >> 2)
        pixels.append((r, g, b))
    
    img = Image.new('RGB', (orig_w, orig_h))
    img.putdata(pixels)
    
    # Scale to target size
    scaled = img.resize((TARGET_W, TARGET_H), Image.LANCZOS)
    
    # Save preview PNG
    preview_path = os.path.join(r'C:\Users\Administrator\Desktop\2.25', f'{name}_scaled.png')
    scaled.save(preview_path)
    print(f'{name}: {orig_w}x{orig_h} -> {TARGET_W}x{TARGET_H}, preview saved')
    
    # Convert to RGB565 LE bytes
    raw_bytes = bytearray()
    for y in range(TARGET_H):
        for x in range(TARGET_W):
            r, g, b = scaled.getpixel((x, y))
            r5 = (r >> 3) & 0x1F
            g6 = (g >> 2) & 0x3F
            b5 = (b >> 3) & 0x1F
            val = (r5 << 11) | (g6 << 5) | b5
            raw_bytes.append(val & 0xFF)       # low byte first (LE)
            raw_bytes.append((val >> 8) & 0xFF) # high byte
    
    scaled_images.append((name, raw_bytes))

# Write combined C file with all scaled images
with open(os.path.join(out_dir, 'ImageData_225.c'), 'w', encoding='utf-8') as f:
    f.write('#include "ImageData.h"\n\n')
    f.write(f'// Scaled to {TARGET_W}x{TARGET_H} RGB565 for 2.25 inch LCD\n\n')
    
    for name, raw in scaled_images:
        var_name = f'gImage_225_{name}'
        f.write(f'const unsigned char {var_name}[{len(raw)}] = {{\n')
        for i in range(0, len(raw), 16):
            chunk = raw[i:i+16]
            hex_vals = ','.join(f'0X{b:02X}' for b in chunk)
            if i + 16 < len(raw):
                f.write(hex_vals + ',\n')
            else:
                f.write(hex_vals + '\n')
        f.write('};\n\n')

print(f'\nGenerated {len(scaled_images)} images, each {TARGET_W*TARGET_H*2} bytes')
