/*
 * LCD_2in25_test.c  ->  SD-card BMP slideshow + IMU viewer for the 2.25" ST7789P3 (76 x 284)
 *
 * On boot it reads 24-bit .bmp photos from the root of a micro-SD card and
 * plays them ONCE as a slideshow (each image shown for AUTO_INTERVAL_MS).
 * After the single pass it switches to the ICM20948 6-axis sensor screen,
 * showing live accelerometer, gyroscope and Euler-angle data.
 *
 * (Referenced from the 3.49" 01-LCD QMI8658 demo; this board is 2.25" and uses
 *  an ICM20948 IMU, so the on-board icm20948 driver is used instead.)
 *
 * Wiring
 *   LCD  (SPI1): CLK=GP30, MOSI=GP31, CS=GP29, DC=GP28, RST=GP10, BL=GP23
 *   SD   (SPI0): SD_SCK=GP34, SD_MOSI=GP35, SD_MISO=GP32, SD_CS=GP33
 *   IMU  (I2C0): SDA=GP36, SCL=GP37, INT1=GP39, INT2=GP38
 *
 * Put photos sized 76 x 284 (portrait), 24-bit .bmp, in the card root.
 * Larger/smaller images are centered and cropped to fit the screen.
 */
#include "LCD_Test.h"
#include "LCD_2in25.h"
#include "ImageData.h"     /* built-in boot photo: gImage_225 (76x284 RGB565) */
#include "icm20948.h"      /* ICM20948 IMU driver (I2C0 @ 0x68, WHOAMI 0xEA) */
#include "QMI8658.h"       /* QMI8658 IMU driver  (I2C0 @ 0x6a/0x6b, id 0x05) */
#include "RGB.h"           /* WS2812 breathing light on GP25 (runs on core1) */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ff.h"
#include "f_util.h"
#include "hw_config.h"
#include "sd_card.h"
#include "diskio.h"

#define LCD_W            76
#define LCD_H            284
#define MAX_FILES        64
#define AUTO_INTERVAL_MS 3000

static UWORD *framebuf = NULL;
static char filenames[MAX_FILES][256];
static int file_count = 0;

/* Defined in FatFs glue.c: sector offset added to every disk_read/disk_write,
 * so a FAT/exFAT partition inside a GPT disk can be mounted as a super-floppy. */
extern uint32_t g_sd_lba_offset;

static uint8_t g_diag_buf[512];

static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* If sector 0 is a GPT protective MBR (partition type 0xEE), parse the GPT and
 * return the start LBA of the first partition whose VBR carries a 0x55AA
 * signature (a FAT/exFAT volume). Returns 0 for plain MBR/SFD cards, which
 * FatFs mounts directly. */
static uint32_t sd_find_gpt_fat_base(void)
{
    static uint8_t b[512];
    if (disk_read(0, b, 0, 1) != RES_OK) return 0;
    if (b[446 + 4] != 0xEE) return 0;                 /* not a GPT protective MBR */
    if (disk_read(0, b, 1, 1) != RES_OK) return 0;    /* GPT header @ LBA1 */
    if (memcmp(b, "EFI PART", 8) != 0) return 0;
    uint32_t pte_lba = rd_u32(b + 72);                /* partition entry array LBA */
    uint32_t n_ent   = rd_u32(b + 80);                /* number of entries */
    uint32_t pte_sz  = rd_u32(b + 84);                /* bytes per entry */
    if (pte_sz < 128 || pte_sz > 512) return 0;
    uint32_t per_sec = 512 / pte_sz;

    for (uint32_t s = 0; s < 8; s++) {                /* scan a few PTE sectors */
        if (disk_read(0, b, pte_lba + s, 1) != RES_OK) return 0;
        for (uint32_t i = 0; i < per_sec; i++) {
            uint32_t idx = s * per_sec + i;
            if (idx >= n_ent) return 0;
            const uint8_t *e = b + i * pte_sz;
            int used = 0;
            for (int k = 0; k < 16; k++) if (e[k]) { used = 1; break; }
            if (!used) continue;
            uint32_t first = rd_u32(e + 32);          /* GPTE first LBA */
            if (!first) continue;
            if (disk_read(0, g_diag_buf, first, 1) == RES_OK &&
                g_diag_buf[510] == 0x55 && g_diag_buf[511] == 0xAA) {
                return first;
            }
        }
    }
    return 0;
}

