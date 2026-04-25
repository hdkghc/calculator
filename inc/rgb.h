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

#endif // __RGB_H__