#ifndef DISPLAY_CPP
#define DISPLAY_CPP

extern "C" {
    #include<pico/stdlib.h>
    #include<hardware/spi.h>
}

#include <vector>
#include <utility>
#include <cstdlib>

#include "inc/rgb.h"

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

namespace std {

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

    // ST7735S 1.8" 128x160 TFT initialization sequence
    const uint8_t ST7735S_InitCode[] = {
        0x01, ST7735S_SLPOUT,                     // Exit sleep mode
        0xFF, 120,                                // Delay 120ms
        
        0x06, 0x21, ST7735S_FRMCTR1, 0x05, 0x3C, 0x3C,  // Frame rate control
        0x03, ST7735S_FRMCTR2, 0x05, 0x3C, 0x3C,        // Idle frame rate
        0x03, ST7735S_FRMCTR3, 0x05, 0x3C, 0x3C,        // Partial frame rate
        0x02, ST7735S_INVCTR, 0xC3,                     // Display inversion control
        
        0x05, ST7735S_PWCTR1, 0x28, 0x08, 0x04, 0x00,    // Power control 1
        0x01, ST7735S_PWCTR2, 0x40,                     // Power control 2
        0x02, ST7735S_PWCTR3, 0x00, 0x40,               // Power control 3
        0x02, ST7735S_PWCTR4, 0x80, 0x10,               // Power control 4
        0x02, ST7735S_PWCTR5, 0x80, 0x10,               // Power control 5
        0x01, ST7735S_VMCTR1, 0x1E,                     // VCOM control
        
        0x01, ST7735S_MADCTL, 0x60,            // Memory access control (direction)
        0x01, ST7735S_COLMOD, TFT_COLOR_RGB565,// Pixel format: RGB565 (16bit)
        
        0x02, ST7735S_GMCTRP1, 0x0F, 0x17,     // Positive gamma
        0x0E, 0x15, 0x0B, 0x0C, 0x06, 0x08, 0x07, 0x04,
        0x03, 0x0D, 0x0F, 0x0F, 0x0A,
        
        0x02, ST7735S_GMCTRN1, 0x07, 0x13,     // Negative gamma
        0x0E, 0x14, 0x09, 0x0A, 0x04, 0x06, 0x05, 0x03,
        0x0D, 0x0E, 0x0E, 0x0F,
        
        0x01, ST7735S_DISPON,                  // Turn on display
        0xFF, 10                               // Delay 10ms
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
        void set_addr(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
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
        void DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
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
        void DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
            if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
            uint16_t x2 = x + w - 1;
            uint16_t y2 = y + h - 1;
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
        void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
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
        void DrawPixels(vector<pair<uint16_t, uint16_t>> points, uint16_t color) {
            if (points.empty()) return;

            uint8_t data[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            cs_select(CS);

            for (auto &p : points) {
                uint16_t x = p.first;
                uint16_t y = p.second;
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
    }; // class BlueTFTdisp

} // namespace std

#endif