/* ---- helpers ------------------------------------------------------------ */

static int has_bmp_ext(const char *name)
{
    int len = strlen(name);
    if (len < 4) return 0;
    return (name[len-4] == '.') &&
           (name[len-3] == 'b' || name[len-3] == 'B') &&
           (name[len-2] == 'm' || name[len-2] == 'M') &&
           (name[len-1] == 'p' || name[len-1] == 'P');
}

static void scan_bmp_files(void)
{
    file_count = 0;
    DIR dp;
    FILINFO fno;

    FRESULT fr = f_opendir(&dp, "0:/");
    if (fr != FR_OK) fr = f_opendir(&dp, "/");
    if (fr != FR_OK) return;

    while (1) {
        fr = f_readdir(&dp, &fno);
        if (fr != FR_OK || fno.fname[0] == '\0') break;
        if (fno.fattrib & AM_DIR) continue;

        const char *name = fno.fname;
        const char *alt  = fno.altname;
        if (has_bmp_ext(name) || has_bmp_ext(alt)) {
            const char *use = has_bmp_ext(name) ? name : alt;
            if (file_count < MAX_FILES) {
                strncpy(filenames[file_count], use, 255);
                filenames[file_count][255] = '\0';
                file_count++;
            }
        }
    }
    f_closedir(&dp);
}

/* Diagnostic detail from the last display_bmp() call, shown on screen so the
 * exact failure point of the first image is visible without a serial cable. */
static int      g_dbg_fr  = 0;   /* last FatFs FRESULT */
static uint16_t g_dbg_bpp = 0;   /* bit depth read from the BMP header */
static int32_t  g_dbg_w   = 0;   /* image width  from header */
static int32_t  g_dbg_h   = 0;   /* image height from header */

/* display_bmp step codes (negative = which step failed). */
#define BMP_OK          0
#define BMP_ERR_OPEN   -1        /* f_open failed */
#define BMP_ERR_HDR    -2        /* header read short */
#define BMP_ERR_SIG    -3        /* not a 'BM' file */
#define BMP_ERR_BPP    -4        /* not 24-bit */
#define BMP_ERR_LSEEK  -5        /* f_lseek to pixel data failed */
#define BMP_ERR_MALLOC -6        /* out of memory for one row */
#define BMP_ERR_READ   -7        /* pixel row read short/failed */

