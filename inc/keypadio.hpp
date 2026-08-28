/** @file inc/keypadio.hpp
 *  @brief Matrix keypad scanner for RT1010 using gpio_rt1011.hpp abstraction
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

#ifndef KEYPADIO_HPP
#define KEYPADIO_HPP

#include <cstdint>
#include <cstring>

#include "gpio_rt1011.hpp"

namespace Keypad {

    /**
     * @brief Matrix keypad scanner
     *
     * Pin assignments (RT1010 80-pin LQFP):
     * - Rows R1~R6: GPIO_11~GPIO_06 (GPIO1, Pins 11~6)
     * - Cols C1~C6: GPIO_05~GPIO_00 (GPIO1, Pins 5~0)
     */
    class KeypadIO {
        public:
            static constexpr uint8_t ROWS = 6;
            static constexpr uint8_t COLS = 6;
            static constexpr uint8_t DEBOUNCE_THRESHOLD = 3;

            /**
             * @brief Constructor (I2C params kept for API compatibility, unused)
             */
            KeypadIO(void * = nullptr, uint8_t = 0, uint8_t = 0) {
                memset(key_state_, 0, sizeof(key_state_));
                memset(debounce_counter_, 0, sizeof(debounce_counter_));
            }

            /**
             * @brief Initialize GPIO pins for keypad matrix
             */
            void init(uint32_t = 100 * 1000) {
                // R1=GPIO_11, R2=GPIO_10, ... R6=GPIO_06
                for (uint8_t i = 0; i < ROWS; i++) {
                    rows_pin_[i] = 11 - i;
                    rows_[i] = gpio::Pin(GPIO1, rows_pin_[i], gpio::Mode::OUTPUT);
                    rows_[i].write(HIGH);
                }
                // C1=GPIO_05, C2=GPIO_04, ... C6=GPIO_00
                for (uint8_t i = 0; i < COLS; i++) {
                    cols_pin_[i] = 5 - i;
                    cols_[i] = gpio::Pin(GPIO1, cols_pin_[i], gpio::Mode::INPUT_PULLUP);
                }
            }

            /**
             * @brief Read keypad state (non-blocking, with debounce)
             */
            bool read(uint8_t &row, uint8_t &col) {
                for (uint8_t r = 0; r < ROWS; r++) {
                    rows_[r].write(LOW);
                    for (volatile int i = 0; i < 10; i++) __NOP();

                    for (uint8_t c = 0; c < COLS; c++) {
                        bool pressed = !cols_[c].read();

                        if (pressed) {
                            if (debounce_counter_[r][c] < DEBOUNCE_THRESHOLD) {
                                debounce_counter_[r][c]++;
                            }
                            if (debounce_counter_[r][c] >= DEBOUNCE_THRESHOLD &&
                                !key_state_[r][c]) {
                                key_state_[r][c] = true;
                                row = r;
                                col = c;
                                rows_[r].write(HIGH);
                                return true;
                            }
                        } else {
                            if (key_state_[r][c]) {
                                key_state_[r][c] = false;
                            }
                            debounce_counter_[r][c] = 0;
                        }
                    }

                    rows_[r].write(HIGH);
                }

                row = 0xFF;
                col = 0xFF;
                return false;
            }

        private:
            uint8_t rows_pin_[ROWS];
            uint8_t cols_pin_[COLS];
            gpio::Pin rows_[ROWS];
            gpio::Pin cols_[COLS];
            bool key_state_[ROWS][COLS];
            uint8_t debounce_counter_[ROWS][COLS];
    };

} // namespace Keypad

#endif // KEYPADIO_HPP