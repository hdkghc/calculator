/** @file inc/dispinterface/stddisplay.hpp
 *  @brief Display initialization and control for the calculator project
 *  @author hdkghc
 *  @version 0.1
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
#define ST7735S_NORON   0x13 ///< Normal display mode on

#define ST7735S_MADCTL_MY  0x80 ///< Row address order (bottom to top)
#define ST7735S_MADCTL_MX  0x40 ///< Column address order (right to left)
#define ST7735S_MADCTL_MV  0x20 ///< Row/column exchange
#define ST7735S_MADCTL_RGB 0x00 ///< RGB color filter panel
#define ST7735S_MADCTL_BGR 0x08 ///< BGR color filter panel

// ==================== TFT Display Parameters ====================
// Landscape: MV swaps X/Y, so display is 160x128
#define TFT_WIDTH        160  ///< Display width in pixels (landscape)
#define TFT_HEIGHT       128  ///< Display height in pixels (landscape)
#define TFT_COLOR_RGB565 0x05 ///< 16-bit RGB565 color mode value

// ==================== Font Structure ====================
/**
 * @brief Glyph descriptor for GFX font rendering
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
     * Format: 0xFF = delay, [cmd_len] [cmd] [data...]
     * Based on LCDWIKI library verified initialization values.
     */
    const uint8_t ST7735S_InitCode[] = {
        // 1. Exit sleep mode
        0x01, ST7735S_SLPOUT,
        0xFF, 120,

        // 2. Frame rate control - normal mode
        0x04, ST7735S_FRMCTR1, 0x05, 0x3C, 0x3C,

        // 3. Frame rate control - idle mode
        0x04, ST7735S_FRMCTR2, 0x05, 0x3C, 0x3C,

        // 4. Frame rate control - partial mode
        0x07, ST7735S_FRMCTR3, 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C,

        // 5. Display inversion control
        0x02, ST7735S_INVCTR, 0x03,

        // 6. Power control 1
        0x04, ST7735S_PWCTR1, 0x28, 0x08, 0x04,

        // 7. Power control 2
        0x02, ST7735S_PWCTR2, 0xC0,

        // 8. Power control 3 (normal mode)
        0x03, ST7735S_PWCTR3, 0x0D, 0x00,

        // 9. Power control 4 (idle mode)
        0x03, ST7735S_PWCTR4, 0x8D, 0x2A,

        // 10. Power control 5 (partial mode)
        0x03, ST7735S_PWCTR5, 0x8D, 0xEE,

        // 11. VCOM control
        0x02, ST7735S_VMCTR1, 0x1A,

        // 12. Positive gamma correction
        0x11, ST7735S_GMCTRP1,
        0x03, 0x22, 0x07, 0x0A, 0x2E, 0x30, 0x25, 0x2A,
        0x28, 0x26, 0x2E, 0x3A, 0x00, 0x01, 0x03, 0x13,

        // 13. Negative gamma correction
        0x11, ST7735S_GMCTRN1,
        0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26, 0x23, 0x27,
        0x27, 0x25, 0x2D, 0x3B, 0x00, 0x01, 0x04, 0x13,

        // 14. Set pixel format to 16-bit RGB565
        0x02, ST7735S_COLMOD, 0x05,

        // 15. Memory access control: Landscape mode
        // MV | MX | RGB: Landscape with USB on left side
        0x02, ST7735S_MADCTL, ST7735S_MADCTL_MV | ST7735S_MADCTL_MX | ST7735S_MADCTL_RGB,

        // 16. Normal display mode on
        0x01, ST7735S_NORON,

        // 17. Display on
        0x01, ST7735S_DISPON,
        0xFF, 100
    };

    /**
     * @brief Driver class for ST7735-based TFT displays (Red Tab variant)
     * 
     * Provides low-level display control for the 1.8" 128x160 ST7735 TFT
     * LCD module with red PCB, configured for landscape orientation.
     * 
     * Default pinout:
     * - BLK (backlight): GPIO 22
     * - CS  (chip select): GPIO 17
     * - DC  (data/command): GPIO 21
     * - RST (reset): GPIO 20
     * - SDA (MOSI data): GPIO 19
     * - SCL (clock): GPIO 18
     */
    class RedTFTdisp {
    protected:
        uint8_t BLK;     ///< Backlight control pin
        uint8_t CS;      ///< SPI chip select pin
        uint8_t DC;      ///< Data/command selection pin
        uint8_t RST;     ///< Hardware reset pin
        uint8_t SDA;     ///< SPI MOSI data pin
        uint8_t SCL;     ///< SPI clock pin
        spi_inst_t *SPI; ///< SPI hardware instance

        // Red tab ST7735 requires no offset for this configuration
        static const uint8_t X_OFFSET = 0; ///< Column offset for red tab
        static const uint8_t Y_OFFSET = 0; ///< Row offset for red tab

        /**
         * @brief Set the display memory write window
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
            buf[0] = 0x00;
            buf[1] = x1 + X_OFFSET;
            buf[2] = 0x00;
            buf[3] = x2 + X_OFFSET;
            spi_write_blocking(SPI, buf, 4);

            // Set row address
            gpio_put(DC, 0);
            cmd = ST7735S_RASET;
            spi_write_blocking(SPI, &cmd, 1);
            gpio_put(DC, 1);
            buf[0] = 0x00;
            buf[1] = y1 + Y_OFFSET;
            buf[2] = 0x00;
            buf[3] = y2 + Y_OFFSET;
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
         * @brief Initialize GPIO pins and SPI peripheral
         */
        void InitPin() {
            spi_init(SPI, 20000000); // 20MHz works fine now
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
                    uint8_t ms = ST7735S_InitCode[i++];
                    sleep_ms(ms);
                    continue;
                }
                uint8_t cmd = ST7735S_InitCode[i++];
                gpio_put(DC, 0);
                spi_write_blocking(SPI, &cmd, 1);
                if (cmd_len > 1) {
                    gpio_put(DC, 1);
                    spi_write_blocking(SPI, &ST7735S_InitCode[i], cmd_len - 1);
                    i += cmd_len - 1;
                }
            }
            cs_deselect(CS);
        }

        /**
         * @brief Fill the entire screen with a single color
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
         * @brief Draw multiple pixels using a bounding box optimization
         */
        void DrawPixelsF(std::vector<std::pair<uint8_t, uint8_t>> points, uint16_t color) {
            if (points.empty()) return;

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
         * @brief Draw a single scaled character using a GFX font
         */
        void DrawChar(uint8_t x, uint8_t y, char c, const GFXfont *font, 
                    uint8_t scale, uint16_t color) {
            if (!font || c < font->first || c > font->last) return;

            GFXglyph glyph = font->glyph[c - font->first];
            uint16_t bo = glyph.bitmapOffset; // Byte offset into bitmap array
            uint8_t w = glyph.width;
            uint8_t h = glyph.height;
            uint8_t bytes_per_row = (w + 7) / 8; // Bytes per row in the glyph bitmap
            
            int16_t glyph_x = x + glyph.xOffset * scale;
            int16_t glyph_y = y + glyph.yOffset * scale;
            uint8_t scaled_w = w * scale;
            uint8_t scaled_h = h * scale;

            if (glyph_x + scaled_w <= 0 || glyph_x >= TFT_WIDTH ||
                glyph_y + scaled_h <= 0 || glyph_y >= TFT_HEIGHT) return;

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

            for (uint8_t dy = 0; dy < h; dy++) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    int16_t actual_y = glyph_y + dy * scale + sy;
                    if (actual_y < y1 || actual_y > y2) continue;
                    
                    for (uint8_t dx = 0; dx < w; dx++) {
                        // Byte index = base offset + row * bytes_per_row + column byte
                        uint16_t byte_idx = bo + dy * bytes_per_row + (dx / 8);
                        uint8_t bit_pos = 7 - (dx % 8);
                        bool set = (font->bitmap[byte_idx] >> bit_pos) & 0x01;
                        
                        for (uint8_t sx = 0; sx < scale; sx++) {
                            int16_t actual_x = glyph_x + dx * scale + sx;
                            if (actual_x < x1 || actual_x > x2) continue;
                            spi_write_blocking(SPI, set ? px : blank, 2);
                        }
                    }
                }
            }

            cs_deselect(CS);
        }

        /**
         * @brief Draw a string using a GFX font with scaling
         */
        void DrawText(uint8_t x, uint8_t y, const GFXfont *font, uint8_t scale,
                     std::string text, uint16_t color) {
            if (!font) return;

            uint8_t cursor_x = x;
            uint8_t cursor_y = y;

            for (char c : text) {
                if (c == '\n') {
                    cursor_x = x;
                    cursor_y += font->yAdvance * scale;
                    continue;
                }
                if (c == '\r') continue;
                if (cursor_y >= TFT_HEIGHT) break;

                DrawChar(cursor_x, cursor_y, c, font, scale, color);

                uint16_t idx = c - font->first;
                if (idx <= (font->last - font->first)) {
                    cursor_x += font->glyph[idx].xAdvance * scale;
                }
            }
        }

        /**
        * @brief Draw formatted text with automatic word wrap and color support
        * 
        * Renders a formatted string at the specified position using a given font.
        * Supports automatic line wrapping when text exceeds TFT_WIDTH and stops
        * drawing when cursor exceeds TFT_HEIGHT.
        * 
        * @param x Starting X coordinate (left edge of text)
        * @param y Starting Y coordinate (top edge of text)
        * @param font Pointer to GFXfont structure. If NULL, function returns immediately
        * @param scale Font scaling factor (1 = normal size, 2 = double size, etc.)
        * @param format Format string with supported specifiers:
        *        - %d: Signed integer
        *        - %s: Null-terminated string
        *        - %c: Single character
        *        - %l: Set drawing color (takes uint16_t argument)
        * @param ... Variable arguments matching the format specifiers
        * 
        * @note Special characters in format string:
        *       - '\\n' forces a line break
        *       - '\\r' is ignored
        *       - Automatic line wrap occurs when cursor_x >= TFT_WIDTH
        * 
        * @warning Requires TFT_WIDTH and TFT_HEIGHT to be defined macros
        * @warning Requires GFXfont structure with valid glyph array
        * 
        * @example
        * @code
        * // Basic text drawing
        * DrawTextF(10, 20, &myFont, 1, "Hello %s!", "World");
        * 
        * // Color switching
        * DrawTextF(10, 20, &myFont, 1, "%lError:%l %s", RED, WHITE, "File not found");
        * 
        * // Mixed formats with automatic wrapping
        * DrawTextF(0, 0, &myFont, 2, "Score: %d\nPlayer: %s", 100, "Player1");
        * @endcode
        */
        void DrawTextF(uint8_t x, uint8_t y, const GFXfont *font, uint8_t scale,
                    std::string format, ...) {
            // Return immediately if font is invalid
            if (!font) return;

            // Initialize variable argument list
            va_list args;
            va_start(args, format);

            // Cursor position tracking
            uint8_t cursor_x = x;
            uint8_t cursor_y = y;
            
            // Default drawing color (white)
            uint16_t color = 0xFFFF;

            // Pointer to traverse the format string
            const char* fmt = format.c_str();
            
            // Parse and render each character in the format string
            while (*fmt) {
                // Check for format specifier
                if (*fmt == '%' && *(fmt + 1)) {
                    fmt++; // Skip the '%' character
                    
                    // Process different format specifiers
                    switch (*fmt) {
                        case 'd': {
                            // Integer format specifier
                            int val = va_arg(args, int);
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%d", val);
                            
                            // Draw each digit character
                            for (char* p = buf; *p; p++) {
                                // Stop if exceeded display height
                                if (cursor_y >= TFT_HEIGHT) {
                                    va_end(args);
                                    return;
                                }
                                
                                // Render single character
                                DrawChar(cursor_x, cursor_y, *p, font, scale, color);
                                
                                // Advance cursor by character width
                                uint16_t idx = *p - font->first;
                                if (idx <= (font->last - font->first)) {
                                    cursor_x += font->glyph[idx].xAdvance * scale;
                                }
                                
                                // Check for automatic line wrap
                                if (cursor_x >= TFT_WIDTH) {
                                    cursor_x = x;
                                    cursor_y += font->yAdvance * scale;
                                }
                            }
                            break;
                        }
                        
                        case 's': {
                            // String format specifier
                            const char* str = va_arg(args, const char*);
                            
                            // Draw each character of the string
                            while (*str) {
                                // Stop if exceeded display height
                                if (cursor_y >= TFT_HEIGHT) {
                                    va_end(args);
                                    return;
                                }
                                
                                // Handle newline character
                                if (*str == '\n') {
                                    cursor_x = x;
                                    cursor_y += font->yAdvance * scale;
                                } 
                                // Skip carriage return
                                else if (*str != '\r') {
                                    // Render character
                                    DrawChar(cursor_x, cursor_y, *str, font, scale, color);
                                    
                                    // Advance cursor position
                                    uint16_t idx = *str - font->first;
                                    if (idx <= (font->last - font->first)) {
                                        cursor_x += font->glyph[idx].xAdvance * scale;
                                    }
                                    
                                    // Check for automatic line wrap
                                    if (cursor_x >= TFT_WIDTH) {
                                        cursor_x = x;
                                        cursor_y += font->yAdvance * scale;
                                    }
                                }
                                str++;
                            }
                            break;
                        }
                        
                        case 'c': {
                            // Character format specifier
                            char ch = (char)va_arg(args, int);
                            
                            // Stop if exceeded display height
                            if (cursor_y >= TFT_HEIGHT) {
                                va_end(args);
                                return;
                            }
                            
                            // Render single character
                            DrawChar(cursor_x, cursor_y, ch, font, scale, color);
                            
                            // Advance cursor position
                            uint16_t idx = ch - font->first;
                            if (idx <= (font->last - font->first)) {
                                cursor_x += font->glyph[idx].xAdvance * scale;
                            }
                            
                            // Check for automatic line wrap
                            if (cursor_x >= TFT_WIDTH) {
                                cursor_x = x;
                                cursor_y += font->yAdvance * scale;
                            }
                            break;
                        }
                        
                        case 'l': {
                            // Color set format specifier (custom, uint16_t)
                            color = (uint16_t)va_arg(args, unsigned int);
                            break;
                        }
                        
                        default: {
                            // Unknown format specifier, output literal characters
                            if (cursor_y >= TFT_HEIGHT) {
                                va_end(args);
                                return;
                            }
                            
                            // Draw the '%' character
                            DrawChar(cursor_x, cursor_y, '%', font, scale, color);
                            uint16_t idx = '%' - font->first;
                            if (idx <= (font->last - font->first)) {
                                cursor_x += font->glyph[idx].xAdvance * scale;
                            }
                            
                            // Check wrap after '%'
                            if (cursor_x >= TFT_WIDTH) {
                                cursor_x = x;
                                cursor_y += font->yAdvance * scale;
                            }
                            
                            // Draw the character following '%'
                            if (cursor_y < TFT_HEIGHT) {
                                DrawChar(cursor_x, cursor_y, *fmt, font, scale, color);
                                idx = *fmt - font->first;
                                if (idx <= (font->last - font->first)) {
                                    cursor_x += font->glyph[idx].xAdvance * scale;
                                }
                                
                                // Check wrap after literal character
                                if (cursor_x >= TFT_WIDTH) {
                                    cursor_x = x;
                                    cursor_y += font->yAdvance * scale;
                                }
                            }
                            break;
                        }
                    }
                } else {
                    // Regular character (not a format specifier)
                    
                    // Stop if exceeded display height
                    if (cursor_y >= TFT_HEIGHT) {
                        va_end(args);
                        return;
                    }
                    
                    // Handle special control characters
                    if (*fmt == '\n') {
                        // Line break: reset X, advance Y
                        cursor_x = x;
                        cursor_y += font->yAdvance * scale;
                    } else if (*fmt != '\r') {
                        // Draw normal character (ignore carriage return)
                        DrawChar(cursor_x, cursor_y, *fmt, font, scale, color);
                        
                        // Advance cursor by character width
                        uint16_t idx = *fmt - font->first;
                        if (idx <= (font->last - font->first)) {
                            cursor_x += font->glyph[idx].xAdvance * scale;
                        }
                        
                        // Automatic line wrap when exceeding display width
                        if (cursor_x >= TFT_WIDTH) {
                            cursor_x = x;
                            cursor_y += font->yAdvance * scale;
                        }
                    }
                }
                // Move to next character in format string
                fmt++;
            }

            // Clean up variable argument list
            va_end(args);
        }

        /**
         * @brief Draw a string using a GFX font with scaling
         */
        void DrawTextC(uint8_t x, uint8_t y, const GFXfont *font, uint8_t scale,
                     std::string text, std::vector<uint16_t> color) {
            if (!font) return;

            uint8_t cursor_x = x;
            uint8_t cursor_y = y;

            for (size_t i = 0; i < text.size(); ++i) {
                char c = text[i];
                if (c == '\n') {
                    cursor_x = x;
                    cursor_y += font->yAdvance * scale;
                    continue;
                }
                if (c == '\r') continue;
                if (cursor_y >= TFT_HEIGHT) break;

                DrawChar(cursor_x, cursor_y, c, font, scale, color[i % color.size()]);

                uint16_t idx = c - font->first;
                if (idx <= (font->last - font->first)) {
                    cursor_x += font->glyph[idx].xAdvance * scale;
                }
            }
        }
    }; // class RedTFTdisp

} // namespace Display

#endif // DISPLAY_HPP