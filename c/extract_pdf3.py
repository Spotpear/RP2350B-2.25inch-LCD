# -*- coding: utf-8 -*-
import sys
import io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

import pdfplumber

# Search for resolution/memory address/column/page settings in ST7789P3
with pdfplumber.open(r'C:\Users\Administrator\Desktop\ST7789P3.pdf') as pdf:
    for i, page in enumerate(pdf.pages):
        text = page.extract_text()
        if text and any(kw in text for kw in ['resolution', '240 x 320', '240x320', 'CASET', 'RASET', '2Ah', '2Bh', 'Column Address', 'Page Address', 'Memory Address', 'display resolution', 'panel resolution']):
            print(f'--- Page {i+1} ---')
            print(text[:4000])
            print()
