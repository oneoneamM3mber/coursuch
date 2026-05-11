#!/usr/bin/env python3
"""
Генерирует тестовые BMP изображения для демонстрации StegoLab.
Запустить: python3 generate_test_images.py
"""
import struct
import os
import random

def write_bmp(filename, width, height, pixels_rgb):
    """pixels_rgb: list of (R,G,B) tuples, row by row top-to-bottom"""
    # BMP stores rows bottom-to-top, padded to 4-byte boundary
    row_size = (width * 3 + 3) & ~3
    pad = row_size - width * 3
    pixel_data = bytearray()
    for row in reversed(pixels_rgb):  # flip vertically
        for (r, g, b) in row:
            pixel_data += bytes([b, g, r])  # BMP is BGR
        pixel_data += bytes(pad)
    
    file_size = 54 + len(pixel_data)
    header = struct.pack('<2sIHHI', b'BM', file_size, 0, 0, 54)
    dib = struct.pack('<IiiHHIIiiII',
        40, width, height, 1, 24, 0,
        len(pixel_data), 2835, 2835, 0, 0)
    
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(dib)
        f.write(pixel_data)
    print(f"  Created: {filename} ({width}x{height}, {file_size} bytes)")

def gradient_image(width, height):
    pixels = []
    for y in range(height):
        row = []
        for x in range(width):
            r = int(255 * x / width)
            g = int(255 * y / height)
            b = int(255 * (1 - x / width))
            row.append((r, g, b))
        pixels.append(row)
    return pixels

def noise_image(width, height):
    random.seed(42)
    pixels = []
    for y in range(height):
        row = [(random.randint(100,200), random.randint(100,200), random.randint(100,200))
               for x in range(width)]
        pixels.append(row)
    return pixels

def solid_color_blocks(width, height):
    colors = [(220,60,60),(60,150,220),(60,200,100),(220,180,60),(180,60,220)]
    block_w = width // len(colors)
    pixels = []
    for y in range(height):
        row = []
        for x in range(width):
            idx = min(x // block_w, len(colors)-1)
            row.append(colors[idx])
        pixels.append(row)
    return pixels

os.makedirs('resources', exist_ok=True)

print("Generating test BMP images...")
write_bmp('resources/test_gradient_800x600.bmp', 800, 600, gradient_image(800, 600))
write_bmp('resources/test_noise_640x480.bmp',    640, 480, noise_image(640, 480))
write_bmp('resources/test_blocks_400x300.bmp',   400, 300, solid_color_blocks(400, 300))
write_bmp('resources/test_small_100x100.bmp',    100, 100, gradient_image(100, 100))

# Also write a sample text file
with open('resources/sample_message.txt', 'w', encoding='utf-8') as f:
    f.write("Это тестовое скрытое сообщение для демонстрации LSB стеганографии.\n")
    f.write("Метод LSB (Least Significant Bit) позволяет скрывать данные\n")
    f.write("путём замены младших битов пикселей битами сообщения.\n")
    f.write("PSNR > 40 дБ гарантирует визуальную неотличимость изображений.\n")
    f.write("\nАвтор: Студент кафедры информационной безопасности\n")
    f.write("Курсовая работа: Стеганография изображений (C++, Qt, SQLite)\n")

print("  Created: resources/sample_message.txt")
print("\nAll test files ready! Open StegoLab and load from resources/ folder.")
