# -*- coding: utf-8 -*-
import sys
import io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

import pdfplumber

print('=== ST7789P3 ===')
with pdfplumber.open(r'C:\Users\Administrator\Desktop\ST7789P3.pdf') as pdf:
    for i, page in enumerate(pdf.pages):
        text = page.extract_text()
        if text:
            print(f'--- Page {i+1} ---')
            print(text[:5000])
            print()
