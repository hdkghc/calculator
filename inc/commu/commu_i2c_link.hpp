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
#include <functional>
#include <cstring>
#include <vector>
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"

#include "gpio_rt1011.hpp"

namespace commu {

    /**
     * @brief I2C Slave event callback type
     * @param data      Pointer to received data
     * @param len       Length of received data
     * @param is_write  true for write (master -> slave), false for read (slave -> master)
     * @param user_data User context pointer
     */
    using i2c_slave_callback_t = std::function<void(const uint8_t* data, size_t len,
                                                      bool is_write, void* user_data)>;

    /**
     * @brief I2C physical layer with master/slave support and role negotiation
     *
     * Pin assignments (RT1011 80-pin LQFP):
     * - SDA: GPIO_AD_09 (GPIO1, Pin 23)
     * - SCL: GPIO_AD_10 (GPIO1, Pin 24)
     * - RN:  GPIO_AD_08 (GPIO1, Pin 22) - Ring Normal (cable detect)
     * - TN:  GPIO_AD_07 (GPIO1, Pin 21) - Tip Normal (cable detect)
     *
     * Role negotiation (audio jack hotplug):
     * - First device to insert cable becomes master (pulls SCL low)
     * - Second device becomes slave (pulls SDA low in response)
     * - Uses 3-step handshake to confirm both ends ready
     */
    class I2CLink {
        public:
            /**
             * @brief Construct a new I2CLink object
             * @param i2c   I2C peripheral (LPI2C1)
             * @param addr  I2C slave address (7-bit)
             */
            I2CLink(I2C_Type *i2c = LPI2C1, uint8_t addr = 0x42)
                : i2c_(i2c), addr_(addr),
                  sda_(23), scl_(24), rn_(22), tn_(21),
                  slave_initialized_(false), master_initialized_(false) {}

            /**
             * @brief Initialize GPIO pins for I2C and detection
             * @param sda  SDA pin number (default: 23 = GPIO_AD_09)
             * @param scl  SCL pin number (default: 24 = GPIO_AD_10)
             * @param rn   RN pin number (default: 22 = GPIO_AD_08)
             * @param tn   TN pin number (default: 21 = GPIO_AD_07)
             */
            void init_pins(uint32_t sda = 23, uint32_t scl = 24,
                           uint32_t rn = 22, uint32_t tn = 21) {
                sda_ = sda; scl_ = scl; rn_ = rn; tn_ = tn;

                // Detection pins: input with pull-down
                rn_pin_ = gpio::Pin(GPIO1, rn_, gpio::Mode::INPUT_PULLDOWN);
                tn_pin_ = gpio::Pin(GPIO1, tn_, gpio::Mode::INPUT_PULLDOWN);

                // I2C pins: input with pull-up (configured later)
                sda_pin_ = gpio::Pin(GPIO1, sda_, gpio::Mode::INPUT_PULLUP);
                scl_pin_ = gpio::Pin(GPIO1, scl_, gpio::Mode::INPUT_PULLUP);

                // Cache register pointers for fast GPIO ops
                set_reg_ = (volatile uint32_t*)(GPIO1_BASE + 0x84);  // DR_SET
                clr_reg_ = (volatile uint32_t*)(GPIO1_BASE + 0x88);  // DR_CLEAR
                dr_reg_  = (volatile uint32_t*)(GPIO1_BASE + 0x00);  // DR
                psr_reg_ = (volatile uint32_t*)(GPIO1_BASE + 0x08);  // PSR
            }

            /**
             * @brief Check if cable is inserted
             * @return true  Cable inserted (TN pulled low)
             * @return false No cable
             */
            bool is_cable_inserted() const {
                return !tn_pin_.read();
            }

            /**
             * @brief Check if SCL is high (no master present)
             * @return true  SCL is high (bus free)
             * @return false SCL is low (master is driving)
             */
            bool is_scl_high() const {
                return scl_pin_.read();
            }

            /**
             * @brief Check if SDA is high
             * @return true  SDA is high
             * @return false SDA is low
             */
            bool is_sda_high() const {
                return sda_pin_.read();
            }

