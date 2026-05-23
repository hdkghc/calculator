/** @file /inc/display.hpp
 *  @brief Display initialization and control for the calculator project
 *  @author hdkghc
 *  @date 2026.05.17
 *  @version 0.1
 *  License: GNU General Public License v3.0
 */

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

extern "C" {
    #include<pico/stdlib.h>
    #include<hardware/spi.h>
}

#include <vector>
#include <utility>
#include <cstdlib>
#include <string>

#include "rgb.h"

// ==================== TFT ST7735S Command Constants ====================
#define ST7735S_SLPOUT            0x11    // Exit sleep mode
#define ST7735S_FRMCTR1           0xB1    // Frame rate control (normal mode)
#define ST7735S_FRMCTR2           0xB2    // Frame rate control (idle mode)
#define ST7735S_FRMCTR3           0xB3    // Frame rate control (partial mode)
#define ST7735S_INVCTR            0xB4    // Display inversion control
#define ST7735S_PWCTR1            0xC0    // Power control 1
#define ST7735S_PWCTR2            0xC1    // Power control 2
#define ST7735S_PWCTR3            0xC2    // Power control 3
#define ST7735S_PWCTR4            0xC3    // Power control 4
#define ST7735S_PWCTR5            0xC4    // Power control 5
#define ST7735S_VMCTR1            0xC5    // VCOM control
#define ST7735S_MADCTL            0x36    // Memory access control (direction)
#define ST7735S_COLMOD            0x3A    // Pixel format
#define ST7735S_GMCTRP1           0xE0    // Positive gamma correction
#define ST7735S_GMCTRN1           0xE1    // Negative gamma correction
#define ST7735S_DISPON            0x29    // Display on
#define ST7735S_CASET             0x2A    // Column address set
#define ST7735S_RASET             0x2B    // Row address set
#define ST7735S_RAMWR             0x2C    // Memory write
#define ST7735S_MADCTL_MY         0x80    // Row address order
#define ST7735S_MADCTL_MX         0x40    // Column address order
#define ST7735S_MADCTL_MV         0x20    // Row/column exchange
#define ST7735S_MADCTL_RGB        0x00    // RGB color filter
#define ST7735S_MADCTL_BGR        0x08    // BGR color filter

// ==================== TFT Display Parameters ====================
#define TFT_WIDTH                 128
#define TFT_HEIGHT                160
#define TFT_COLOR_RGB565          0x05

// ==================== Font Structure  ====================
typedef struct {
    uint16_t bitmapOffset;
    uint8_t  width;
    uint8_t  height;
    uint8_t  xAdvance;
    int8_t   xOffset;
    int8_t   yOffset;
} GFXglyph;

typedef struct {
    uint8_t  *bitmap;
    GFXglyph *glyph;
    uint16_t first;
    uint16_t last;
    uint8_t  yAdvance;
    const char *subset;
} GFXfont;

namespace Display {

    static inline void cs_select(uint cs_pin) {
        asm volatile("nop \n nop \n nop");
        gpio_put(cs_pin, 0);
        asm volatile("nop \n nop \n nop");
    }

    static inline void cs_deselect(uint cs_pin) {
        asm volatile("nop \n nop \n nop");
        gpio_put(cs_pin, 1);
        asm volatile("nop \n nop \n nop");
    }

    // ==================== ST7735S Initialization Code Sequence ====================
    // Format:
    // 0xFF        = Delay (followed by delay ms)
    // [cmd_len]   = Total bytes: 1 command + N data
    // [command]   = ST7735S command (uses your macros)
    // [data...]   = Parameters
    // ------------------------------------------------------------
    const uint8_t ST7735S_InitCode[] = {
        // 1. Wake up display from sleep
        0x01, ST7735S_SLPOUT,
        0xFF, 150,

        // 2. Frame rate control for normal mode
        0x04, ST7735S_FRMCTR1, 0x00, 0x06, 0x03,

        // 3. Frame rate control for idle mode
        0x04, ST7735S_FRMCTR2, 0x01, 0x06, 0x03,

        // 4. Frame rate control for partial mode
        0x04, ST7735S_FRMCTR3, 0x01, 0x66, 0x03,

        // 5. Display inversion control
        0x02, ST7735S_INVCTR, 0x07,

        // 6. Power control 1
        0x05, ST7735S_PWCTR1, 0xA2, 0x02, 0x84, 0x01,

        // 7. Power control 2
        0x02, ST7735S_PWCTR2, 0xC5,

        // 8. Power control 3
        0x03, ST7735S_PWCTR3, 0x0A, 0x00,

        // 9. Power control 4
        0x03, ST7735S_PWCTR4, 0x8A, 0x2A,

        // 10. Power control 5
        0x03, ST7735S_PWCTR5, 0x8A, 0xEE,

        // 11. VCOM control
        0x02, ST7735S_VMCTR1, 0x3E,

        // 12. Memory access control (BGR color, portrait orientation)
        0x02, ST7735S_MADCTL, ST7735S_MADCTL_BGR | ST7735S_MADCTL_MY,

        // 13. Set pixel format to 16-bit RGB565
        0x02, ST7735S_COLMOD, 0x05,

        // 14. Positive gamma curve
        0x10, ST7735S_GMCTRP1,
        0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22,
        0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,

        // 15. Negative gamma curve
        0x10, ST7735S_GMCTRN1,
        0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E,
        0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10,

        // 16. Turn display ON
        0x01, ST7735S_DISPON,
        0xFF, 100
    };

    class BlueTFTdisp {
    protected:
        uint8_t BLK, CS, DC, RST, SDA, SCL;
        spi_inst_t *SPI;

