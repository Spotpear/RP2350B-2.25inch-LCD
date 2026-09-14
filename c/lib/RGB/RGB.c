#include "RGB.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"

#include "ws2812.pio.h"   /* generated from ws2812.pio by pico_generate_pio_header */

#define RGB_IS_RGBW  false
#define RGB_FREQ_HZ  800000.0f

static PIO  s_pio = pio0;
static uint s_sm  = 0;

/* WS2812 wants the 24 colour bits at the top of the 32-bit FIFO word. */
static inline void put_pixel(uint32_t grb)
{
    pio_sm_put_blocking(s_pio, s_sm, grb << 8u);
}

/* WS2812 byte order is G, R, B. */
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

void RGB_Init(uint pin)
{
    uint offset = pio_add_program(s_pio, &ws2812_program);
    ws2812_program_init(s_pio, s_sm, offset, pin, RGB_FREQ_HZ, RGB_IS_RGBW);
}

void RGB_SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    put_pixel(urgb_u32(r, g, b));
}

/* Colour-cycle "breathing" effect, ported from the reference RBG.py:
 * seven colours at half brightness (127), each pair blended over 100 steps
 * with a 10 ms step, so the LED continuously drifts through the rainbow. */
void RGB_BreatheTask(void)
{
    static const uint8_t colors[][3] = {
        {127,   0,   0},   /* red     */
        {  0, 127,   0},   /* green   */
        {  0,   0, 127},   /* blue    */
        {127, 127,   0},   /* yellow  */
        {  0, 127, 127},   /* cyan    */
        {127,   0, 127},   /* magenta */
        {127, 127, 127},   /* white   */
    };
    const int n = (int)(sizeof(colors) / sizeof(colors[0]));
    const int steps = 100;

    while (true) {
        for (int i = 0; i < n; i++) {
            const uint8_t *from = colors[i];
            const uint8_t *to   = colors[(i + 1) % n];
            for (int s = 0; s < steps; s++) {
                uint8_t r = (uint8_t)(from[0] + (to[0] - from[0]) * s / steps);
                uint8_t g = (uint8_t)(from[1] + (to[1] - from[1]) * s / steps);
                uint8_t b = (uint8_t)(from[2] + (to[2] - from[2]) * s / steps);
                RGB_SetColor(r, g, b);
                sleep_ms(10);
            }
        }
    }
}

/* core1 entry: bring up the LED here so its PIO/FIFO use stays on core1. */
static void rgb_core1_entry(void)
{
    RGB_Init(RGB_PIN);
    RGB_BreatheTask();   /* never returns */
}

void RGB_Start(void)
{
    multicore_launch_core1(rgb_core1_entry);
}
