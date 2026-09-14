#include "LCD_2in25.h"
#include "DEV_Config.h"

#include <stdlib.h> //itoa()
#include <stdio.h>

LCD_2IN25_ATTRIBUTES LCD_2IN25;

/******************************************************************************
function :	Hardware reset
parameter:
******************************************************************************/
static void LCD_2IN25_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(100);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(100);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(100);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void LCD_2IN25_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void LCD_2IN25_SendData_8Bit(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void LCD_2IN25_SendData_16Bit(UWORD Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte((Data >> 8) & 0xFF);
    DEV_SPI_WriteByte(Data & 0xFF);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Initialize the lcd register
parameter:
******************************************************************************/
static void LCD_2IN25_InitReg(void)
{
    LCD_2IN25_SendCommand(0x11);
    DEV_Delay_ms(120);

    LCD_2IN25_SendCommand(0xB2);
    LCD_2IN25_SendData_8Bit(0x0C);
    LCD_2IN25_SendData_8Bit(0x0C);
    LCD_2IN25_SendData_8Bit(0x00);
    LCD_2IN25_SendData_8Bit(0x33);
    LCD_2IN25_SendData_8Bit(0x33);

    LCD_2IN25_SendCommand(0xB0);
    LCD_2IN25_SendData_8Bit(0x00);
    LCD_2IN25_SendData_8Bit(0xE0);

    LCD_2IN25_SendCommand(0x3A);
    LCD_2IN25_SendData_8Bit(0x05);

    // MADCTL (0x36) is set by LCD_2IN25_SetAttributes() before this function
    // Do NOT override it here

    LCD_2IN25_SendCommand(0xB7);
    LCD_2IN25_SendData_8Bit(0x45);

    LCD_2IN25_SendCommand(0xBB);
    LCD_2IN25_SendData_8Bit(0x1D);

    LCD_2IN25_SendCommand(0xC0);
    LCD_2IN25_SendData_8Bit(0x2C);

    LCD_2IN25_SendCommand(0xC2);
    LCD_2IN25_SendData_8Bit(0x01);

    LCD_2IN25_SendCommand(0xC3);
    LCD_2IN25_SendData_8Bit(0x19);

    LCD_2IN25_SendCommand(0xC4);
    LCD_2IN25_SendData_8Bit(0x20);

    LCD_2IN25_SendCommand(0xC6);
    LCD_2IN25_SendData_8Bit(0x0F);

    LCD_2IN25_SendCommand(0xD0);
    LCD_2IN25_SendData_8Bit(0xA4);
    LCD_2IN25_SendData_8Bit(0xA1);

    LCD_2IN25_SendCommand(0xD6);
    LCD_2IN25_SendData_8Bit(0xA1);

    LCD_2IN25_SendCommand(0xE0);
    LCD_2IN25_SendData_8Bit(0xD0);
    LCD_2IN25_SendData_8Bit(0x10);
    LCD_2IN25_SendData_8Bit(0x21);
    LCD_2IN25_SendData_8Bit(0x14);
    LCD_2IN25_SendData_8Bit(0x15);
    LCD_2IN25_SendData_8Bit(0x2D);
    LCD_2IN25_SendData_8Bit(0x41);
    LCD_2IN25_SendData_8Bit(0x44);
    LCD_2IN25_SendData_8Bit(0x4F);
    LCD_2IN25_SendData_8Bit(0x28);
    LCD_2IN25_SendData_8Bit(0x0E);
    LCD_2IN25_SendData_8Bit(0x0C);
    LCD_2IN25_SendData_8Bit(0x1D);
    LCD_2IN25_SendData_8Bit(0x1F);

    LCD_2IN25_SendCommand(0xE1);
    LCD_2IN25_SendData_8Bit(0xD0);
    LCD_2IN25_SendData_8Bit(0x0F);
    LCD_2IN25_SendData_8Bit(0x1B);
    LCD_2IN25_SendData_8Bit(0x0D);
    LCD_2IN25_SendData_8Bit(0x0D);
    LCD_2IN25_SendData_8Bit(0x26);
    LCD_2IN25_SendData_8Bit(0x42);
    LCD_2IN25_SendData_8Bit(0x54);
    LCD_2IN25_SendData_8Bit(0x50);
    LCD_2IN25_SendData_8Bit(0x3E);
    LCD_2IN25_SendData_8Bit(0x1A);
    LCD_2IN25_SendData_8Bit(0x18);
    LCD_2IN25_SendData_8Bit(0x22);
    LCD_2IN25_SendData_8Bit(0x25);

    /* DISPON (0x29) is intentionally NOT sent here.  After SLPOUT (0x11) the
     * display output is still OFF; the GRAM contains random power-on data.
     * If we turn the output ON now, that garbage is visible while
     * LCD_2IN25_Clear() fills the GRAM top-to-bottom (~35 ms at 10 MHz SPI),
     * producing a "shadow" flash in the lower half (cleared last).
     * Instead, the caller must LCD_2IN25_Clear() first, then call
     * LCD_2IN25_TurnOnDisplay() to enable the output on a clean frame. */
}

/********************************************************************************
function:	Set the resolution and scanning method of the screen
parameter:
        Scan_dir:   Scan direction
********************************************************************************/
static void LCD_2IN25_SetAttributes(UBYTE Scan_dir)
{
    // Get the screen scan direction
    LCD_2IN25.SCAN_DIR = Scan_dir;
    UBYTE MemoryAccessReg = 0x00;

    // ZJY225KP-PG01: 76(RGB) x 284, ST7789P3
    // Portrait: 76 wide x 284 tall (native orientation)
    // Landscape: 284 wide x 76 tall
    if (Scan_dir == HORIZONTAL)
    {
        // Landscape: swap dimensions
        LCD_2IN25.HEIGHT = LCD_2IN25_WIDTH;   // 76
        LCD_2IN25.WIDTH = LCD_2IN25_HEIGHT;   // 284
        MemoryAccessReg = 0xE0;  // MY=1, MX=1, MV=1, ML=1
    }
    else
    {
        // Portrait: native 76 x 284
        LCD_2IN25.WIDTH = LCD_2IN25_WIDTH;    // 76
        LCD_2IN25.HEIGHT = LCD_2IN25_HEIGHT; // 284
        MemoryAccessReg = 0x00;  // MY=0, MX=0, MV=0
    }

    // Set the read / write scan direction of the frame memory
    LCD_2IN25_SendCommand(0x36);              // MADCTL
    LCD_2IN25_SendData_8Bit(MemoryAccessReg);
}

/********************************************************************************
function :	Initialize the lcd
parameter:
********************************************************************************/
void LCD_2IN25_Init(UBYTE Scan_dir)
{
    // Hardware reset
    LCD_2IN25_Reset();

    // Set the initialization register (includes sleep out)
    LCD_2IN25_InitReg();

    // Set the resolution and scanning method AFTER InitReg
    // (so MADCTL isn't overwritten by InitReg)
    LCD_2IN25_SetAttributes(Scan_dir);
}

/********************************************************************************
function :	Turn on the display output (DISPON 0x29)
parameter:
            Call this AFTER writing the first frame to GRAM (e.g. after
            LCD_2IN25_Clear()) so the panel never shows power-on GRAM garbage.
********************************************************************************/
void LCD_2IN25_TurnOnDisplay(void)
{
    LCD_2IN25_SendCommand(0x29);
}

/********************************************************************************
function:	Sets the start position and size of the display area
parameter:
        Xstart 	:   X direction Start coordinates
        Ystart  :   Y direction Start coordinates
        Xend    :   X direction end coordinates
        Yend    :   Y direction end coordinates
********************************************************************************/
void LCD_2IN25_SetWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend)
{
    // Add GRAM offset for ST7789P3 (240x320 GRAM -> 76x284 screen)
    UWORD xs = Xstart + LCD_2IN25_X_OFFSET;
    UWORD xe = Xend + LCD_2IN25_X_OFFSET - 1;  // CASET is inclusive [xs, xe]
    UWORD ys = Ystart + LCD_2IN25_Y_OFFSET;
    UWORD ye = Yend + LCD_2IN25_Y_OFFSET - 1;  // RASET is inclusive [ys, ye]

    // set the X coordinates (CASET)
    LCD_2IN25_SendCommand(0x2A);
    LCD_2IN25_SendData_8Bit((xs >> 8) & 0xFF);
    LCD_2IN25_SendData_8Bit(xs & 0xFF);
    LCD_2IN25_SendData_8Bit((xe >> 8) & 0xFF);
    LCD_2IN25_SendData_8Bit(xe & 0xFF);

    // set the Y coordinates (RASET)
    LCD_2IN25_SendCommand(0x2B);
    LCD_2IN25_SendData_8Bit((ys >> 8) & 0xFF);
    LCD_2IN25_SendData_8Bit(ys & 0xFF);
    LCD_2IN25_SendData_8Bit((ye >> 8) & 0xFF);
    LCD_2IN25_SendData_8Bit(ye & 0xFF);

    LCD_2IN25_SendCommand(0X2C);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void LCD_2IN25_Clear(UWORD Color)
{
    UWORD j, x;
    /* One reusable scan line instead of a full-screen framebuffer. The old code
     * put UWORD Image[WIDTH*HEIGHT] (~43 KB) on the STACK as a variable-length
     * array; that easily overflows the stack into the heap and corrupts malloc's
     * metadata, which later makes the first malloc() in the slideshow hang/fault
     * (panel freezes on the SD-detect screen). A single line is only ~0.5 KB.
     * Sized to the larger panel dimension so it covers both orientations. */
    UWORD line[LCD_2IN25_HEIGHT];   /* 284 entries max -> ~568 bytes */

    UWORD c = ((Color << 8) & 0xff00) | (Color >> 8);
    for (x = 0; x < LCD_2IN25.WIDTH; x++)
    {
        line[x] = c;
    }

    LCD_2IN25_SetWindows(0, 0, LCD_2IN25.WIDTH, LCD_2IN25.HEIGHT);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    for (j = 0; j < LCD_2IN25.HEIGHT; j++)
    {
        DEV_SPI_Write_nByte((uint8_t *)line, LCD_2IN25.WIDTH * 2);
    }
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Sends the image buffer in RAM to displays
parameter:
******************************************************************************/
void LCD_2IN25_Display(UWORD *Image)
{
    UWORD j;
    LCD_2IN25_SetWindows(0, 0, LCD_2IN25.WIDTH, LCD_2IN25.HEIGHT);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    for (j = 0; j < LCD_2IN25.HEIGHT; j++)
    {
        DEV_SPI_Write_nByte((uint8_t *)&Image[j * LCD_2IN25.WIDTH], LCD_2IN25.WIDTH * 2);
    }
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

void LCD_2IN25_DisplayWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD *Image)
{
    // display
    UDOUBLE Addr = 0;

    UWORD j;
    LCD_2IN25_SetWindows(Xstart, Ystart, Xend, Yend);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    for (j = Ystart; j < Yend; j++)
    {
        Addr = Xstart + j * LCD_2IN25.WIDTH;
        DEV_SPI_Write_nByte((uint8_t *)&Image[Addr], (Xend - Xstart) * 2);
    }
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

void LCD_2IN25_DisplayPoint(UWORD X, UWORD Y, UWORD Color)
{
    LCD_2IN25_SetWindows(X, Y, X, Y);
    LCD_2IN25_SendData_16Bit(Color);
}
