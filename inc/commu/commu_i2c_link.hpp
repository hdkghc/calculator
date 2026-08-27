/** @file inc/commu/commu_i2c_link.hpp
 *  @brief I2C physical layer abstraction with GPIO control for role negotiation
 *  @author hdkghc
 *  @version 0.1
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  SPDX-FileCopyrightText: 2026 hdkghc <peitongxin@outlook.com>
 * 
 *  This program is free software: you can redistribute it and/or modify
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

#ifndef COMMU_I2C_LINK_HPP
#define COMMU_I2C_LINK_HPP

#include <cstdint>
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"

#include "gpio_rt1011.hpp"

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
             * @brief Construct a new I2CLink object
             * @param i2c   I2C peripheral (LPI2C1)
             * @param addr  I2C slave address
             */
            I2CLink(I2C_Type *i2c = LPI2C1, uint8_t addr = 0x42)
                : i2c_(i2c), addr_(addr), sda_(17), scl_(20), rn_(22), tn_(21) {}

            /**
             * @brief Initialize GPIO pins for I2C and detection
             * @param sda  SDA pin number (default: 17 = AD_03)
             * @param scl  SCL pin number (default: 20 = AD_06)
             * @param rn   RN pin number (default: 22 = AD_08)
             * @param tn   TN pin number (default: 21 = AD_07)
             */
            void init_pins(uint32_t sda = 17, uint32_t scl = 20, uint32_t rn = 22, uint32_t tn = 21) {
                sda_ = sda; scl_ = scl; rn_ = rn; tn_ = tn;

                // Detection pins: input with pull-down
                rn_pin_ = gpio::Pin(GPIO1, rn_, gpio::Mode::INPUT_PULLDOWN);
                tn_pin_ = gpio::Pin(GPIO1, tn_, gpio::Mode::INPUT_PULLDOWN);

                // I2C pins: input with pull-up (configured later in enable_master/enable_slave)
                sda_pin_ = gpio::Pin(GPIO1, sda_, gpio::Mode::INPUT_PULLUP);
                scl_pin_ = gpio::Pin(GPIO1, scl_, gpio::Mode::INPUT_PULLUP);
            }

            /**
             * @brief Check if cable is inserted (TN pin pulled low)
             * @return true  Cable inserted
             * @return false No cable
             */
            bool is_cable_inserted() const {
                return tn_pin_.read() == 0;
            }

            /**
             * @brief Set a GPIO pin to high level
             * @param pin  GPIO pin number
             */
            void set_pin_high(uint32_t pin) {
                gpio::Pin p(GPIO1, pin, gpio::Mode::OUTPUT);
                p.write(HIGH);
            }

            /**
             * @brief Set a GPIO pin to low level
             * @param pin  GPIO pin number
             */
            void set_pin_low(uint32_t pin) {
                gpio::Pin p(GPIO1, pin, gpio::Mode::OUTPUT);
                p.write(LOW);
            }

            /**
             * @brief Read a GPIO pin level
             * @param pin  GPIO pin number
             * @return true  High level
             * @return false Low level
             */
            bool read_pin(uint32_t pin) const {
                gpio::Pin p(GPIO1, pin, gpio::Mode::INPUT);
                return p.read();
            }

            /**
             * @brief Get SDA pin number
             * @return uint32_t SDA pin number
             */
            uint32_t sda_pin() const { return sda_; }

            /**
             * @brief Get SCL pin number
             * @return uint32_t SCL pin number
             */
            uint32_t scl_pin() const { return scl_; }

            /**
             * @brief Get TN pin number
             * @return uint32_t TN pin number
             */
            uint32_t tn_pin() const { return tn_; }

            /**
             * @brief Enable I2C master mode
             */
            void enable_master() {
                CLOCK_EnableClock(kCLOCK_Lpi2c1);
                configure_i2c_pins();

                lpi2c_master_config_t masterConfig;
                LPI2C_MasterGetDefaultConfig(&masterConfig);
                masterConfig.baudRate_Hz = 100 * 1000;
                LPI2C_MasterInit(i2c_, &masterConfig, CLOCK_GetFreq(kCLOCK_Lpi2c1));
            }

            /**
             * @brief Enable I2C slave mode
             * @param handler  Slave transfer callback handler
             */
            void enable_slave(lpi2c_slave_transfer_callback_t handler) {
                (void)handler;
                CLOCK_EnableClock(kCLOCK_Lpi2c1);
                configure_i2c_pins();

                lpi2c_slave_config_t slaveConfig;
                LPI2C_SlaveGetDefaultConfig(&slaveConfig);
                slaveConfig.address0 = addr_;
                slaveConfig.enableGeneralCall = false;
                LPI2C_SlaveInit(i2c_, &slaveConfig);
            }

            /**
             * @brief Master write to I2C bus
             * @param data  Data buffer
             * @param len   Data length
             * @return true  Write success
             * @return false Write failed
             */
            bool master_write(const uint8_t *data, size_t len) {
                status_t status = LPI2C_MasterWriteBlocking(i2c_, data, len, addr_,
                                                            kLPI2C_TransferDefaultFlag);
                return status == kStatus_Success;
            }

            /**
             * @brief Master read from I2C bus
             * @param buf         Buffer to store read data
             * @param len         Number of bytes to read
             * @param timeout_ms  Timeout in milliseconds
             * @return true       Read success
             * @return false      Read failed
             */
            bool master_read(uint8_t *buf, size_t len, uint32_t timeout_ms) {
                (void)timeout_ms;
                status_t status = LPI2C_MasterReadBlocking(i2c_, buf, len, addr_,
                                                           kLPI2C_TransferDefaultFlag);
                return status == kStatus_Success;
            }

            /**
             * @brief Get I2C peripheral pointer
             * @return I2C_Type*  I2C peripheral
             */
            I2C_Type *i2c() const { return i2c_; }

        private:
            /**
             * @brief Configure I2C pins with IOMUXC
             */
            void configure_i2c_pins() {
                #ifdef IOMUXC_GPIO_AD_13_LPI2C1_SDA
                    IOMUXC_SetPinMux(IOMUXC_GPIO_AD_13_LPI2C1_SDA, 0U);
                    IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_13_LPI2C1_SDA,
                                        IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                                        IOMUXC_SW_PAD_CTL_PAD_PUS(3U) |
                                        IOMUXC_SW_PAD_CTL_PAD_SPEED(2U) |
                                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U));
                #else
                    #warning "IOMUXC_GPIO_AD_13_LPI2C1_SDA not defined"
                #endif

                #ifdef IOMUXC_GPIO_AD_14_LPI2C1_SCL
                    IOMUXC_SetPinMux(IOMUXC_GPIO_AD_14_LPI2C1_SCL, 0U);
                    IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_14_LPI2C1_SCL,
                                        IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                                        IOMUXC_SW_PAD_CTL_PAD_PUS(3U) |
                                        IOMUXC_SW_PAD_CTL_PAD_SPEED(2U) |
                                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U));
                #else
                    #warning "IOMUXC_GPIO_AD_14_LPI2C1_SCL not defined"
                #endif

                // Re-init GPIO pins with correct mode
                sda_pin_ = gpio::Pin(GPIO1, sda_, gpio::Mode::INPUT_PULLUP);
                scl_pin_ = gpio::Pin(GPIO1, scl_, gpio::Mode::INPUT_PULLUP);
            }

            I2C_Type *i2c_;
            uint8_t addr_;
            uint32_t sda_, scl_, rn_, tn_;

            gpio::Pin sda_pin_;
            gpio::Pin scl_pin_;
            gpio::Pin rn_pin_;
            gpio::Pin tn_pin_;
    };

} // namespace commu

#endif // COMMU_I2C_LINK_HPP