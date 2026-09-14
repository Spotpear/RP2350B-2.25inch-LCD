# -*- coding: utf-8 -*-
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
import pdfplumber

# Extract CASET, RASET, MADCTL pages from ST7789P3
with pdfplumber.open(r'C:\Users\Administrator\Desktop\ST7789P3.pdf') as pdf:
    # Pages around 159 (CASET), 161 (RASET), 176 (MADCTL)
    for pg_num in [159, 160, 161, 162, 176, 177, 178]:
        if pg_num <= len(pdf.pages):
            page = pdf.pages[pg_num - 1]
            text = page.extract_text()
            if text:
                print(f'=== Page {pg_num} ===')
                print(text)
                print()
