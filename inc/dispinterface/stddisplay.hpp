/** @file inc/dispinterface/stddisplay.hpp
 *  @brief Display initialization and control for the calculator project
 *  @author hdkghc
 *  @version 0.1
 *  @date 2026
 *  Copyright (C) 2026 hdkghc (peitongxin@outlook.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

extern "C" {
    #include <pico/stdlib.h>
    #include <hardware/spi.h>
}

#include <vector>
#include <utility>
#include <cstdlib>
#include <string>

#include "rgb.h"

// ==================== TFT ST7735S Command Constants ====================
#define ST7735S_SLPOUT  0x11 ///< Exit sleep mode
#define ST7735S_FRMCTR1 0xB1 ///< Frame rate control for normal mode
#define ST7735S_FRMCTR2 0xB2 ///< Frame rate control for idle mode
#define ST7735S_FRMCTR3 0xB3 ///< Frame rate control for partial mode
#define ST7735S_INVCTR  0xB4 ///< Display inversion control
#define ST7735S_PWCTR1  0xC0 ///< Power control 1
#define ST7735S_PWCTR2  0xC1 ///< Power control 2
#define ST7735S_PWCTR3  0xC2 ///< Power control 3 (normal mode)
#define ST7735S_PWCTR4  0xC3 ///< Power control 4 (idle mode)
#define ST7735S_PWCTR5  0xC4 ///< Power control 5 (partial mode)
#define ST7735S_VMCTR1  0xC5 ///< VCOM control
#define ST7735S_MADCTL  0x36 ///< Memory access control (display direction)
#define ST7735S_COLMOD  0x3A ///< Pixel format setting
#define ST7735S_GMCTRP1 0xE0 ///< Positive gamma correction
#define ST7735S_GMCTRN1 0xE1 ///< Negative gamma correction
#define ST7735S_DISPON  0x29 ///< Display on
#define ST7735S_CASET   0x2A ///< Column address set
#define ST7735S_RASET   0x2B ///< Row address set
#define ST7735S_RAMWR   0x2C ///< Memory write
#define ST7735S_SWRESET 0x01 ///< Software reset
#define ST7735S_NORON   0x13 ///< Normal display mode on
#define ST7735S_INVON   0x21 ///< Display inversion on

#define ST7735S_MADCTL_MY  0x80 ///< MADCTL: Row address order (bottom to top)
#define ST7735S_MADCTL_MX  0x40 ///< MADCTL: Column address order (right to left)
#define ST7735S_MADCTL_MV  0x20 ///< MADCTL: Row/column exchange
#define ST7735S_MADCTL_RGB 0x00 ///< MADCTL: RGB color filter panel
#define ST7735S_MADCTL_BGR 0x08 ///< MADCTL: BGR color filter panel

// ==================== TFT Display Parameters ====================
#define TFT_WIDTH        128  ///< Display width in pixels
#define TFT_HEIGHT       160  ///< Display height in pixels
#define TFT_COLOR_RGB565 0x05 ///< 16-bit RGB565 color mode value

// ==================== Font Structure ====================
/**
 * @brief Glyph descriptor for GFX font rendering
 * 
 * Contains positioning and size information for a single character glyph
 * within a GFX font bitmap.
 */
typedef struct {
    uint16_t bitmapOffset; ///< Offset into the font bitmap array
    uint8_t  width;        ///< Width of the glyph in pixels
    uint8_t  height;       ///< Height of the glyph in pixels
    uint8_t  xAdvance;     ///< Horizontal advance to next character
    int8_t   xOffset;      ///< X offset from cursor position
    int8_t   yOffset;      ///< Y offset from cursor baseline
} GFXglyph;

/**
 * @brief GFX font structure for bitmap font rendering
 * 
 * Defines a complete bitmap font including glyph metrics and pixel data.
 * Compatible with Adafruit GFX font format.
 */
typedef struct {
    uint8_t  *bitmap;  ///< Pointer to the font bitmap data
    GFXglyph *glyph;   ///< Array of glyph descriptors
    uint16_t first;    ///< First character code in the font
    uint16_t last;     ///< Last character code in the font
    uint8_t  yAdvance; ///< Vertical line spacing
    const char *subset;///< Optional font subset name
} GFXfont;

namespace Display {