            /**
             * @brief Wait for SCL to become high
             * @param timeout_ms  Timeout in milliseconds
             * @return true  SCL became high
             * @return false Timeout
             */
            bool wait_scl_high(uint32_t timeout_ms = 500) {
                uint32_t start = get_tick_ms();
                while (!is_scl_high()) {
                    if (get_tick_ms() - start > timeout_ms) return false;
                }
                return true;
            }

            /**
             * @brief Wait for SDA to become low
             * @param timeout_ms  Timeout in milliseconds
             * @return true  SDA became low
             * @return false Timeout
             */
            bool wait_sda_low(uint32_t timeout_ms = 500) {
                uint32_t start = get_tick_ms();
                while (is_sda_high()) {
                    if (get_tick_ms() - start > timeout_ms) return false;
                }
                return true;
            }

            /**
             * @brief Wait for SDA to become high
             * @param timeout_ms  Timeout in milliseconds
             * @return true  SDA became high
             * @return false Timeout
             */
            bool wait_sda_high(uint32_t timeout_ms = 500) {
                uint32_t start = get_tick_ms();
                while (!is_sda_high()) {
                    if (get_tick_ms() - start > timeout_ms) return false;
                }
                return true;
            }

            // ================================================================
            // Master mode
            // ================================================================

            /**
             * @brief Enable I2C master mode
             * @param baudrate  I2C baud rate in Hz (default 100kHz)
             */
            void enable_master(uint32_t baudrate = 100 * 1000) {
                if (master_initialized_) return;

                CLOCK_EnableClock(kCLOCK_Lpi2c1);
                configure_i2c_pins();

                lpi2c_master_config_t masterConfig;
                LPI2C_MasterGetDefaultConfig(&masterConfig);
                masterConfig.baudRate_Hz = baudrate;
                LPI2C_MasterInit(i2c_, &masterConfig,
                                 CLOCK_GetFreq((clock_name_t)kCLOCK_Lpi2c1));

                master_initialized_ = true;
                slave_initialized_ = false;
            }

            // ================================================================
            // Slave mode
            // ================================================================

            /**
             * @brief Enable I2C slave mode with callback
             * @param callback  Function called when slave receives data or is read
             * @param user_data User context passed to callback
             */
            void enable_slave(i2c_slave_callback_t callback = nullptr,
                              void* user_data = nullptr) {
                if (slave_initialized_) return;

                CLOCK_EnableClock(kCLOCK_Lpi2c1);
                configure_i2c_pins();

                // Save callback
                slave_callback_ = callback;
                slave_user_data_ = user_data;
                slave_rx_buffer_.clear();
                slave_tx_buffer_.clear();
                slave_tx_index_ = 0;

                lpi2c_slave_config_t slaveConfig;
                LPI2C_SlaveGetDefaultConfig(&slaveConfig);
                slaveConfig.address0 = addr_;
                slaveConfig.enableGeneralCall = false;
                slaveConfig.sclStall.enableAddress = true;
                slaveConfig.sclStall.enableRx = true;
                slaveConfig.sclStall.enableTx = true;
                LPI2C_SlaveInit(i2c_, &slaveConfig);

                // Create handle and start non-blocking transfer
                LPI2C_SlaveTransferCreateHandle(i2c_, &slave_handle_,
                                                  slave_event_callback, this);
                LPI2C_SlaveTransferNonBlocking(i2c_, &slave_handle_,
                                                kLPI2C_SlaveAllEvents);

                // Enable interrupt
                NVIC_EnableIRQ(LPI2C1_IRQn);

                slave_initialized_ = true;
                master_initialized_ = false;
            }

            /**
             * @brief Set slave transmit data (response to master read)
             * @param data  Data to send
             * @param len   Length of data
             */
            void slave_set_tx_data(const uint8_t* data, size_t len) {
                slave_tx_buffer_.assign(data, data + len);
                slave_tx_index_ = 0;
            }

            /**
             * @brief Get received slave data (from master write)
             * @return const std::vector<uint8_t>&  Received data
             */
            const std::vector<uint8_t>& slave_get_rx_data() const {
                return slave_rx_buffer_;
            }

