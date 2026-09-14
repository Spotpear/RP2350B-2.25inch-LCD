/*
 * hw_config.c - SD card hardware configuration for the 2.25" LCD slideshow
 *
 * SD card on SPI0 (RP2350B):
 *     SD_SCK  -> GP34   (SPI0 SCK)
 *     SD_MOSI -> GP35   (SPI0 TX)
 *     SD_MISO -> GP32   (SPI0 RX)
 *     SD_CS   -> GP33   (driven as GPIO by the driver)
 *
 * LCD stays on SPI1 (CLK=GP30, MOSI=GP31, CS=GP29, DC=GP28, RST=GP10, BL=GP23),
 * so there is no bus conflict with the SD card.
 */
#include <assert.h>
#include "hw_config.h"

static spi_t spis[] = {
    {   // spis[0] - SD card on SPI0
        .hw_inst = spi0,
        .sck_gpio = 34,
        .mosi_gpio = 35,
        .miso_gpio = 32,
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
        /* Enable the RP2350 internal pull-up on MISO (GP32). SD cards require
         * their DO/MISO line to be pulled up; the 04 reference board has an
         * external pull-up so it disables the internal one, but this board
         * apparently does not (card returned STA_NOINIT). This does NOT change
         * any pin assignment - GP32 is still MISO - it only switches on an
         * internal resistor. If your board already has an external pull-up,
         * this is harmless. Set back to `true` to disable it. */
        .no_miso_gpio_pull_up = false,
        .baud_rate = 1000 * 1000  // 1 MHz - safe for stability
    }
};

static sd_spi_if_t spi_ifs[] = {
    {   // spi_ifs[0]
        .spi = &spis[0],
        .ss_gpio = 33,               // SD_CS
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA
    }
};

static sd_card_t sd_cards[] = {
    {   // sd_cards[0]
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],
        .use_card_detect = false,
        .card_detect_gpio = 0,
        .card_detected_true = 0,
        .card_detect_use_pull = false,
        .card_detect_pull_hi = false
    }
};

size_t sd_get_num() { return count_of(sd_cards); }

sd_card_t *sd_get_by_num(size_t num) {
    assert(num < sd_get_num());
    if (num < sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}
