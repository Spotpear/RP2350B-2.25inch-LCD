# -*- coding: utf-8 -*-
"""Extract images from ImageData.c and convert/scale them to 76x284 RGB565."""
import re, struct, os

# Parse ImageData.c
with open(r'C:\Users\Administrator\Desktop\2.25\examples\ImageData.c', 'r') as f:
    content = f.read()

# Find all image arrays
pattern = r'const unsigned char (gImage_\w+)\[(\d+)\]\s*=\s*\{([^}]+)\}'
images = {}
for m in re.finditer(pattern, content):
    name = m.group(1)
    size = int(m.group(2))
    hex_str = m.group(3)
    # Extract all hex bytes
    hex_bytes = re.findall(r'0X([0-9A-Fa-f]{2})', hex_str)
    data = bytes(int(h, 16) for h in hex_bytes)
    images[name] = data
    print(f'{name}: {size} bytes, parsed {len(data)} bytes')

# Parse header: first 4 bytes = width(2, big-endian) + height(2, big-enden)
# Actually let's check the header format
for name, data in images.items():
    if len(data) >= 8:
        hdr = data[:8]
        # Try different header interpretations
        w_be = (hdr[0] << 8) | hdr[1]
        h_be = (hdr[2] << 8) | hdr[3]
        w_le = (hdr[1] << 8) | hdr[0]
        h_le = (hdr[3] << 8) | hdr[2]
        pixel_data = len(data) - 4  # assuming 4-byte header
        pixels = pixel_data // 2
        print(f'{name}: hdr_be W={w_be} H={h_be}, hdr_le W={w_le} H={h_le}, pixels={pixels}')
        # Check which makes sense
        if w_be * h_be == pixels:
            print(f'  -> BE header matches: {w_be}x{h_be}')
        if w_le * h_le == pixels:
            print(f'  -> LE header matches: {w_le}x{h_le}')

# Save each image as PNG for inspection
try:
    from PIL import Image
    has_pil = True
except ImportError:
    has_pil = False
    print('PIL not available, skipping PNG export')

if has_pil:
    out_dir = r'C:\Users\Administrator\Desktop\2.25\img_preview'
    os.makedirs(out_dir, exist_ok=True)
    for name, data in images.items():
        if len(data) < 6:
            continue
        # Try 4-byte header: W(2 BE) H(2 BE)
        w = (data[0] << 8) | data[1]
        h = (data[2] << 8) | data[3]
        pixels = (len(data) - 4) // 2
        if w * h != pixels:
            # Try without header (pure pixel data)
            # guess 240x240
            for gw in [240, 180, 120, 80, 76]:
                gh = pixels // gw
                if gw * gh == pixels:
                    w, h = gw, gh
                    print(f'  {name}: no header, guessing {w}x{h}')
                    break
            else:
                print(f'  {name}: cannot determine size, pixels={pixels}')
                continue
        
        pixel_data = data[4:] if w * h == (len(data)-4)//2 else data
        img = Image.frombytes('RGB', (w, h), pixel_data, 'raw', 'BGR;16')
        img.save(os.path.join(out_dir, f'{name}.png'))
        print(f'  Saved {name}.png ({w}x{h})')
