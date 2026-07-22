/** @file inc/commu/commu_i2c_link.hpp
 *  @brief I2C physical layer abstraction with GPIO control for role negotiation
 *  @author hdkghc
 *  @version 0.1
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  SPDX-FileCopyrightText: 2026 hdkghc <peitongxin@outlook.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMU_I2C_LINK_HPP
#define COMMU_I2C_LINK_HPP

#include <cstdint>
extern "C" {
    #include <pico/stdlib.h>
    #include <hardware/i2c.h>
    #include <hardware/gpio.h>
}


namespace commu {

    /**
     * @brief I2C link layer with GPIO control for role negotiation
     * 
     * Manages the physical pins (SDA, SCL, RN, TN) and provides
     * both master and slave I2C operations.
     * 
     * @note TN (Tip Normal) is used for cable insertion detection.
     *       TN low = cable inserted, TN high = cable removed.
     */
    class I2CLink {
        public:
            /**
             * @brief Constructor
             * @param i2c  I2C hardware instance (i2c0 or i2c1)
             * @param addr Slave address to use (when in slave mode)
             */
            I2CLink(i2c_inst_t *i2c = i2c0, uint8_t addr = 0x42)
                : i2c_(i2c), addr_(addr), sda_(4), scl_(5), rn_(6), tn_(7) {}

            /**
             * @brief Initialize GPIO pins
             * @param sda SDA pin (default 4)
             * @param scl SCL pin (default 5)
             * @param rn  Ring Normal pin (default 6)
             * @param tn  Tip Normal pin (default 7)
             */
            void init_pins(uint sda = 4, uint scl = 5, uint rn = 6, uint tn = 7) {
                sda_ = sda; scl_ = scl; rn_ = rn; tn_ = tn;

                // Detection pins: pull-down, inserted = low
                gpio_init(rn_);
                gpio_set_dir(rn_, GPIO_IN);
                gpio_pull_down(rn_);

                gpio_init(tn_);
                gpio_set_dir(tn_, GPIO_IN);
                gpio_pull_down(tn_);

                // I2C pins: initially inputs with pull-up
                gpio_init(sda_);
                gpio_set_dir(sda_, GPIO_IN);
                gpio_pull_up(sda_);

                gpio_init(scl_);
                gpio_set_dir(scl_, GPIO_IN);
                gpio_pull_up(scl_);
            }

            /**
             * @brief Check if cable is inserted
             * @return true if cable inserted (TN low), false if removed (TN high)
             */
            bool is_cable_inserted() const {
                return gpio_get(tn_) == 0;
            }

            /** @brief Set a GPIO pin high (output) */
            void set_pin_high(uint pin) {
                gpio_set_dir(pin, GPIO_OUT);
                gpio_put(pin, 1);
            }

            /** @brief Set a GPIO pin low (output) */
            void set_pin_low(uint pin) {
                gpio_set_dir(pin, GPIO_OUT);
                gpio_put(pin, 0);
            }

            /** @brief Read a GPIO pin (input) */
            bool read_pin(uint pin) const {
                return gpio_get(pin);
            }

            /** @brief Get SDA pin number */
            uint sda_pin() const { return sda_; }

            /** @brief Get SCL pin number */
            uint scl_pin() const { return scl_; }

            /** @brief Get TN pin number (cable detection) */
            uint tn_pin() const { return tn_; }

            /** @brief Enable I2C master mode (100 kHz) */
            void enable_master() {
                i2c_init(i2c_, 100 * 1000);
                gpio_set_function(sda_, GPIO_FUNC_I2C);
                gpio_set_function(scl_, GPIO_FUNC_I2C);
                gpio_pull_up(sda_);
                gpio_pull_up(scl_);
            }

            /**
             * @brief Enable I2C slave mode with callback
             * @param handler I2C slave event handler
             */
            void enable_slave(i2c_slave_handler_t handler) {
                gpio_set_function(sda_, GPIO_FUNC_I2C);
                gpio_set_function(scl_, GPIO_FUNC_I2C);
                gpio_disable_pulls(sda_);
                gpio_disable_pulls(scl_);
                i2c_set_slave_mode(i2c_, true, addr_);
                i2c_slave_init(i2c_, addr_, handler);
            }

            /**
             * @brief Master write to slave
             * @param data Data buffer
             * @param len  Length in bytes
             * @return     true if all bytes written
             */
            bool master_write(const uint8_t *data, size_t len) {
                return i2c_write_blocking(i2c_, addr_, data, len, false) == (int)len;
            }

            /**
             * @brief Master read from slave
             * @param buf  Output buffer
             * @param len  Number of bytes to read
             * @param timeout_ms Timeout in milliseconds (ignored for simplicity)
             * @return     true if all bytes read
             */
            bool master_read(uint8_t *buf, size_t len, uint32_t timeout_ms) {
                (void)timeout_ms;  // Not used in blocking read
                return i2c_read_blocking(i2c_, addr_, buf, len, false) == (int)len;
            }

            /** @brief Get I2C instance */
            i2c_inst_t *i2c() const { return i2c_; }

        private:
            i2c_inst_t *i2c_;
            uint8_t addr_;
            uint sda_, scl_, rn_, tn_;
    };

} // namespace commu

#endif // COMMU_I2C_LINK_HPP