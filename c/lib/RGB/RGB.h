#ifndef _RGB_H_
#define _RGB_H_

#include "pico/stdlib.h"

/*
 * WS2812 (NeoPixel) RGB breathing light for the 2.25" board.
 *
 * A single addressable WS2812 LED is wired to GP25 (same pin as the reference
 * CircuitPython RBG.py). Driven via PIO so it needs no CPU timing loop, and the
 * colour-cycle "breathing" effect runs on core1 so the slideshow + IMU screen
 * on core0 are completely unaffected.
 */

#ifndef RGB_PIN
#define RGB_PIN 25          /* GP25 - WS2812 data line */
#endif

/* Initialise the PIO WS2812 driver on the given GPIO. */
void RGB_Init(uint pin);

/* Push one colour to the LED (0-255 per channel). */
void RGB_SetColor(uint8_t r, uint8_t g, uint8_t b);

/* Smoothly cycle through a set of colours forever (never returns). */
void RGB_BreatheTask(void);

/* Launch the breathing effect on core1 (returns immediately). Call once from
 * core0 after DEV_Module_Init(); everything else keeps running normally. */
void RGB_Start(void);

#endif /* _RGB_H_ */
