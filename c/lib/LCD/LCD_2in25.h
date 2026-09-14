#ifndef __LCD_2IN25_H
#define __LCD_2IN25_H

#include "DEV_Config.h"

// ZJY225KP-PG01: 2.25 inch, 76(RGB) x 284, ST7789P3
// ST7789P3 GRAM is 240x320
// Screen is 76 pixels wide x 284 pixels tall (in 16bpp mode)
// GRAM column offset = (240 - 76) / 2 = 82
// GRAM row offset = (320 - 284) / 2 = 18
#define LCD_2IN25_HEIGHT      284
#define LCD_2IN25_WIDTH       76

#define LCD_2IN25_X_OFFSET    82
#define LCD_2IN25_Y_OFFSET    18

#define LCD_2IN25_WIDTH_Byte  76

#define HORIZONTAL 0
#define VERTICAL   1


	
typedef struct{
	UWORD WIDTH;
	UWORD HEIGHT;
	UBYTE SCAN_DIR;
}LCD_2IN25_ATTRIBUTES;
extern LCD_2IN25_ATTRIBUTES LCD_2IN25;

/********************************************************************************
function:	
			Macro definition variable name
********************************************************************************/
void LCD_2IN25_Init(UBYTE Scan_dir);
void LCD_2IN25_TurnOnDisplay(void);
void LCD_2IN25_Clear(UWORD Color);
void LCD_2IN25_Display(UWORD *Image);
void LCD_2IN25_DisplayWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD *Image);
void LCD_2IN25_DisplayPoint(UWORD X, UWORD Y, UWORD Color);

void Handler_2IN25_LCD(int signo);
#endif