            /**
             * @brief Clear received slave data buffer
             */
            void slave_clear_rx_data() {
                slave_rx_buffer_.clear();
            }

            // ================================================================
            // Master I2C operations
            // ================================================================

            /**
             * @brief Master write to I2C bus
             * @param data  Data buffer
             * @param len   Data length
             * @return true  Write success
             * @return false Write failed
             */
            bool master_write(const uint8_t* data, size_t len) {
                if (!master_initialized_ || !is_cable_inserted()) return false;

                status_t status = LPI2C_MasterWriteBlocking(
                    i2c_, data, len, addr_, kLPI2C_TransferDefaultFlag);
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
            bool master_read(uint8_t* buf, size_t len, uint32_t timeout_ms = 100) {
                (void)timeout_ms;
                if (!master_initialized_ || !is_cable_inserted()) return false;

                status_t status = LPI2C_MasterReadBlocking(
                    i2c_, buf, len, addr_, kLPI2C_TransferDefaultFlag);
                return status == kStatus_Success;
            }

            // ================================================================
            // Role negotiation (audio jack hotplug)
            // ================================================================

            /**
             * @brief Try to claim master role (first device to insert cable)
             * @param timeout_ms  Timeout in milliseconds
             * @return true  Master role claimed successfully
             * @return false Failed to claim master role (another device is master)
             */
            bool claim_master(uint32_t timeout_ms = 500) {
                if (!is_cable_inserted()) return false;

                // Step 1: Check if SCL is high (no master present)
                if (!is_scl_high()) {
                    return false;  // Another device is already master
                }

                // Step 2: Pull SCL low to claim master role
                set_scl_low();

                // Step 3: Wait for slave to respond by pulling SDA low
                if (!wait_sda_low(timeout_ms)) {
                    set_scl_high();
                    return false;
                }

                // Step 4: Release SCL high (slave detected)
                set_scl_high();

                // Step 5: Wait for SDA to go high (slave done)
                if (!wait_sda_high(timeout_ms)) {
                    return false;
                }

                return true;
            }

            /**
             * @brief Try to claim slave role (second device to insert cable)
             * @param timeout_ms  Timeout in milliseconds
             * @return true  Slave role claimed successfully
             * @return false Failed to claim slave role
             */
            bool claim_slave(uint32_t timeout_ms = 500) {
                if (!is_cable_inserted()) return false;

                // Step 1: Check if SCL is low (master is driving)
                if (is_scl_high()) {
                    return false;  // No master present
                }

                // Step 2: Pull SDA low to acknowledge slave presence
                set_sda_low();

                // Step 3: Wait for master to release SCL (high)
                if (!wait_scl_high(timeout_ms)) {
                    set_sda_high();
                    return false;
                }

                // Step 4: Release SDA high (master detected)
                set_sda_high();

                return true;
            }

            /**
             * @brief Perform 3-step handshake as master
             * @param timeout_ms  Timeout in milliseconds
             * @return true  Handshake successful
             * @return false Handshake failed
             */
            bool handshake_master(uint32_t timeout_ms = 5000) {
                if (!is_cable_inserted()) return false;

                // Step 1: Send first pulse (SCL low 1ms)
                set_scl_low();
                delay_ms(1);
                set_scl_high();

                // Step 2: Wait for slave response (SDA low)
                if (!wait_sda_low(timeout_ms)) return false;

                // Step 3: Wait for slave to release SDA (high)
                if (!wait_sda_high(timeout_ms)) return false;

                // Step 4: Send second pulse (SCL low 1ms)
                set_scl_low();
                delay_ms(1);
                set_scl_high();

                return true;
            }

            /**
             * @brief Perform 3-step handshake as slave
             * @param timeout_ms  Timeout in milliseconds
             * @return true  Handshake successful
             * @return false Handshake failed
             */
            bool handshake_slave(uint32_t timeout_ms = 5000) {
                if (!is_cable_inserted()) return false;

                // Step 1: Wait for master pulse (SCL low -> high)
                if (!wait_scl_high(timeout_ms)) return false;

                // Step 2: Respond with SDA low 1ms
                set_sda_low();
                delay_ms(1);
                set_sda_high();

                // Step 3: Wait for second pulse (SCL low -> high)
                if (!wait_scl_high(timeout_ms)) return false;

                return true;
            }

            /**
             * @brief Clean up connection (release pins)
             */
            void disconnect() {
                set_scl_high();
                set_sda_high();
                if (slave_initialized_) {
                    LPI2C_SlaveTransferAbort(i2c_, &slave_handle_);
                    slave_initialized_ = false;
                }
                if (master_initialized_) {
                    LPI2C_MasterDeinit(i2c_);
                    master_initialized_ = false;
                }
                slave_rx_buffer_.clear();
                slave_tx_buffer_.clear();
                slave_tx_index_ = 0;
            }

            /**
             * @brief Check if slave mode is initialized
             */
            bool is_slave_ready() const { return slave_initialized_; }

            /**
             * @brief Check if master mode is initialized
             */
            bool is_master_ready() const { return master_initialized_; }

            // ================================================================
            // Getters
            // ================================================================

            uint32_t sda_pin() const { return sda_; }
            uint32_t scl_pin() const { return scl_; }
            uint32_t tn_pin() const { return tn_; }
            I2C_Type* i2c() const { return i2c_; }
            uint8_t slave_address() const { return addr_; }

        private:
            // ================================================================
            // Fast GPIO operations (direct register access)
            // ================================================================

            __attribute__((always_inline))
            inline void set_scl_low() {
                *clr_reg_ = (1UL << scl_);
            }

            __attribute__((always_inline))
            inline void set_scl_high() {
                *set_reg_ = (1UL << scl_);
            }

            __attribute__((always_inline))
            inline void set_sda_low() {
                *clr_reg_ = (1UL << sda_);
            }

            __attribute__((always_inline))
            inline void set_sda_high() {
                *set_reg_ = (1UL << sda_);
            }

            // ================================================================
            // IOMUXC configuration
            // ================================================================

            /**
             * @brief Configure I2C pins with IOMUXC
             *
             * Pin assignments:
             * - SDA: GPIO_AD_09 (GPIO1, Pin 23) -> LPI2C1_SDA
             * - SCL: GPIO_AD_10 (GPIO1, Pin 24) -> LPI2C1_SCL
             */
            void configure_i2c_pins() {
                // GPIO_AD_09 -> LPI2C1_SDA
                #ifdef IOMUXC_GPIO_AD_09_LPI2C1_SDA
                    IOMUXC_SetPinMux(IOMUXC_GPIO_AD_09_LPI2C1_SDA, 0U);
                    IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_09_LPI2C1_SDA,
                                        IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                                        IOMUXC_SW_PAD_CTL_PAD_PUS(3U) |
                                        IOMUXC_SW_PAD_CTL_PAD_SPEED(2U) |
                                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U));
                #else
                    #warning "IOMUXC_GPIO_AD_09_LPI2C1_SDA not defined"
                #endif

