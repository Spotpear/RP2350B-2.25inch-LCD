# -*- coding: utf-8 -*-
"""Crop Messi Argentina photo for 76x284 LCD - vertical full body."""
from PIL import Image

TARGET_W = 76
TARGET_H = 284

img = Image.open(r'C:\Users\Administrator\Desktop\2.25\messi.jpg')
if img.mode != 'RGB':
    img = img.convert('RGB')

orig_w, orig_h = img.size
print(f'Original: {orig_w}x{orig_h}')

# Messi photo is 1080x1350 (portrait-ish), crop center to match 76:284 ratio
target_ratio = TARGET_H / TARGET_W  # 3.74
img_ratio = orig_h / orig_w

if img_ratio > target_ratio:
    # Image is taller than needed, crop height
    new_w = orig_w
    new_h = int(new_w * target_ratio)
    top = (orig_h - new_h) // 2
    crop = img.crop((0, top, new_w, top + new_h))
else:
    # Image is wider than needed, crop width
    new_h = orig_h
    new_w = int(new_h / target_ratio)
    left = (orig_w - new_w) // 2
    crop = img.crop((left, 0, left + new_w, new_h))

cw, ch = crop.size
print(f'Crop: {cw}x{ch}, ratio: {ch/cw:.2f}')

# Resize to exactly 76x284
final = crop.resize((TARGET_W, TARGET_H), Image.LANCZOS)
print(f'Final: {TARGET_W}x{TARGET_H}')

# Save preview
final.save(r'C:\Users\Administrator\Desktop\2.25\photo_preview.png')
print('Preview saved')

# Convert to RGB565 BE bytes (high byte first - matches GUI_Paint Scale 65,
# which stores Image[Addr]=Color>>8, Image[Addr+1]=Color&0xFF; LE causes 花屏)
raw_bytes = bytearray()
for y in range(TARGET_H):
    for x in range(TARGET_W):
        r, g, b = final.getpixel((x, y))
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        val = (r5 << 11) | (g6 << 5) | b5
        raw_bytes.append((val >> 8) & 0xFF)
        raw_bytes.append(val & 0xFF)

# Write C file
with open(r'C:\Users\Administrator\Desktop\2.25\examples\ImageData_225.c', 'w', encoding='utf-8') as f:
    f.write('#include "ImageData.h"\n\n')
    f.write(f'// Messi Argentina {TARGET_W}x{TARGET_H} RGB565 for 2.25 inch LCD\n')
    f.write(f'const unsigned char gImage_225[{len(raw_bytes)}] = {{\n')
    for i in range(0, len(raw_bytes), 16):
        chunk = raw_bytes[i:i+16]
        hex_vals = ','.join(f'0X{b:02X}' for b in chunk)
        if i + 16 < len(raw_bytes):
            f.write(hex_vals + ',\n')
        else:
            f.write(hex_vals + '\n')
    f.write('};\n')

print(f'Done: {len(raw_bytes)} bytes written')
