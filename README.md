# RP2350B 2.25inch LCD

Demo project for the **RP2350B (Pico 2, 48-GPIO variant)** with a 2.25inch LCD, including Pico SDK C examples and MicroPython examples.

---

## 1. Specifications

| Item | Value |
| --- | --- |
| MCU | Raspberry Pi RP2350B (48-GPIO variant, `PICO_RP2350A = 0`) |
| Display | 2.25 inch LCD, model ZJY225KP-PG01 |
| Driver IC | ST7789P3 |
| Resolution | 76 (RGB) x 284 (portrait) |
| Color depth | 16bpp RGB565 |
| GRAM offset | X_OFFSET = 82, Y_OFFSET = 18 |
| Board header | `c/boards/pico2_rp2350b.h` |
| Pico SDK | 2.1.0 |

> This board uses the RP2350B (48-pin) package. The SD card (GP32-GP35) and IMU (GP36/GP37) signals sit on high-numbered GPIOs that only exist on the "B" variant, so the project ships a custom board header in `boards/` which sets `PICO_RP2350A = 0` to make those pins valid.

## 2. Pin Connections

**LCD (SPI1)**

| LCD | GPIO |
| --- | --- |
| CLK | GP30 |
| MOSI | GP31 |
| CS | GP29 |
| DC | GP28 |
| RST | GP10 |
| BL (backlight) | GP23 |

**micro-SD card (SPI0)**

| SD | GPIO |
| --- | --- |
| SCK | GP34 |
| MOSI | GP35 |
| MISO | GP32 |
| CS | GP33 |

**IMU (I2C0)**

| IMU | GPIO |
| --- | --- |
| SDA | GP36 |
| SCL | GP37 |
| INT1 | GP39 |
| INT2 | GP38 |

- ICM20948: I2C address `0x68`, WHOAMI `0xEA`
- QMI8658: I2C address `0x6a` / `0x6b`, ID `0x05`

**WS2812 RGB LED**

| Signal | GPIO |
| --- | --- |
| WS2812 DATA | GP25 (driven from core1) |

## 3. Directory Layout

```
.
├── c/                      # Pico SDK C project
│   ├── CMakeLists.txt
│   ├── boards/             # Custom board header (RP2350B)
│   ├── examples/           # Test examples
│   │   ├── LCD_2in25_test.c
│   │   ├── ImageData.c/h       # Built-in boot photo (gImage_225, 76x284 RGB565)
│   │   └── ImageData_225.c
│   ├── lib/
│   │   ├── Config/         # Low-level HW interface and pin definitions (DEV_Config)
│   │   ├── LCD/            # LCD_2in25 driver (ST7789P3)
│   │   ├── GUI/            # GUI_Paint drawing library
│   │   ├── Fonts/          # Fonts (incl. CJK)
│   │   ├── FatFs/          # FatFs + SD card driver (GPT aware)
│   │   ├── Icm20948/       # ICM20948 6-axis sensor driver
│   │   ├── QMI8658/        # QMI8658 6-axis sensor driver
│   │   ├── Infrared/       # Infrared TX/RX
│   │   └── RGB/            # WS2812 (PIO) driver
│   ├── main.c
│   ├── pico_sdk_import.cmake
│   └── img_preview/        # Image preview generator output
├── python/                 # MicroPython examples
│   ├── RBG.py              # WS2812 PIO driver + colour fade
│   └── adc.py              # ADC sampling (GP47) and voltage readout
└── firmware.uf2            # Pre-built firmware
```

## 4. Build & Flash (C project)

Requirements: Raspberry Pi Pico SDK 2.1.0 and the `arm-none-eabi-gcc` toolchain (using the VS Code Raspberry Pi Pico extension is recommended).

```bash
cd c
mkdir -p build
cd build
export PICO_SDK_PATH=/path/to/pico-sdk     # point to your pico-sdk

cmake ..
make
```

This produces `build/main.uf2`. Hold the BOOTSEL button while plugging in USB, then copy the uf2 file onto the mass-storage drive that appears.

> You can also flash the pre-built `firmware.uf2` in the repository root directly.

## 5. Examples

**`examples/LCD_2in25_test.c` — SD-card BMP slideshow + IMU viewer**

1. On boot it reads 24-bit `.bmp` photos from the root of the micro-SD card and plays them **once** as a slideshow, showing each image for `AUTO_INTERVAL_MS` (3000 ms by default). Photos should be 76 x 284 (portrait); other sizes are centre-cropped to fit.
2. After the pass it switches to the ICM20948 6-axis sensor screen, showing live accelerometer, gyroscope and Euler-angle data.
3. `main.c` drives the backlight (GP23) low at the earliest moment user code runs, removing any power-on flash.

**`python/RBG.py`** — drives the WS2812 on GP25 with an `rp2` PIO program, fading through red / green / blue / yellow / cyan / magenta / white.

**`python/adc.py`** — reads the ADC on GP47 and prints the converted voltage every 0.5 s.

## 6. Notes

- `c/build/` is a generated CMake directory and is excluded via `.gitignore`.
- The SD driver includes GPT protective-MBR parsing, so a FAT/exFAT partition inside a GPT disk also mounts correctly.