/* Draw one 24-bit BMP, centered/cropped onto the 76x284 canvas. */
static int display_bmp(const char *path)
{
    /* static, not on the stack: with exFAT enabled sizeof(FIL) is large and the
     * image path is already stack-heavy; keeping it off the stack avoids
     * overflow/corruption that can freeze the first image. */
    static FIL fil;
    FRESULT fr;
    UINT br;

    fr = f_open(&fil, path, FA_READ);
    g_dbg_fr = (int)fr;
    if (fr != FR_OK) return BMP_ERR_OPEN;

    uint8_t hdr[54];
    fr = f_read(&fil, hdr, 54, &br);
    g_dbg_fr = (int)fr;
    if (fr != FR_OK || br != 54) { f_close(&fil); return BMP_ERR_HDR; }
    if (hdr[0] != 'B' || hdr[1] != 'M') { f_close(&fil); return BMP_ERR_SIG; }

    uint32_t data_offset = hdr[10] | (hdr[11]<<8) | (hdr[12]<<16) | (hdr[13]<<24);
    int32_t  img_w = (int32_t)(hdr[18] | (hdr[19]<<8) | (hdr[20]<<16) | (hdr[21]<<24));
    int32_t  img_h = (int32_t)(hdr[22] | (hdr[23]<<8) | (hdr[24]<<16) | (hdr[25]<<24));
    uint16_t bpp   = hdr[28] | (hdr[29]<<8);

    g_dbg_bpp = bpp;
    g_dbg_w   = img_w;
    g_dbg_h   = img_h;

    /* Accept 24-bit (BGR) and 32-bit (BGRA) uncompressed BMPs. Some photo
     * exporters save 32-bit even for opaque images, which the old 24-bit-only
     * check silently rejected -> "a few photos never entered the slideshow". */
    if (bpp != 24 && bpp != 32) { f_close(&fil); return BMP_ERR_BPP; }
    uint32_t bytes_pp = bpp / 8;   /* 3 for BGR, 4 for BGRA */

    int top_down = 0;
    if (img_h < 0) { top_down = 1; img_h = -img_h; }

    fr = f_lseek(&fil, data_offset);
    g_dbg_fr = (int)fr;
    if (fr != FR_OK) { f_close(&fil); return BMP_ERR_LSEEK; }

    uint32_t row_bytes  = (uint32_t)img_w * bytes_pp;
    uint32_t padded_row = (row_bytes + 3) & ~3u;

    uint8_t *line = (uint8_t *)malloc(padded_row);
    if (!line) { f_close(&fil); return BMP_ERR_MALLOC; }

    /* Center offsets (may be negative when the image is bigger than screen). */
    int off_x = (LCD_W - img_w) / 2;
    int off_y = (LCD_H - img_h) / 2;

    Paint_Clear(BLACK);

    int read_err = 0;
    for (int32_t row = 0; row < img_h; row++) {
        fr = f_read(&fil, line, padded_row, &br);
        if (fr != FR_OK || br != padded_row) { g_dbg_fr = (int)fr; read_err = 1; break; }

        /* BMP source row -> image y (bottom-up unless top_down). */
        int32_t iy = top_down ? row : (img_h - 1 - row);
        int ty = off_y + iy;
        if (ty < 0 || ty >= LCD_H) continue;

        for (int32_t x = 0; x < img_w; x++) {
            int tx = off_x + x;
            if (tx < 0 || tx >= LCD_W) continue;
            uint8_t b = line[x*bytes_pp + 0];
            uint8_t g = line[x*bytes_pp + 1];
            uint8_t r = line[x*bytes_pp + 2];
            uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            Paint_SetPixel((UWORD)tx, (UWORD)ty, color);
        }
    }

    free(line);
    f_close(&fil);
    if (read_err) return BMP_ERR_READ;   /* header parsed but pixel data short */
    LCD_2IN25_Display(framebuf);
    return BMP_OK;
}

