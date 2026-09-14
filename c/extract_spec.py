# -*- coding: utf-8 -*-
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
import pdfplumber

# Extract ALL text from the spec sheet to find exact resolution and driving method
with pdfplumber.open(r'C:\Users\Administrator\Desktop\ZJY225KP-PG01.pdf') as pdf:
    for i, page in enumerate(pdf.pages):
        text = page.extract_text()
        if text:
            print(f'=== Page {i+1} ===')
            print(text)
            print()
