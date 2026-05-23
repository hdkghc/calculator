/** @file /src/main.cpp
 *  @brief Main entry point for the calculator project
 *  @author hdkghc
 *  @date 2026.05.17
 *  @version 0.1
 *  License: GNU General Public License v3.0
 */

// #define PROGMEM __attribute__((aligned(4), section(".rodata")))

extern "C" {
    #include <hardware/spi.h>
    #include <pico/stdlib.h>
}
// #include "fonts/CW.h"
// #include "fonts/CWMath.h"
#include "display.hpp"

using namespace std;

int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    Display::BlueTFTdisp display;
    display.InitPin();
    display.InitDisplay();
    display.ClearScreen((uint16_t)Color::BLACK);
    while(1)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(500);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(500);
    }
}
