/** @file /inc/keypadio.hpp
 *  @brief I2C slave interface for receiving keypad data from Nano
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

extern "C" {
    #include <pico/stdlib.h>
    #include <hardware/i2c.h>
}

#include <cstdint>

namespace Keypad {

    /**
     * @brief I2C slave driver for receiving keypad events from Nano
     * 
     * Nano acts as I2C master: on keypress it writes 1 byte then
     * reads back 1 byte (ACK/NAK). Pico polls the I2C peripheral
     * registers in slave mode.
     */
    class KeypadIO {
            static constexpr uint8_t PICO_ADDR = 0x08; ///< I2C slave address
            static constexpr uint8_t ACK       = 0x06; ///< Acknowledge
            static constexpr uint8_t NAK       = 0x15; ///< Negative acknowledge

            i2c_inst_t *i2c;    ///< I2C hardware instance
            uint8_t     sda;    ///< SDA pin
            uint8_t     scl;    ///< SCL pin
            uint8_t     rx_buf; ///< Received byte buffer
            bool        has_data; ///< Valid data pending
            uint8_t     response; ///< Response to send back to Nano

        public:
            /**
             * @brief Constructor
             * @param i2c  I2C instance (default i2c1)
             * @param sda  SDA pin (default 26)
             * @param scl  SCL pin (default 27)
             */
            KeypadIO(i2c_inst_t *i2c = i2c1, uint sda = 26, uint scl = 27)
                : i2c(i2c), sda(sda), scl(scl),
                rx_buf(0), has_data(false), response(NAK) {}

            /**
             * @brief Initialize I2C in slave mode
             * @param freq  Bus frequency in Hz (default 100kHz)
             */
            void init(uint freq = 100 * 1000) {
                i2c_init(i2c, freq);
                gpio_set_function(sda, GPIO_FUNC_I2C);
                gpio_set_function(scl, GPIO_FUNC_I2C);
                i2c_set_slave_mode(i2c, true, PICO_ADDR);
            }

            /**
             * @brief Poll for incoming keypad data
             * 
             * Checks I2C raw interrupt status registers. Nano writes 1 byte
             * then reads 1 byte response. This method handles both phases.
             * 
             * @param row  Output: row index (0-5)
             * @param col  Output: column index (0-5)
             * @return     true if a valid key was received
             */
            bool read(uint8_t &row, uint8_t &col) {
                auto *hw = i2c_get_hw(i2c);

                // Nano wrote data to us
                if (hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_RX_FULL_BITS) {
                    rx_buf   = (uint8_t)hw->data_cmd;
                    has_data = true;
                    response = ACK;
                    hw->clr_rx_done;
                }

                // Nano requests our response
                if (hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_RD_REQ_BITS) {
                    hw->data_cmd = response;
                    hw->clr_rd_req;
                    response = NAK;  // Reset for next transaction
                }

                if (has_data) {
                    has_data = false;
                    row = (rx_buf >> 4) & 0x0F;
                    col = rx_buf & 0x0F;
                    return true;
                }
                return false;
            }
    };

} // namespace Keypad

#endif