    /**
     * @brief Select the SPI chip select pin (active low)
     * @param cs_pin GPIO pin number for chip select
     */
    static inline void cs_select(uint cs_pin) {
        asm volatile("nop \n nop \n nop");
        gpio_put(cs_pin, 0);
        asm volatile("nop \n nop \n nop");
    }

    /**
     * @brief Deselect the SPI chip select pin (active high)
     * @param cs_pin GPIO pin number for chip select
     */
    static inline void cs_deselect(uint cs_pin) {
        asm volatile("nop \n nop \n nop");
        gpio_put(cs_pin, 1);
        asm volatile("nop \n nop \n nop");
    }

    /**
     * @brief ST7735S initialization sequence for Red Tab 1.8" display
     * 
     * Format specification:
     * - 0xFF followed by delay value (milliseconds)
     * - Otherwise: [command_length] [command_byte] [data_bytes...]
     *   where command_length includes the command byte itself
     * 
     * This sequence is optimized for the red PCB variant of the
     * 1.8" 128x160 ST7735 TFT display module.
     */
    const uint8_t ST7735S_InitCode[] = {
        // 1. Software reset
        0x01, ST7735S_SWRESET,
        0xFF, 150,

        // 2. Exit sleep mode
        0x01, ST7735S_SLPOUT,
        0xFF, 150,

        // 3. Frame rate control - normal mode
        0x04, ST7735S_FRMCTR1, 0x01, 0x2C, 0x2D,

        // 4. Frame rate control - idle mode
        0x04, ST7735S_FRMCTR2, 0x01, 0x2C, 0x2D,

        // 5. Frame rate control - partial mode
        0x07, ST7735S_FRMCTR3, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,

        // 6. Display inversion control
        0x02, ST7735S_INVCTR, 0x07,

        // 7. Power control 1
        0x04, ST7735S_PWCTR1, 0xA2, 0x02, 0x84,

        // 8. Power control 2
        0x02, ST7735S_PWCTR2, 0xC5,

        // 9. Power control 3 (normal mode)
        0x03, ST7735S_PWCTR3, 0x0A, 0x00,

        // 10. Power control 4 (idle mode)
        0x03, ST7735S_PWCTR4, 0x8A, 0x2A,

        // 11. Power control 5 (partial mode)
        0x03, ST7735S_PWCTR5, 0x8A, 0xEE,

        // 12. VCOM control
        0x02, ST7735S_VMCTR1, 0x0E,

        // 13. Memory access control
        // MX + MV + BGR: Portrait mode with USB connector at bottom
        0x02, ST7735S_MADCTL, ST7735S_MADCTL_MX | ST7735S_MADCTL_MV | ST7735S_MADCTL_BGR,

        // 14. Set pixel format to 16-bit RGB565
        0x02, ST7735S_COLMOD, 0x05,

        // 15. Enable display inversion for red tab
        0x01, ST7735S_INVON,

        // 16. Positive gamma correction
        0x10, ST7735S_GMCTRP1,
        0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
        0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,

        // 17. Negative gamma correction
        0x10, ST7735S_GMCTRN1,
        0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,

        // 18. Normal display mode on
        0x01, ST7735S_NORON,

        // 19. Display on
        0x01, ST7735S_DISPON,
        0xFF, 100
    };

    /**
     * @brief Driver class for ST7735-based TFT displays (Red Tab variant)
     * 
     * Provides low-level display control for the 1.8" 128x160 ST7735 TFT
     * LCD module with red PCB. Supports pixel drawing, rectangles, lines,
     * bitmap font text rendering, and optimized batch pixel operations.
     * 
     * Default pinout matches the common red tab module wiring:
     * - BLK (backlight): GPIO 22
     * - CS  (chip select): GPIO 17
     * - DC  (data/command): GPIO 21
     * - RST (reset): GPIO 20
     * - SDA (MOSI data): GPIO 19
     * - SCL (clock): GPIO 18
     * - SPI instance: spi0
     */
    class RedTFTdisp {
    protected:
        uint8_t BLK;     ///< Backlight control pin
        uint8_t CS;      ///< SPI chip select pin
        uint8_t DC;      ///< Data/command selection pin
        uint8_t RST;     ///< Hardware reset pin
        uint8_t SDA;     ///< SPI MOSI data pin
        uint8_t SCL;     ///< SPI clock pin
        spi_inst_t *SPI; ///< SPI hardware instance (spi0 or spi1)

