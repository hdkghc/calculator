/** @file /inc/rgb.h
 *  @brief RGB color manipulation functions for the calculator project
 *  @author hdkghc
 *  @date 2026.05.17
 *  @version 0.1
 *  License: GNU General Public License v3.0
 */

#ifndef __RGB_H__
#define __RGB_H__

#include <cstdint>

uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint8_t getR(uint16_t color) {
    return (color >> 8) & 0xF8;
}
uint8_t getG(uint16_t color) {
    return (color >> 3) & 0xFC;
}
uint8_t getB(uint16_t color) {
    return (color << 3) & 0xF8;
}

enum class Color : uint16_t {
    BLACK = 0x0000,
    WHITE = 0xFFFF,
    RED = 0xF800,
    GREEN = 0x07E0,
    BLUE = 0x001F,
    YELLOW = 0xFFE0,
    CYAN = 0x07FF,
    MAGENTA = 0xF81F,
    ORANGE = 0xFC00,
    PURPLE = 0x8010,
    GRAY = 0x8410,
};

#endif // __RGB_H__