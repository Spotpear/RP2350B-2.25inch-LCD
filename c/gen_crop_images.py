# -*- coding: utf-8 -*-
"""Crop (not scale) images from ImageData.c to 76x284 RGB565 for 2.25 inch LCD.
If source is smaller than target in some dimension, use the source size."""
import re, os
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

img_info = {
    'gImage_13': (240, 240),
    'gImage_14': (180, 240),
}

TARGET_W = 76
TARGET_H = 284

out_dir = r'C:\Users\Administrator\Desktop\2.25\examples'
scaled_images = []

for name, (orig_w, orig_h) in img_info.items():
    data = images[name]
    pixel_data = data[8:]  # skip 8-byte header
    
    # Create PIL image from RGB565 LE data
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
    
    # Crop from center, take 76 wide x min(240, 284) tall
    crop_w = min(TARGET_W, orig_w)
    crop_h = min(TARGET_H, orig_h)
    left = (orig_w - crop_w) // 2
    top = (orig_h - crop_h) // 2
    cropped = img.crop((left, top, left + crop_w, top + crop_h))
    
    # Save preview
    cropped.save(os.path.join(r'C:\Users\Administrator\Desktop\2.25', f'{name}_cropped.png'))
    print(f'{name}: {orig_w}x{orig_h} -> crop {crop_w}x{crop_h} from ({left},{top})')
    
    # Convert cropped to RGB565 LE bytes
    raw_bytes = bytearray()
    for y in range(crop_h):
        for x in range(crop_w):
            r, g, b = cropped.getpixel((x, y))
            r5 = (r >> 3) & 0x1F
            g6 = (g >> 2) & 0x3F
            b5 = (b >> 3) & 0x1F
            val = (r5 << 11) | (g6 << 5) | b5
            raw_bytes.append(val & 0xFF)
            raw_bytes.append((val >> 8) & 0xFF)
    
    scaled_images.append((name, raw_bytes, crop_w, crop_h))

# Write C file
with open(os.path.join(out_dir, 'ImageData_225.c'), 'w', encoding='utf-8') as f:
    f.write('#include "ImageData.h"\n\n')
    f.write(f'// Cropped to 76x{min(240, TARGET_H)} RGB565 for 2.25 inch LCD\n\n')
    
    for name, raw, w, h in scaled_images:
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

print(f'\nGenerated {len(scaled_images)} cropped images')
for name, raw, w, h in scaled_images:
    print(f'  {name}: {w}x{h} = {len(raw)} bytes')
