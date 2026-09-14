# -*- coding: utf-8 -*-
"""Generate a 76x284 RGB565 image C array for the 2.25 inch LCD."""
import struct

WIDTH = 76
HEIGHT = 284

# Generate a colorful gradient image
pixels = []
for y in range(HEIGHT):
    for x in range(WIDTH):
        r = (x * 255) // (WIDTH - 1) if WIDTH > 1 else 0
        g = (y * 255) // (HEIGHT - 1) if HEIGHT > 1 else 0
        b = ((x + y) * 255) // (WIDTH + HEIGHT - 2) if (WIDTH + HEIGHT) > 2 else 0
        # RGB565: 5 bits R, 6 bits G, 5 bits B
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        pixels.append(struct.pack('>H', rgb565))

# Write as C array
header = f'const unsigned char gImage_225[{WIDTH * HEIGHT * 2}] = {{'
with open(r'C:\Users\Administrator\Desktop\2.25\examples\ImageData_225.c', 'w', encoding='utf-8') as f:
    f.write('#include "ImageData.h"\n\n')
    f.write(f'// 76x284 RGB565 image for 2.25 inch LCD\n')
    f.write(header)
    
    raw = b''.join(pixels)
    bytes_per_line = 16
    
    f.write('0X{:02X},0X{:02X},'.format(raw[0], raw[1]))
    
    for i in range(2, len(raw), 2):
        if i > 2:
            f.write(',')
        if (i % bytes_per_line) == 0:
            f.write('\n')
        f.write('0X{:02X},0X{:02X}'.format(raw[i], raw[i+1]))
    
    f.write('\n};\n')

print(f'Generated: {WIDTH}x{HEIGHT} = {WIDTH*HEIGHT} pixels = {WIDTH*HEIGHT*2} bytes')
