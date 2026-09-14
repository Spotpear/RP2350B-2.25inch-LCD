# -*- coding: utf-8 -*-
import pdfplumber

print('=== 屏幕规格书 ZJY225KP-PG01 ===')
with pdfplumber.open(r'C:\Users\Administrator\Desktop\屏幕2.25\焊接接式裸屏规格书ZJY225KP-PG01.pdf') as pdf:
    for i, page in enumerate(pdf.pages):
        text = page.extract_text()
        if text:
            print(f'--- Page {i+1} ---')
            print(text)
            print()

print()
print('=== ST7789P3 ===')
with pdfplumber.open(r'C:\Users\Administrator\Desktop\ST7789P3.pdf') as pdf:
    for i, page in enumerate(pdf.pages):
        text = page.extract_text()
        if text:
            print(f'--- Page {i+1} ---')
            print(text[:3000])
            print()