        /**
         * @brief Set rectangular window for writing RAM
         * @param x1 Start column
         * @param y1 Start row
         * @param x2 End column
         * @param y2 End row
         */
        void set_addr(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
            uint8_t buf[4];
            cs_select(CS);

            gpio_put(DC, 0);
            uint8_t cmd = ST7735S_CASET;
            spi_write_blocking(SPI, &cmd, 1);
            gpio_put(DC, 1);
            buf[0] = 0; buf[1] = x1; buf[2] = 0; buf[3] = x2;
            spi_write_blocking(SPI, buf, 4);

            gpio_put(DC, 0);
            cmd = ST7735S_RASET;
            spi_write_blocking(SPI, &cmd, 1);
            gpio_put(DC, 1);
            buf[0] = 0; buf[1] = y1; buf[2] = 0; buf[3] = y2;
            spi_write_blocking(SPI, buf, 4);

            gpio_put(DC, 0);
            cmd = ST7735S_RAMWR;
            spi_write_blocking(SPI, &cmd, 1);
            gpio_put(DC, 1);
        }

    public:
        BlueTFTdisp(
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
         * @brief Initialize GPIO and SPI for TFT
         */
        void InitPin() {
            spi_init(SPI, 20000000);
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
         * @brief Send initialization commands to TFT controller
         */
        void InitDisplay() {
            gpio_put(RST, 0); // Hardware reset
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
         * @brief Fill entire screen with one color
         * @param color RGB565 color
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
         * @brief Draw one pixel on TFT
         * @param x X coordinate
         * @param y Y coordinate
         * @param color RGB565 color
         */
        void DrawPixel(uint8_t x, uint8_t y, uint16_t color) {
            if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
            set_addr(x, y, x, y);
            uint8_t px[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            spi_write_blocking(SPI, px, 2);
            cs_deselect(CS);
        }

        /**
         * @brief Draw a solid rectangle
         * @param x Start X
         * @param y Start Y
         * @param w Width
         * @param h Height
         * @param color RGB565 color
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
         * @param x0 Start X
         * @param y0 Start Y
         * @param x1 End X
         * @param y1 End Y
         * @param color RGB565 color
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
         * @brief Draw multiple pixels with same color
         * @param points Vector of (x,y) pairs
         * @param color RGB565 color
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
         * @brief Draw multiple pixels with same color faster
         * @param points Vector of (x,y) pairs
         * @param color RGB565 color
         */
        void DrawPixelsF(std::vector<std::pair<uint8_t, uint8_t>> points, uint16_t color) {
            if (points.empty()) return;

            uint8_t min_x = 127, max_x = 0;
            uint8_t min_y = 159, max_y = 0;
            for (auto &p : points) {
                uint8_t x = p.first;
                uint8_t y = p.second;
                if (x >= 128 || y >= 160) continue;
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }

            set_addr(min_x, min_y, max_x, max_y);
            uint8_t px[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };

            for (uint8_t y = min_y; y <= max_y; y++) {
                for (uint8_t x = min_x; x <= max_x; x++) {
                    bool found = false;
                    for (auto &p : points) {
                        if (p.first == x && p.second == y) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        spi_write_blocking(SPI, px, 2);
                    } else {
                        uint8_t blank[2] = {0,0};
                        spi_write_blocking(SPI, blank, 2);
                    }
                }
            }

            cs_deselect(CS);
        }
        /**
         * @brief Draw a scaled character from GFXfont
         * @param x Start X
         * @param y Start Y
         * @param c ASCII character
         * @param font Pointer to GFXfont
         * @param scale Scaling factor (1=normal, 2=2x, etc)
         * @param color Text color
         */
        void DrawChar(uint8_t x, uint8_t y, char c, const GFXfont *font, uint8_t scale, uint16_t color) {
            if ((c < font->first) || (c > font->last)) return;

            GFXglyph glyph = font->glyph[c - font->first];
            uint8_t *bitmap = font->bitmap + glyph.bitmapOffset;

            int8_t glyph_x = x + glyph.xOffset * scale;
            int8_t glyph_y = y + glyph.yOffset * scale;
            uint8_t w = glyph.width * scale;
            uint8_t h = glyph.height * scale;

            uint8_t bits = 0;
            for (uint8_t dy = 0; dy < glyph.height; dy++) {
                for (uint8_t dx = 0; dx < glyph.width; dx++) {
                    uint8_t set = (bitmap[bits / 8] & (0x80 >> (bits % 8))) ? 1 : 0;
                    bits++;
                    if (set) {
                        if (scale == 1) {
                            DrawPixel(glyph_x + dx, glyph_y + dy, color);
                        } else {
                            DrawRect(glyph_x + dx * scale, glyph_y + dy * scale, scale, scale, color);
                        }
                    }
                }
            }
        }

        /**
         * @brief Draw string with custom font, scaling, position
         * @param x Start X
         * @param y Start Y
         * @param font GFXfont pointer
         * @param scale Size scale (1,2,3...)
         * @param text String to display
         * @param color Text color
         * @usage DrawText(10, 20, &myFont, 2, "Hello", RGB565(255,255,255));
         */
        void DrawText(uint8_t x, uint8_t y, const GFXfont *font, uint8_t scale, std::string text, uint16_t color) {
            uint8_t cursor_x = x;
            uint8_t cursor_y = y;

            for (char c : text) {
                if (c == '\n') {
                    cursor_x = x;
                    cursor_y += font->yAdvance * scale;
                    continue;
                }
                if (c == '\r') continue;

                DrawChar(cursor_x, cursor_y, c, font, scale, color);
                GFXglyph glyph = font->glyph[c - font->first];
                cursor_x += glyph.xAdvance * scale;
            }
        }
    }; // class BlueTFTdisp

} // namespace Display

#endif