# RP2350B 2.25inch LCD

[中文](#中文说明) | [English](#english)

基于 **RP2350B (Pico 2, 48-GPIO 版本)** 的 2.25 寸 LCD 演示工程，包含 Pico SDK C 例程与 MicroPython 例程。

Demo project for the **RP2350B (Pico 2, 48-GPIO variant)** with a 2.25inch LCD, including Pico SDK C examples and MicroPython examples.

---

## 中文说明

### 1. 硬件规格

| 项目 | 参数 |
| --- | --- |
| 主控 | Raspberry Pi RP2350B（48 GPIO 版本，`PICO_RP2350A = 0`） |
| 屏幕 | 2.25 inch LCD，型号 ZJY225KP-PG01 |
| 驱动 IC | ST7789P3 |
| 分辨率 | 76 (RGB) x 284（竖屏） |
| 颜色 | 16bpp RGB565 |
| GRAM 偏移 | X_OFFSET = 82，Y_OFFSET = 18 |
| 板级头文件 | `c/boards/pico2_rp2350b.h` |
| Pico SDK | 2.1.0 |

> 该板为 RP2350B（48 脚）封装，SD 卡（GP32–GP35）与 IMU（GP36/GP37）使用的高编号 GPIO 仅在 "B" 版本上存在，因此工程通过 `boards/pico2_rp2350b.h` 自定义板级头文件将 `PICO_RP2350A` 置为 0 来启用这些引脚。

### 2. 管脚连接

**LCD（SPI1）**

| LCD | GPIO |
| --- | --- |
| CLK | GP30 |
| MOSI | GP31 |
| CS | GP29 |
| DC | GP28 |
| RST | GP10 |
| BL（背光） | GP23 |

**micro-SD 卡（SPI0）**

| SD | GPIO |
| --- | --- |
| SCK | GP34 |
| MOSI | GP35 |
| MISO | GP32 |
| CS | GP33 |

**IMU（I2C0）**

| IMU | GPIO |
| --- | --- |
| SDA | GP36 |
| SCL | GP37 |
| INT1 | GP39 |
| INT2 | GP38 |

- ICM20948：I2C 地址 `0x68`，WHOAMI `0xEA`
- QMI8658：I2C 地址 `0x6a` / `0x6b`，ID `0x05`

**WS2812 RGB 灯**

| 信号 | GPIO |
| --- | --- |
| WS2812 DATA | GP25（由 core1 驱动） |

### 3. 目录结构

```
.
├── c/                      # Pico SDK C 工程
│   ├── CMakeLists.txt
│   ├── boards/             # 自定义板级头文件 (RP2350B)
│   ├── examples/           # 测试例程
│   │   ├── LCD_2in25_test.c
│   │   ├── ImageData.c/h       # 内置开机图片 (gImage_225, 76x284 RGB565)
│   │   └── ImageData_225.c
│   ├── lib/
│   │   ├── Config/         # 硬件底层接口与引脚定义 (DEV_Config)
│   │   ├── LCD/            # LCD_2in25 驱动 (ST7789P3)
│   │   ├── GUI/            # GUI_Paint 绘图库
│   │   ├── Fonts/          # 字库（含中文）
│   │   ├── FatFs/          # FatFs + SD 卡驱动（支持 GPT 分区）
│   │   ├── Icm20948/       # ICM20948 六轴传感器驱动
│   │   ├── QMI8658/        # QMI8658 六轴传感器驱动
│   │   ├── Infrared/       # 红外收发
│   │   └── RGB/            # WS2812 (PIO) 驱动
│   ├── main.c
│   ├── pico_sdk_import.cmake
│   └── img_preview/        # 图片预览生成结果
├── python/                 # MicroPython 例程
│   ├── RBG.py              # WS2812 PIO 驱动 + 颜色渐变呼吸灯
│   └── adc.py              # ADC 采样（GP47）读取电压
└── firmware.uf2            # 预编译固件
```

### 4. 编译与烧录（C 工程）

依赖：Raspberry Pi Pico SDK 2.1.0 与 `arm-none-eabi-gcc` 工具链（推荐直接使用 VS Code 的 Raspberry Pi Pico 扩展）。

```bash
cd c
mkdir -p build
cd build
export PICO_SDK_PATH=/path/to/pico-sdk     # 指向你的 pico-sdk 目录

cmake ..
make
```

编译完成后会生成 `build/main.uf2`。按住开发板 BOOTSEL 键并插入 USB，将 uf2 文件拷贝到出现的 U 盘中即可完成烧录。

> 也可直接烧录仓库根目录下预编译好的 `firmware.uf2`，无需自行编译。

### 5. 例程说明

**`examples/LCD_2in25_test.c` — SD 卡 BMP 相册 + IMU 数据查看器**

1. 开机后从 micro-SD 卡根目录读取 24-bit `.bmp` 图片，按 `AUTO_INTERVAL_MS`（默认 3000 ms）的间隔**播放一轮**幻灯片；
   图片建议尺寸为 76 x 284（竖屏），其他尺寸会居中裁剪适配。
2. 播放完毕后自动切换到 ICM20948 六轴传感器界面，实时显示加速度、角速度与欧拉角数据。
3. `main.c` 会在用户代码运行的第一时间将背光（GP23）拉低，避免上电瞬间的亮屏闪烁。

**`python/RBG.py`** — 使用 `rp2` PIO 驱动 GP25 上的 WS2812，在红/绿/蓝/黄/青/品红/白之间循环渐变。

**`python/adc.py`** — 读取 GP47 上的 ADC 值并换算为电压，每 0.5 s 打印一次。

### 6. 注意事项

- `c/build/` 为 CMake 生成目录，已被 `.gitignore` 排除，无需提交。
- 若使用其他型号的 micro-SD 卡，注意该驱动已内置 GPT 保护性 MBR 的解析逻辑，可正常挂载位于 GPT 磁盘内的 FAT/exFAT 分区。

---

## English

### 1. Specifications

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

### 2. Pin Connections

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

### 3. Directory Layout

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
│   │   ├── Fonts/          # Fonts (incl. Chinese)
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

### 4. Build & Flash (C project)

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

### 5. Examples

**`examples/LCD_2in25_test.c` — SD-card BMP slideshow + IMU viewer**

1. On boot it reads 24-bit `.bmp` photos from the root of the micro-SD card and plays them **once** as a slideshow, showing each image for `AUTO_INTERVAL_MS` (3000 ms by default). Photos should be 76 x 284 (portrait); other sizes are centre-cropped to fit.
2. After the pass it switches to the ICM20948 6-axis sensor screen, showing live accelerometer, gyroscope and Euler-angle data.
3. `main.c` drives the backlight (GP23) low at the earliest moment user code runs, removing any power-on flash.

**`python/RBG.py`** — drives the WS2812 on GP25 with an `rp2` PIO program, fading through red / green / blue / yellow / cyan / magenta / white.

**`python/adc.py`** — reads the ADC on GP47 and prints the converted voltage every 0.5 s.

### 6. Notes

- `c/build/` is a generated CMake directory and is excluded via `.gitignore`.
- The SD driver includes GPT protective-MBR parsing, so a FAT/exFAT partition inside a GPT disk also mounts correctly.