        /**
         * @brief Set the display memory write window
         * 
         * Configures the rectangular region of the display's GRAM
         * that subsequent memory writes will fill. Must be called
         * before any RAMWR operation.
         * 
         * @param x1 Start column address (0 to TFT_WIDTH-1)
         * @param y1 Start row address (0 to TFT_HEIGHT-1)
         * @param x2 End column address (0 to TFT_WIDTH-1)
         * @param y2 End row address (0 to TFT_HEIGHT-1)
         */
        void set_addr(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
            uint8_t buf[4];
            cs_select(CS);

            // Set column address
            gpio_put(DC, 0);
            uint8_t cmd = ST7735S_CASET;
            spi_write_blocking(SPI, &cmd, 1);
            gpio_put(DC, 1);
            buf[0] = 0; buf[1] = x1; buf[2] = 0; buf[3] = x2;
            spi_write_blocking(SPI, buf, 4);

            // Set row address
            gpio_put(DC, 0);
            cmd = ST7735S_RASET;
            spi_write_blocking(SPI, &cmd, 1);
            gpio_put(DC, 1);
            buf[0] = 0; buf[1] = y1; buf[2] = 0; buf[3] = y2;
            spi_write_blocking(SPI, buf, 4);

            // Start memory write
            gpio_put(DC, 0);
            cmd = ST7735S_RAMWR;
            spi_write_blocking(SPI, &cmd, 1);
            gpio_put(DC, 1);
        }

    public:
        /**
         * @brief Construct a Red Tab TFT display driver instance
         * 
         * @param blk Backlight control pin (default: 22)
         * @param cs  Chip select pin (default: 17)
         * @param dc  Data/command pin (default: 21)
         * @param rst Reset pin (default: 20)
         * @param sda SPI MOSI data pin (default: 19)
         * @param scl SPI clock pin (default: 18)
         * @param spi SPI hardware instance (default: spi0)
         */
        RedTFTdisp(
            uint8_t blk = 22, 
            uint8_t cs = 17, 
            uint8_t dc = 21, 
            uint8_t rst = 20, 
            uint8_t sda = 19, 
            uint8_t scl = 18, 
            spi_inst_t *spi = spi0
        ) : 
            SPI(spi),
            BLK(blk), CS(cs), DC(dc), 
            RST(rst), SDA(sda), SCL(scl) 
            {}

        /**
         * @brief Initialize GPIO pins and SPI peripheral for TFT communication
         * 
         * Configures SPI at 40 MHz (suitable for red tab modules),
         * sets up all control pins as outputs, and enables the backlight.
         * Must be called once before any display operations.
         */
        void InitPin() {
            spi_init(SPI, 40000000);
            gpio_set_function(SDA, GPIO_FUNC_SPI);
            gpio_set_function(SCL, GPIO_FUNC_SPI);
            gpio_init(BLK);
            gpio_set_dir(BLK, GPIO_OUT);
            gpio_put(BLK, 1);
            gpio_init(CS);
            gpio_set_dir(CS, GPIO_OUT);
            gpio_put(CS, 1);
            gpio_init(DC);
            gpio_set_dir(DC, GPIO_OUT);
            gpio_put(DC, 1);
            gpio_init(RST);
            gpio_set_dir(RST, GPIO_OUT);
            gpio_put(RST, 1);
        }

        /**
         * @brief Send initialization commands to the ST7735S controller
         * 
         * Performs hardware reset followed by the complete initialization
         * sequence including power control, gamma correction, display
         * orientation, and pixel format configuration.
         */
        void InitDisplay() {
            // Hardware reset
            gpio_put(RST, 0);
            sleep_ms(10);
            gpio_put(RST, 1);
            sleep_ms(120);

            cs_select(CS);
            size_t i = 0;
            while (i < sizeof(ST7735S_InitCode)) {
                uint8_t cmd_len = ST7735S_InitCode[i++];
                if (cmd_len == 0xFF) {
                    // Delay command
                    uint8_t ms = ST7735S_InitCode[i++];
                    sleep_ms(ms);
                    continue;
                }
                uint8_t cmd = ST7735S_InitCode[i++];
                gpio_put(DC, 0); // Command mode
                spi_write_blocking(SPI, &cmd, 1);
                if (cmd_len > 1) {
                    gpio_put(DC, 1); // Data mode
                    spi_write_blocking(SPI, &ST7735S_InitCode[i], cmd_len - 1);
                    i += cmd_len - 1;
                }
            }
            cs_deselect(CS);
        }

