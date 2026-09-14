import struct

elf_path = r'C:\Users\Administrator\Desktop\2.25\build\main.elf'
uf2_path = r'C:\Users\Administrator\Desktop\2.25\build\main.uf2'

with open(elf_path, 'rb') as f:
    d = f.read()

addr = 0x10000000
fam = 0xe48bff56  # RP2350
num = (len(d) + 255) // 256
chunks = []
for i in range(num):
    chunk = d[i*256:(i+1)*256]
    pad = 256 - len(chunk)
    # UF2 block: 32B header + 256B payload + 220B padding + 4B magicEnd = 512
    block = struct.pack('<IIIIIIII', 0x0A324655, 0x9E5D5157, addr + i*256, 256, i, num, fam, 0)
    block += chunk + b'\x00' * pad   # 256 bytes payload (padded)
    block += b'\x00' * 220            # 220 bytes padding
    block += struct.pack('<I', 0x0A324655)  # 4 bytes magicEnd
    assert len(block) == 512, f"len={len(block)}"
    chunks.append(block)

with open(uf2_path, 'wb') as f:
    for c in chunks:
        f.write(c)

print(f'UF2: {512 * len(chunks)} bytes, {len(chunks)} blocks')