                // GPIO_AD_10 -> LPI2C1_SCL
                #ifdef IOMUXC_GPIO_AD_10_LPI2C1_SCL
                    IOMUXC_SetPinMux(IOMUXC_GPIO_AD_10_LPI2C1_SCL, 0U);
                    IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_10_LPI2C1_SCL,
                                        IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                                        IOMUXC_SW_PAD_CTL_PAD_PUS(3U) |
                                        IOMUXC_SW_PAD_CTL_PAD_SPEED(2U) |
                                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U));
                #else
                    #warning "IOMUXC_GPIO_AD_10_LPI2C1_SCL not defined"
                #endif

                // Re-init GPIO pins with correct mode
                sda_pin_ = gpio::Pin(GPIO1, sda_, gpio::Mode::INPUT_PULLUP);
                scl_pin_ = gpio::Pin(GPIO1, scl_, gpio::Mode::INPUT_PULLUP);
            }

            // ================================================================
            // Delay utilities
            // ================================================================

            static inline void delay_ms(uint32_t ms) {
                uint32_t cpu_freq = CLOCK_GetFreq((clock_name_t)0);
                if (cpu_freq == 0) cpu_freq = 500000000;
                volatile uint64_t count = ((uint64_t)cpu_freq / 1000) * ms / 3;
                while (count--) {
                    __NOP();
                }
            }

            static inline uint32_t get_tick_ms() {
                // Simple approximation using CPU cycles
                static uint32_t base = 0;
                uint32_t cpu_freq = CLOCK_GetFreq((clock_name_t)0);
                if (cpu_freq == 0) cpu_freq = 500000000;
                uint32_t cycles = (uint32_t)__get_CCNT();
                if (base == 0) base = cycles;
                return (cycles - base) / (cpu_freq / 1000);
            }

            // ================================================================
            // LPI2C Slave event callback (static wrapper)
            // ================================================================

            static void slave_event_callback(LPI2C_Type* base,
                                              lpi2c_slave_transfer_t* transfer,
                                              void* user_data) {
                (void)base;
                I2CLink* self = static_cast<I2CLink*>(user_data);

                switch (transfer->event) {
                    case kLPI2C_SlaveAddressMatchEvent:
                        // Master addressed us
                        break;

                    case kLPI2C_SlaveReceiveEvent:
                        // Master is writing data to us
                        if (transfer->data && transfer->dataSize > 0) {
                            self->slave_rx_buffer_.insert(
                                self->slave_rx_buffer_.end(),
                                transfer->data,
                                transfer->data + transfer->dataSize);
                        }
                        // Pass data to user callback
                        if (self->slave_callback_) {
                            self->slave_callback_(
                                transfer->data,
                                transfer->dataSize,
                                true,  // is_write = master->slave
                                self->slave_user_data_);
                        }
                        break;

                    case kLPI2C_SlaveTransmitEvent:
                        // Master is reading data from us
                        if (self->slave_tx_index_ < self->slave_tx_buffer_.size()) {
                            size_t remaining = self->slave_tx_buffer_.size() -
                                               self->slave_tx_index_;
                            size_t chunk = remaining < transfer->dataSize ?
                                           remaining : transfer->dataSize;
                            memcpy((void*)transfer->data,
                                   self->slave_tx_buffer_.data() +
                                   self->slave_tx_index_,
                                   chunk);
                            transfer->dataSize = chunk;
                            self->slave_tx_index_ += chunk;
                        } else {
                            transfer->dataSize = 0;
                        }
                        // Pass to user callback
                        if (self->slave_callback_) {
                            self->slave_callback_(
                                transfer->data,
                                transfer->dataSize,
                                false,  // is_write = slave->master
                                self->slave_user_data_);
                        }
                        break;

                    case kLPI2C_SlaveTransmitAckEvent:
                        // Master is asking for ACK/NACK
                        // We always ACK if we have data
                        LPI2C_SlaveTransmitAck(base, self->slave_tx_index_ <
                                               self->slave_tx_buffer_.size());
                        break;

                    case kLPI2C_SlaveCompletionEvent:
                        // Transfer completed
                        self->slave_tx_index_ = 0;
                        break;

                    case kLPI2C_SlaveRepeatedStartEvent:
                        // Repeated start detected
                        break;

                    default:
                        break;
                }
            }

            // ================================================================
            // Member variables
            // ================================================================

            // I2C peripheral
            I2C_Type* i2c_;
            uint8_t addr_;

            // Pin numbers
            uint32_t sda_, scl_, rn_, tn_;

            // GPIO pins
            gpio::Pin sda_pin_;
            gpio::Pin scl_pin_;
            gpio::Pin rn_pin_;
            gpio::Pin tn_pin_;

            // Cached register pointers for fast GPIO ops
            volatile uint32_t* set_reg_;
            volatile uint32_t* clr_reg_;
            volatile uint32_t* dr_reg_;
            volatile uint32_t* psr_reg_;

            // State
            bool master_initialized_;
            bool slave_initialized_;

            // Slave buffers
            std::vector<uint8_t> slave_rx_buffer_;
            std::vector<uint8_t> slave_tx_buffer_;
            size_t slave_tx_index_ = 0;

            // Slave callback
            i2c_slave_callback_t slave_callback_;
            void* slave_user_data_ = nullptr;

            // Slave handle
            lpi2c_slave_handle_t slave_handle_;
    };

} // namespace commu

#endif // COMMU_I2C_LINK_HPP