        /**
         * @brief Fill the entire screen with a single color
         * 
         * @param color 16-bit RGB565 color value
         */
        void ClearScreen(uint16_t color) {
            cs_select(CS);
            set_addr(0, 0, TFT_WIDTH-1, TFT_HEIGHT-1);
            uint8_t px[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            for (int i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
                spi_write_blocking(SPI, px, 2);
            }
            cs_deselect(CS);
        }

        /**
         * @brief Draw a single pixel on the display
         * 
         * Pixels outside the visible area are silently ignored.
         * 
         * @param x X coordinate (0 to TFT_WIDTH-1)
         * @param y Y coordinate (0 to TFT_HEIGHT-1)
         * @param color 16-bit RGB565 color value
         */
        void DrawPixel(uint8_t x, uint8_t y, uint16_t color) {
            if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
            set_addr(x, y, x, y);
            uint8_t px[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            spi_write_blocking(SPI, px, 2);
            cs_deselect(CS);
        }

        /**
         * @brief Draw a filled rectangle on the display
         * 
         * Automatically clips the rectangle to the visible display area.
         * 
         * @param x Left edge X coordinate
         * @param y Top edge Y coordinate
         * @param w Rectangle width in pixels
         * @param h Rectangle height in pixels
         * @param color 16-bit RGB565 color value
         */
        void DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
            if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
            uint8_t x2 = x + w - 1;
            uint8_t y2 = y + h - 1;
            if (x2 >= TFT_WIDTH)  x2 = TFT_WIDTH - 1;
            if (y2 >= TFT_HEIGHT) y2 = TFT_HEIGHT - 1;
            set_addr(x, y, x2, y2);
            uint8_t px[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            uint32_t cnt = (uint32_t)(x2 - x + 1) * (y2 - y + 1);
            for (uint32_t i = 0; i < cnt; i++) {
                spi_write_blocking(SPI, px, 2);
            }
            cs_deselect(CS);
        }

        /**
         * @brief Draw a straight line using Bresenham's algorithm
         * 
         * @param x0 Start X coordinate
         * @param y0 Start Y coordinate
         * @param x1 End X coordinate
         * @param y1 End Y coordinate
         * @param color 16-bit RGB565 color value
         */
        void DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
            int dx = abs((int)x1 - (int)x0);
            int dy = -abs((int)y1 - (int)y0);
            int sx = (x0 < x1) ? 1 : -1;
            int sy = (y0 < y1) ? 1 : -1;
            int err = dx + dy;
            while (true) {
                DrawPixel(x0, y0, color);
                if (x0 == x1 && y0 == y1) break;
                int e2 = 2 * err;
                if (e2 >= dy) { err += dy; x0 += sx; }
                if (e2 <= dx) { err += dx; y0 += sy; }
            }
        }

        /**
         * @brief Draw multiple individual pixels with the same color
         * 
         * Each pixel is drawn with its own CASET/RASET/RAMWR sequence,
         * which is slower but allows scattered pixel locations.
         * For clustered pixels, consider using DrawPixelsF() instead.
         * 
         * @param points Vector of (x,y) coordinate pairs
         * @param color 16-bit RGB565 color value
         */
        void DrawPixels(std::vector<std::pair<uint8_t, uint8_t>> points, uint16_t color) {
            if (points.empty()) return;

            uint8_t data[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            cs_select(CS);

            for (auto &p : points) {
                uint8_t x = p.first;
                uint8_t y = p.second;
                if (x >= TFT_WIDTH || y >= TFT_HEIGHT) continue;

                // Set column
                gpio_put(DC, 0);
                uint8_t cmd = ST7735S_CASET;
                spi_write_blocking(SPI, &cmd, 1);
                gpio_put(DC, 1);
                uint8_t col[4] = {0, x, 0, x};
                spi_write_blocking(SPI, col, 4);

                // Set row
                gpio_put(DC, 0);
                cmd = ST7735S_RASET;
                spi_write_blocking(SPI, &cmd, 1);
                gpio_put(DC, 1);
                uint8_t row[4] = {0, y, 0, y};
                spi_write_blocking(SPI, row, 4);

                // Write pixel
                gpio_put(DC, 0);
                cmd = ST7735S_RAMWR;
                spi_write_blocking(SPI, &cmd, 1);
                gpio_put(DC, 1);
                spi_write_blocking(SPI, data, 2);
            }

            cs_deselect(CS);
        }

        /**
         * @brief Draw multiple pixels using a bounding box optimization
         * 
         * Calculates the bounding rectangle of all provided points and
         * writes the entire region with color data or blanks. This is
         * significantly faster than DrawPixels() for clustered points.
         * 
         * Best suited for font rendering and small sprite drawing.
         * 
         * @param points Vector of (x,y) coordinate pairs
         * @param color 16-bit RGB565 color value
         */
        void DrawPixelsF(std::vector<std::pair<uint8_t, uint8_t>> points, uint16_t color) {
            if (points.empty()) return;

            // Find bounding box
            uint8_t min_x = TFT_WIDTH - 1, max_x = 0;
            uint8_t min_y = TFT_HEIGHT - 1, max_y = 0;
            for (auto &p : points) {
                uint8_t x = p.first;
                uint8_t y = p.second;
                if (x >= TFT_WIDTH || y >= TFT_HEIGHT) continue;
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }

            set_addr(min_x, min_y, max_x, max_y);
            uint8_t px[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            uint8_t blank[2] = {0, 0};

            // Write bounding box region
            for (uint8_t y = min_y; y <= max_y; y++) {
                for (uint8_t x = min_x; x <= max_x; x++) {
                    bool found = false;
                    for (auto &p : points) {
                        if (p.first == x && p.second == y) {
                            found = true;
                            break;
                        }
                    }
                    spi_write_blocking(SPI, found ? px : blank, 2);
                }
            }

            cs_deselect(CS);
        }

        /**
         * @brief Draw a single scaled character using a GFX font with optimized batch rendering
         * 
         * Renders a character glyph from the specified GFX font at the
         * given position with optional integer scaling. Uses batch pixel
         * operations for improved performance over per-pixel drawing.
         * Characters outside the font's character range are silently ignored.
         * 
         * @param x Cursor X position (left edge of character cell)
         * @param y Cursor Y position (baseline of character cell)
         * @param c ASCII character to render
         * @param font Pointer to the GFXfont structure
         * @param scale Integer scaling factor (1 = original size)
         * @param color 16-bit RGB565 color value
         */
        void DrawChar(uint8_t x, uint8_t y, char c, const GFXfont *font, 
                      uint8_t scale, uint16_t color) {
            if (!font || c < font->first || c > font->last) return;

            GFXglyph glyph = font->glyph[c - font->first];
            
            int8_t glyph_x = x + glyph.xOffset * scale;
            int8_t glyph_y = y + glyph.yOffset * scale;
            uint8_t w = glyph.width;
            uint8_t h = glyph.height;
            uint8_t scaled_w = w * scale;
            uint8_t scaled_h = h * scale;

            // Skip if completely off-screen
            if (glyph_x + scaled_w <= 0 || glyph_x >= TFT_WIDTH ||
                glyph_y + scaled_h <= 0 || glyph_y >= TFT_HEIGHT) return;

            // Clamp to display boundaries
            uint8_t x1 = (glyph_x < 0) ? 0 : glyph_x;
            uint8_t y1 = (glyph_y < 0) ? 0 : glyph_y;
            uint8_t x2 = glyph_x + scaled_w - 1;
            uint8_t y2 = glyph_y + scaled_h - 1;
            if (x2 >= TFT_WIDTH)  x2 = TFT_WIDTH - 1;
            if (y2 >= TFT_HEIGHT) y2 = TFT_HEIGHT - 1;

            cs_select(CS);
            set_addr(x1, y1, x2, y2);

            uint8_t px[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            uint8_t blank[2] = {0, 0};
            uint16_t bo = glyph.bitmapOffset;

            // Render glyph directly to display GRAM for better performance
            for (uint8_t dy = 0; dy < h; dy++) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    uint8_t actual_y = glyph_y + dy * scale + sy;
                    if (actual_y < y1 || actual_y > y2) continue;
                    
                    for (uint8_t dx = 0; dx < w; dx++) {
                        uint16_t bit_idx = bo * 8 + dy * w + dx;
                        uint8_t byte_val = font->bitmap[bit_idx / 8];
                        bool set = (byte_val >> (7 - (bit_idx % 8))) & 0x01;
                        
                        for (uint8_t sx = 0; sx < scale; sx++) {
                            uint8_t actual_x = glyph_x + dx * scale + sx;
                            if (actual_x < x1 || actual_x > x2) continue;
                            spi_write_blocking(SPI, set ? px : blank, 2);
                        }
                    }
                }
            }

            cs_deselect(CS);
        }

        /**
         * @brief Draw a string using a GFX font with scaling and optimized rendering
         * 
         * Renders a null-terminated string using the specified GFX font.
         * Supports newline characters (\\n) for multi-line text.
         * Carriage return characters (\\r) are ignored.
         * Uses optimized batch rendering for better performance.
         * 
         * @param x Starting X coordinate
         * @param y Starting Y coordinate (baseline)
         * @param font Pointer to the GFXfont structure
         * @param scale Integer scaling factor (1 = original size)
         * @param text String to render
         * @param color 16-bit RGB565 color value
         * 
         * @code
         * DrawText(10, 20, &myFont, 2, "Hello World!", RGB565(255,255,255));
         * @endcode
         */
        void DrawText(uint8_t x, uint8_t y, const GFXfont *font, uint8_t scale,
                     std::string text, uint16_t color) {
            if (!font) return;

            uint8_t cursor_x = x;
            uint8_t cursor_y = y;
            uint8_t line_height = font->yAdvance * scale;

            for (char c : text) {
                if (c == '\n') {
                    cursor_x = x;
                    cursor_y += line_height;
                    continue;
                }
                if (c == '\r') continue;
                if (cursor_y >= TFT_HEIGHT) break; // Stop if beyond screen

                DrawChar(cursor_x, cursor_y, c, font, scale, color);

                uint16_t idx = c - font->first;
                if (idx < (font->last - font->first + 1)) {
                    cursor_x += font->glyph[idx].xAdvance * scale;
                }
            }
        }

        /**
         * @brief Draw a string with automatic text wrapping
         * 
         * Renders text that automatically wraps to the next line when
         * exceeding the specified maximum width. Words are wrapped
         * at character boundaries.
         * 
         * @param x Starting X coordinate
         * @param y Starting Y coordinate (baseline)
         * @param max_width Maximum pixel width before wrapping
         * @param font Pointer to the GFXfont structure
         * @param scale Integer scaling factor (1 = original size)
         * @param text String to render
         * @param color 16-bit RGB565 color value
         */
        void DrawTextWrapped(uint8_t x, uint8_t y, uint8_t max_width,
                            const GFXfont *font, uint8_t scale,
                            std::string text, uint16_t color) {
            if (!font) return;

            uint8_t cursor_x = x;
            uint8_t cursor_y = y;
            uint8_t line_height = font->yAdvance * scale;
            uint8_t word_width = 0;
            std::string word;

            for (char c : text) {
                if (c == ' ' || c == '\n') {
                    // Check if word fits on current line
                    if (cursor_x + word_width > x + max_width && cursor_x > x) {
                        cursor_x = x;
                        cursor_y += line_height;
                    }

                    // Render the word
                    for (char wc : word) {
                        DrawChar(cursor_x, cursor_y, wc, font, scale, color);
                        uint16_t idx = wc - font->first;
                        if (idx < (font->last - font->first + 1)) {
                            cursor_x += font->glyph[idx].xAdvance * scale;
                        }
                    }

                    word.clear();
                    word_width = 0;

                    if (c == ' ') {
                        // Draw space
                        cursor_x += font->glyph[' ' - font->first].xAdvance * scale;
                    } else if (c == '\n') {
                        cursor_x = x;
                        cursor_y += line_height;
                    }
                } else {
                    word += c;
                    uint16_t idx = c - font->first;
                    if (idx < (font->last - font->first + 1)) {
                        word_width += font->glyph[idx].xAdvance * scale;
                    }
                }

                if (cursor_y >= TFT_HEIGHT) break;
            }

            // Render remaining word if any
            if (!word.empty()) {
                if (cursor_x + word_width > x + max_width && cursor_x > x) {
                    cursor_x = x;
                    cursor_y += line_height;
                }
                for (char wc : word) {
                    if (cursor_y >= TFT_HEIGHT) break;
                    DrawChar(cursor_x, cursor_y, wc, font, scale, color);
                    uint16_t idx = wc - font->first;
                    if (idx < (font->last - font->first + 1)) {
                        cursor_x += font->glyph[idx].xAdvance * scale;
                    }
                }
            }
        }
    }; // class RedTFTdisp

} // namespace Display

#endif // DISPLAY_HPP