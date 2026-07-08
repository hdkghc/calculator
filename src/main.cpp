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

extern "C" {
    #include <pico/stdlib.h>
    #include <pico/time.h>
}

#ifndef PROGMEM
#define PROGMEM __attribute__((aligned(4), section(".rodata")))
#endif
#include "dispinterface/stddisplay.hpp"
#include "keypadio.hpp"
#include "../fonts/CW.h"
#include "rgb.h"

#include "expbuild.hpp"
#include "cas/treesimp.hpp"

using namespace std;
using namespace Keypad;

int main() {
    // LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Init display
    Display::RedTFTdisp display;
    display.InitPin();
    display.InitDisplay();
    display.ClearScreen(0x0000);

    // Init keypad I2C
    Keypad::KeypadIO keypad;
    keypad.init();

    char buf[32];

    Expbuild expb;

    while (true) {
        uint8_t row, col;
        if (keypad.read(row, col)) {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);

            display.ClearScreen(0x0000);

            // snprintf(buf, sizeof(buf), "R:%d C:%d", row, col);
            // display.DrawText(0, 12, &ClassWiz_CW_Display_Regular12pt, 1,
            //                 buf, 0xFFFF);

            expb.press(row, col);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            sleep_ms(20);
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            display.DrawText(0, 12, &ClassWiz_CW_Display_Regular12pt, 1,
                            expb.exp.c_str(), ((expb.flg & 7) == 0) ? ((uint16_t)Color::ORANGE) : RGB111_to_RGB565(expb.flg & M_ALPHA, expb.flg & M_SHIFT, expb.flg & M_CTRL));
            display.DrawLine(expb.cp * 9, 0, expb.cp * 9, 12, (expb.flg & M_INSERT) ? (uint16_t)Color::PURPLE : (uint16_t)Color::ORANGE);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        }
        sleep_ms(20);
    }

    return 0;
}