/* Returns 0 on success, -1 if the image could not be shown. */
static int show_image(int idx)
{
    if (file_count == 0) return -1;
    char path[280];
    snprintf(path, sizeof(path), "0:/%s", filenames[idx]);

    Paint_NewImage((UBYTE *)framebuf, LCD_2IN25.WIDTH, LCD_2IN25.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(BLACK);

    return display_bmp(path);   /* no red screen: caller just skips bad files */
}

static void show_status(const char *l1, const char *l2, UWORD bg, UWORD fg)
{
    Paint_NewImage((UBYTE *)framebuf, LCD_2IN25.WIDTH, LCD_2IN25.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(bg);
    if (l1) Paint_DrawString_EN(1, 1,  l1, &Font12, fg, bg);
    if (l2) Paint_DrawString_EN(1, 16, l2, &Font12, fg, bg);
    LCD_2IN25_Display(framebuf);
}

/* ---- SD-card detection result screen -----------------------------------
 * One consolidated screen that clearly reports whether the micro-SD card is
 * properly inserted and, if so, how many .bmp photos were found. It stays on
 * screen long enough to read before the slideshow starts.
 *   inserted : card was detected by disk_initialize() (STA has no NODISK bit)
 *   mounted  : FatFs mounted the volume OK
 *   photos   : number of .bmp files found in the card root (-1 = not scanned)
 */
static void show_sd_detect(bool inserted, bool mounted, int photos, int code)
{
    char l1[24], l2[24], l3[24];
    UWORD bg, fg;

    l3[0] = '\0';

    if (!inserted) {
        /* No card in the socket, or card not responding at all. */
        snprintf(l1, sizeof(l1), "SD: NO CARD");
        snprintf(l2, sizeof(l2), "insert card");
        snprintf(l3, sizeof(l3), "STA=0x%02X", (unsigned)code);
        bg = RED;  fg = WHITE;
    } else if (!mounted) {
        /* Card present but the filesystem could not be mounted.
         * code carries the FatFs FRESULT so the exact cause is visible:
         *   FR=1  disk error  (wiring / SPI / addressing)
         *   FR=3  not ready   (card did not initialise)
         *   FR=13 no filesys  (read OK but no FAT/exFAT -> reformat FAT32) */
        snprintf(l1, sizeof(l1), "SD: MOUNT ERR");
        snprintf(l2, sizeof(l2), "FR=%d", code);
        if (code == 13)      snprintf(l3, sizeof(l3), "no FAT/exFAT");
        else if (code == 1)  snprintf(l3, sizeof(l3), "disk error");
        else if (code == 3)  snprintf(l3, sizeof(l3), "not ready");
        else                 snprintf(l3, sizeof(l3), "mount fail");
        bg = RED;  fg = WHITE;
    } else if (photos <= 0) {
        /* Card OK but no photos on it. */
        snprintf(l1, sizeof(l1), "SD: OK");
        snprintf(l2, sizeof(l2), "0 photos");
        bg = BLUE; fg = WHITE;
    } else {
        /* Card OK and photos found. */
        snprintf(l1, sizeof(l1), "SD: OK");
        snprintf(l2, sizeof(l2), "%d photos", photos);
        bg = WHITE; fg = BLACK;
    }

    Paint_NewImage((UBYTE *)framebuf, LCD_2IN25.WIDTH, LCD_2IN25.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(bg);
    Paint_DrawString_EN(2,  4, "SD DETECT", &Font12, fg, bg);
    Paint_DrawString_EN(2, 30, l1, &Font12, fg, bg);
    Paint_DrawString_EN(2, 46, l2, &Font12, fg, bg);
    if (l3[0]) Paint_DrawString_EN(2, 62, l3, &Font12, fg, bg);
    LCD_2IN25_Display(framebuf);
}

/* ---- IMU 6-axis viewer (ICM20948) --------------------------------------- */

/* Draw the live 6-axis sensor screen forever.  Layout is tuned for the tall,
 * narrow 76x284 panel using the small Font12 (7x12).
 * Accel is shown in mg, gyro in dps.
 *
 * The IMU is auto-detected at runtime: ICM20948 (I2C0 0x68, WHOAMI 0xEA) is
 * tried first, then QMI8658 (0x6a/0x6b, chip-id 0x05), so the screen works on
 * either board variant. Shown after the slideshow finishes one pass.
 * (Set #if 1 back to #if 0 to disable.) */
/* Which IMU the auto-probe found. */
enum { IMU_NONE = 0, IMU_ICM20948, IMU_QMI8658 };

#if 1
static void imu_viewer(void)
{
    /* RGB565 label colors (same palette as the 01-LCD reference demo). */
    const UWORD COL_HDR = 0xF410;   /* light red header bar */
    const UWORD COL_ACC = 0x4F30;   /* green  ACC label bar */
    const UWORD COL_GYR = 0x2595;   /* teal   GYR label bar */

    /* Scale factors for the ICM20948 path, matching icm20948init(): accel
     * FS = 2g -> 16384 LSB/g (mg = raw / 16.384); gyro FS = 1000dps -> 32.8 LSB/dps. */
    const float ACC_MG_PER_LSB  = 1000.0f / 16384.0f;
    const float GYR_DPS_PER_LSB = 1.0f / 32.8f;

    /* ---- Auto-detect the IMU ----
     * This board family ships with either an ICM20948 (I2C 0x68, WHOAMI 0xEA)
     * or a QMI8658 (I2C 0x6a/0x6b, chip-id 0x05). Probe ICM20948 first, then
     * fall back to QMI8658, so the 6-axis screen works on both variants. */
    int imu = IMU_NONE;
    if (icm20948Check()) {
        imu = IMU_ICM20948;
        printf("ICM20948 detected\r\n");
        icm20948init();
    } else if (QMI8658_init()) {
        imu = IMU_QMI8658;
        printf("QMI8658 detected\r\n");
    } else {
        printf("No IMU detected (ICM20948/QMI8658)\r\n");
        show_status("IMU", "NOT FOUND", RED, WHITE);
        while (1) DEV_Delay_ms(1000);
    }

    /* ---- Draw the static layout ONCE: white background, colored labels ----
     * Six stacked rows (ACC X/Y/Z, GYR X/Y/Z). Each row is a colored label with
     * its live value drawn just underneath. Only the value area is repainted in
     * the loop, so the labels never flicker. Tuned for the 76 x 284 panel. */
    Paint_NewImage((UBYTE *)framebuf, LCD_2IN25.WIDTH, LCD_2IN25.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(WHITE);

    /* header bar */
    Paint_DrawRectangle(0, 0, LCD_W, 24, COL_HDR, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(4, 4, "6-AXIS", &Font16, BLACK, COL_HDR);

    static const char *labels[6] = { "ACC_X","ACC_Y","ACC_Z","GYR_X","GYR_Y","GYR_Z" };
    int value_y[6];
    for (int i = 0; i < 6; i++) {
        int top = 32 + i * 42;                 /* row top on screen */
        value_y[i] = top + 18;                 /* value sits under the label */
        UWORD c = (i < 3) ? COL_ACC : COL_GYR; /* green for ACC, teal for GYR */
        Paint_DrawRectangle(2, top, 62, top + 16, c, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawString_EN(3, top, labels[i], &Font16, BLACK, c);
    }
    LCD_2IN25_Display(framebuf);

    /* ---- Live loop: read the sensor and repaint only the six value rows ----
     * ACC in mg, GYR in dps. Values are updated ~10x per second so the numbers
     * move as the board is tilted/rotated. */
    while (1) {
        float v[6];
        if (imu == IMU_ICM20948) {
            int16_t ax, ay, az, gx, gy, gz;
            icm20948AccelRead(&ax, &ay, &az);
            icm20948GyroRead(&gx, &gy, &gz);
            v[0] = ax * ACC_MG_PER_LSB; v[1] = ay * ACC_MG_PER_LSB; v[2] = az * ACC_MG_PER_LSB;
            v[3] = gx * GYR_DPS_PER_LSB; v[4] = gy * GYR_DPS_PER_LSB; v[5] = gz * GYR_DPS_PER_LSB;
        } else {
            /* QMI8658 already returns acc in mg and gyro in dps. */
            float acc[3], gyro[3];
            unsigned int tim = 0;
            QMI8658_read_xyz(acc, gyro, &tim);
            v[0] = acc[0];  v[1] = acc[1];  v[2] = acc[2];
            v[3] = gyro[0]; v[4] = gyro[1]; v[5] = gyro[2];
        }
        for (int i = 0; i < 6; i++) {
            /* Clean signed number with 2 decimals, incl. negative coordinates.
             * The board's built-in Paint_DrawNum() mishandles negatives (its
             * `temp % 10` on a negative int emits stray symbols like ',' '.' '/');
             * the 3.49" reference avoids this by formatting with sprintf. We do
             * the same: snprintf("%.2f") renders a proper leading '-' and a
             * decimal point with no garbage. */
            char num[16];
            snprintf(num, sizeof(num), "%.2f", v[i]);
            Paint_DrawRectangle(2, value_y[i], LCD_W, value_y[i] + 14, WHITE,
                                DOT_PIXEL_1X1, DRAW_FILL_FULL);
            Paint_DrawString_EN(4, value_y[i], num, &Font12, BLACK, WHITE);
        }
        LCD_2IN25_Display(framebuf);
        DEV_Delay_ms(100);
    }
}
#endif

/* How long the built-in boot photo stays on screen before SD detection. */
#define BOOT_PHOTO_MS  3000

/* ---- Boot splash: show the built-in photo ------------------------------ */
/* Draws the photo that ships with the board (compiled into flash as
 * gImage_225, 76x284 RGB565) directly from the firmware image, so it shows
 * even with no SD card inserted. Runs before any SD-card bring-up. */
static void show_boot_photo(void)
{
    /* gImage_225 is exactly LCD_W x LCD_H pixels, 2 bytes per pixel, stored
     * row-major in the same byte order LCD_2IN25_Display() streams over SPI,
     * so the array can be handed to the display driver as-is.
     * The display output is still OFF at this point (DISPON was not sent in
     * LCD_2IN25_InitReg): push the photo into GRAM first, THEN turn the
     * display on, so the very first visible frame is the photo itself. */
    LCD_2IN25_Display((UWORD *)gImage_225);
    LCD_2IN25_TurnOnDisplay();
    /* First valid frame is on screen now - safe to light the backlight,
     * so power-on goes straight to the photo with no white flash. */
    DEV_SET_PWM(50);
    DEV_Delay_ms(BOOT_PHOTO_MS);
}

/* ---- entry point -------------------------------------------------------- */

int LCD_2in25_test(void)
{
    DEV_Delay_ms(100);
    printf("2.25inch LCD SD-card slideshow\r\n");

    if (DEV_Module_Init() != 0) {
        return -1;
    }
    /* Backlight stays OFF here on purpose: the panel is an undriven white
     * state until the first frame is pushed and DISPON is sent. Turning the
     * backlight on now would show a ~0.5 s white flash during LCD init.
     * show_boot_photo() enables it right after TurnOnDisplay(). */

    /* Start the WS2812 RGB breathing light on GP25 (core1). It runs
     * independently for the whole program - slideshow and IMU are unaffected. */
    RGB_Start();

    LCD_2IN25_Init(VERTICAL);
    /* No white clear / no TurnOnDisplay here: LCD_2IN25_InitReg() deliberately
     * leaves the display output OFF (DISPON 0x29 is not sent), so the panel
     * shows nothing while we prepare the first frame. show_boot_photo() pushes
     * the boot photo into GRAM first and only then turns the display on, so
     * power-on goes straight to the photo - no white frame, no GRAM garbage. */

    UDOUBLE Imagesize = LCD_2IN25.HEIGHT * LCD_2IN25.WIDTH * 2;
    if ((framebuf = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for framebuffer memory...\r\n");
        return -1;
    }

    /* ---- Boot splash: show the photo built into the firmware first, before
     * any SD-card detection. It lives in flash, so it works with or without
     * a card inserted. */
    printf("Showing built-in boot photo\r\n");
    show_boot_photo();

    /* ---- SD-card bring-up: on ANY failure (no card / init error / mount
     * error) skip the slideshow entirely and go straight to the IMU (gyro)
     * screen - no error screen, no hanging. sd_ok tracks whether we have a
     * mounted filesystem to play photos from. */
    bool sd_ok = false;

    if (!sd_init_driver()) {
        printf("sd_init_driver() failed -> no SD card, enter IMU viewer\r\n");
    } else {
        g_sd_lba_offset = 0;
        DSTATUS ds = disk_initialize(0);
        printf("disk_initialize STA=0x%02X\r\n", (unsigned)ds);
        if (ds & 0x02) {                /* STA_NODISK: no card in the socket */
            printf("SD: no card detected (STA_NODISK) -> enter IMU viewer\r\n");
        } else {
            uint32_t part_base = sd_find_gpt_fat_base();
            if (part_base) {
                g_sd_lba_offset = part_base;    /* GPT disk -> mount partition as super-floppy */
                printf("GPT disk: FAT partition at LBA %lu\r\n", (unsigned long)part_base);
            }

            /* static (not on the stack): with exFAT enabled sizeof(FATFS) is large. */
            static FATFS fs;
            FRESULT fr = f_mount(&fs, "0:", 1);
            printf("f_mount FR=%d (%s)\r\n", (int)fr, FRESULT_str(fr));
            if (fr != FR_OK) {
                /* FR=1 disk error, FR=3 not ready, FR=13 no FAT/exFAT filesystem. */
                printf("SD: mount failed -> enter IMU viewer\r\n");
            } else {
                sd_ok = true;
            }
        }
    }

    if (sd_ok) {
        scan_bmp_files();
        printf("Found %d BMP files\r\n", file_count);

        /* ---- Consolidated SD detection result ----
         * Card is inserted and mounted at this point; show whether photos exist
         * and how many, then pause a few seconds so it can be read. */
        show_sd_detect(true, true, file_count, 0);
        DEV_Delay_ms(4000);

        /* Play the slideshow ONCE, then switch to the ICM20948 6-axis viewer.
         * Unreadable files are skipped so the pass never stalls; if EVERY file
         * is skipped, show a status so the screen is not left blank. With 0
         * photos the loop is a no-op and we fall straight through to the IMU
         * viewer instead of hanging on the "0 photos" screen. */
        int shown = 0;
        for (int i = 0; i < file_count; i++) {
            int rc = show_image(i);
            if (rc == BMP_OK) {
                shown++;
                DEV_Delay_ms(AUTO_INTERVAL_MS);
            } else {
                /* Make the failure visible on the panel instead of silently
                 * skipping, so the exact dying step of image 0 is obvious. */
                const char *step =
                    (rc == BMP_ERR_OPEN)   ? "OPEN FAIL"  :
                    (rc == BMP_ERR_HDR)    ? "HDR SHORT"  :
                    (rc == BMP_ERR_SIG)    ? "NOT BMP"    :
                    (rc == BMP_ERR_BPP)    ? "NOT 24BIT"  :
                    (rc == BMP_ERR_LSEEK)  ? "LSEEK FAIL" :
                    (rc == BMP_ERR_MALLOC) ? "NO MEMORY"  :
                    (rc == BMP_ERR_READ)   ? "READ FAIL"  : "UNKNOWN";
                char l2[24];
                snprintf(l2, sizeof(l2), "FR%d bpp%u", g_dbg_fr, (unsigned)g_dbg_bpp);
                printf("Image %d (%s) failed: %s  FR=%d bpp=%u w=%ld h=%ld\r\n",
                       i, filenames[i], step, g_dbg_fr, (unsigned)g_dbg_bpp,
                       (long)g_dbg_w, (long)g_dbg_h);
                show_status(step, l2, RED, WHITE);
                DEV_Delay_ms(3000);
            }
        }
        if (file_count > 0 && shown == 0) {
            /* No image could be decoded (e.g. not 24-bit BMP or wrong size). */
            show_status("BMP", "DECODE ERR", BLUE, WHITE);
            DEV_Delay_ms(1500);
        }
    }

    /* ---- IMU 6-axis viewer (ICM20948) - runs forever ---- */
    printf("Entering IMU viewer\r\n");
    imu_viewer();

    free(framebuf);
    framebuf = NULL;
    DEV_Module_Exit();
    return 0;
}
