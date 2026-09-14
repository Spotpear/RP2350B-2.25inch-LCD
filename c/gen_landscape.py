# -*- coding: utf-8 -*-
"""Generate a 76x284 RGB565 landscape image C array for the 2.25 inch LCD."""
import struct, math

WIDTH = 76
HEIGHT = 284

def rgb565(r, g, b):
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

pixels = []
for y in range(HEIGHT):
    for x in range(WIDTH):
        # Sky gradient: deep purple top -> orange -> golden near horizon
        # Sea: blue-teal
        # Sun: bright yellow circle
        horizon = int(HEIGHT * 0.62)  # horizon line at 62%
        
        if y < horizon:
            # Sky
            t = y / horizon  # 0=top, 1=horizon
            if t < 0.3:
                # Deep purple to magenta
                tt = t / 0.3
                r = int(40 + tt * 140)
                g = int(20 + tt * 40)
                b = int(80 + tt * 60)
            elif t < 0.65:
                # Magenta to orange
                tt = (t - 0.3) / 0.35
                r = int(180 + tt * 60)
                g = int(60 + tt * 100)
                b = int(140 - tt * 100)
            else:
                # Orange to golden yellow
                tt = (t - 0.65) / 0.35
                r = int(240 + tt * 15)
                g = int(160 + tt * 70)
                b = int(40 + tt * 30)
            
            # Sun: circle centered at (38, horizon-40) radius 12
            sun_cx = 38
            sun_cy = horizon - 35
            sun_r = 14
            dx = x - sun_cx
            dy = y - sun_cy
            dist = math.sqrt(dx*dx + dy*dy)
            if dist < sun_r:
                # Sun core: bright white-yellow
                r = 255
                g = 240
                b = 180
            elif dist < sun_r + 8:
                # Sun glow
                glow = 1.0 - (dist - sun_r) / 8.0
                r = int(min(255, r + glow * 40))
                g = int(min(255, g + glow * 60))
                b = int(min(255, b + glow * 40))
        else:
            # Sea/reflection
            t = (y - horizon) / (HEIGHT - horizon)  # 0=horizon, 1=bottom
            
            # Reflect sun color in water
            sun_cx = 38
            sun_reflect_y = horizon + (horizon - (horizon - 35))  # mirror
            dx = x - sun_cx
            # Reflection band: widens as it goes down
            reflect_half_width = 3 + t * 20
            if abs(dx) < reflect_half_width:
                # Sun reflection on water
                reflect_strength = (1.0 - t) * (1.0 - abs(dx) / reflect_half_width)
                base_r = int(200 + t * 30)
                base_g = int(150 + t * 20)
                base_b = int(80 + t * 40)
                r = int(min(255, base_r + reflect_strength * 55))
                g = int(min(255, base_g + reflect_strength * 80))
                b = int(min(255, base_b + reflect_strength * 60))
            else:
                # Water gradient: teal to dark blue
                r = int(30 + (1-t) * 40)
                g = int(60 + (1-t) * 60)
                b = int(100 + (1-t) * 50)
        
        pixels.append(struct.pack('>H', rgb565(r, g, b)))

# Write as C array
with open(r'C:\Users\Administrator\Desktop\2.25\examples\ImageData_225.c', 'w', encoding='utf-8') as f:
    f.write('#include "ImageData.h"\n\n')
    f.write(f'// 76x284 RGB565 landscape (sunset over sea)\n')
    f.write(f'const unsigned char gImage_225[{WIDTH * HEIGHT * 2}] = {{\n')
    
    raw = b''.join(pixels)
    bytes_per_line = 16
    
    for i in range(0, len(raw), 2):
        if i > 0:
            if (i % bytes_per_line) == 0:
                f.write(',\n')
            else:
                f.write(',')
        f.write('0X{:02X},0X{:02X}'.format(raw[i], raw[i+1]))
    
    f.write('\n};\n')

print(f'Generated: {WIDTH}x{HEIGHT} = {WIDTH*HEIGHT} pixels = {WIDTH*HEIGHT*2} bytes')
