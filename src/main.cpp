/** @file /src/main.cpp
 *  @brief Main entry point for the calculator project
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
