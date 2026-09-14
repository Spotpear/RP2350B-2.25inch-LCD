#include "LCD_Test.h"   //Examples
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LCD_BL_PIN  23

int main(void)
{
    /* Hold the backlight off from the earliest moment user code runs.
     * (Hardware: R18 pull-up removed, R17 pull-down fitted, so hi-z
     * already means off - this just removes any ambiguity.) */
    gpio_init(LCD_BL_PIN);
    gpio_set_dir(LCD_BL_PIN, GPIO_OUT);
    gpio_put(LCD_BL_PIN, 0);

    LCD_2in25_test();
    return 0;
}
