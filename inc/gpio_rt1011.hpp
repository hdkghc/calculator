/** @file    gpio_rt1011.hpp
 *  @brief   Arduino-style GPIO abstraction layer for i.MX RT1011
 *  @details Provides both simple pin control (like Arduino) and low-level
 *          register access for maximum flexibility.
 *  @author hdkghc
 *
 *  @example
 *  @code
 *   gpio::Pin led(GPIO1, 13);           // Create pin object
 *   led.mode(gpio::Mode::OUTPUT);       // Set as output
 *   led.write(HIGH);                    // Output high
 *   bool val = led.read();              // Read level
 *   led.toggle();                       // Toggle output
 *   led = !led;                         // Assignment operator support
 *
 *   gpio::Port port(GPIO2);             // Batch operations
 *   port.setMask(0x00FF0000);           // Batch set
 *   port.clearMask(0x0000FF00);
 *   uint32_t vals = port.read();
 *
 *   // Advanced IOMUXC configuration
 *   gpio::iomuxc::configAD03_SPI();     // Configure AD_03 as SPI function
 *  @endcode
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

#ifndef GPIO_RT1011_HPP
#define GPIO_RT1011_HPP

#include <cstdint>
#include <initializer_list>
#include "fsl_device_registers.h"

/**
 * @def HIGH
 * @brief Logic high level value
 */
#define HIGH 1

/**
 * @def LOW
 * @brief Logic low level value
 */
#define LOW  0

/**
 * @namespace gpio
 * @brief Main namespace for GPIO abstraction
 */
namespace gpio {

    /**
     * @enum Mode
     * @brief Pin mode configuration options
     */
    enum class Mode : uint8_t {
        INPUT,              /**< Input mode (no pull-up/pull-down) */
        INPUT_PULLUP,       /**< Input mode with internal pull-up resistor */
        INPUT_PULLDOWN,     /**< Input mode with internal pull-down resistor */
        OUTPUT,             /**< Push-pull output mode */
        OUTPUT_OPEN_DRAIN,  /**< Open-drain output mode (requires IOMUXC config) */
    };

    /**
     * @enum InterruptMode
     * @brief Interrupt trigger configuration options
     */
    enum class InterruptMode : uint8_t {
        DISABLED    = 0,            /**< Interrupt disabled */
        RISING      = 1 << 0,       /**< Trigger on rising edge */
        FALLING     = 1 << 1,       /**< Trigger on falling edge */
        BOTH        = RISING | FALLING, /**< Trigger on both edges */
        LOW_LEVEL   = 1 << 2,       /**< Trigger on low level */
        HIGH_LEVEL  = 1 << 3,       /**< Trigger on high level */
    };

    /**
     * @typedef PortType
     * @brief GPIO port pointer type
     */
    using PortType = GPIO_Type*;

    // ============================================================
    // Pin Class
    // ============================================================

    /**
     * @class Pin
     * @brief Single GPIO pin abstraction with Arduino-like interface
     */
    class Pin {
        public:
            /**
             * @brief Default constructor - creates an invalid pin
             */
            Pin() : port_(nullptr), pin_(0), valid_(false) {}

            /**
             * @brief Constructor with port, pin number, and optional mode
             * @param port  GPIO port pointer (e.g., GPIO1, GPIO2)
             * @param pin   Pin number (0-31)
             * @param mode  Initial pin mode (default: INPUT)
             */
            Pin(PortType port, uint32_t pin, Mode mode = Mode::INPUT)
                : port_(port), pin_(pin), valid_(port != nullptr) {
                if (valid_) {
                    setMode(mode);
                }
            }

            // ----- Basic Operations -----

            /**
             * @brief Set the pin operating mode
             * @param mode  Desired pin mode (see Mode enum)
             */
            void setMode(Mode mode) {
                if (!valid_) return;

                switch (mode) {
                    case Mode::INPUT:
                    case Mode::INPUT_PULLUP:
                    case Mode::INPUT_PULLDOWN:
                        port_->GDIR &= ~(1UL << pin_);
                        break;
                    case Mode::OUTPUT:
                    case Mode::OUTPUT_OPEN_DRAIN:
                        port_->GDIR |= (1UL << pin_);
                        break;
                }
                write(LOW);
            }

            /**
             * @brief Alias for setMode() for Arduino compatibility
             */
            void mode(Mode mode) { setMode(mode); }

            /**
             * @brief Write logic level to the pin
             * @param val  HIGH (1) or LOW (0)
             */
            void write(bool val) const {
                if (!valid_) return;
                if (val) {
                    port_->DR |= (1UL << pin_);
                } else {
                    port_->DR &= ~(1UL << pin_);
                }
            }

            /**
             * @brief Read current logic level from the pin
             * @return true if pin is HIGH, false if LOW
             */
            bool read() const {
                if (!valid_) return false;
                return (port_->PSR >> pin_) & 0x1;
            }

            /**
             * @brief Set pin to HIGH level
             */
            void setHigh() const { write(HIGH); }

            /**
             * @brief Set pin to LOW level
             */
            void setLow() const { write(LOW); }

            /**
             * @brief Toggle the current output state
             */
            void toggle() const {
                if (valid_) {
                    port_->DR ^= (1UL << pin_);
                }
            }

            // ----- Interrupt Configuration -----

            /**
             * @brief Configure interrupt mode for this pin
             * @param mode  Interrupt trigger mode (see InterruptMode enum)
             * @note This is a simplified interface.
             */
            void interrupt(InterruptMode mode) {
                (void)mode;  // Suppress unused parameter warning
                if (!valid_) return;
                // TODO: Implement interrupt configuration
            }

            // ----- Operator Overloads (Arduino Style) -----

            /**
             * @brief Assignment operator for writing value
             * @param val  HIGH or LOW
             * @return Reference to this pin
             */
            Pin& operator=(bool val) { write(val); return *this; }

            /**
             * @brief Boolean conversion for reading
             * @return true if pin is HIGH
             */
            explicit operator bool() const { return read(); }

            /**
             * @brief Logical NOT operator
             * @return true if pin is LOW
             */
            bool operator!() const { return !read(); }

            // ----- Getters -----

            /**
             * @brief Get the port pointer
             * @return Port pointer (GPIO_Type*)
             */
            PortType port() const { return port_; }

            /**
             * @brief Get the pin number
             * @return Pin number (0-31)
             */
            uint32_t pin() const { return pin_; }

            /**
             * @brief Check if this pin is valid
             * @return true if the pin was initialized with a valid port
             */
            bool valid() const { return valid_; }

        private:
            PortType port_;    /**< GPIO port pointer */
            uint32_t pin_;     /**< Pin number (0-31) */
            bool valid_;       /**< Validity flag */
    };

    // ============================================================
    // Port Class
    // ============================================================

    /**
     * @class Port
     * @brief Batch operation class for GPIO ports
     */
    class Port {
        public:
            /**
             * @brief Default constructor - creates an invalid port
             */
            Port() : port_(nullptr), valid_(false) {}

            /**
             * @brief Constructor with port pointer
             * @param port  GPIO port pointer
             */
            explicit Port(PortType port) : port_(port), valid_(port != nullptr) {}

            // ----- Batch Write Operations -----

            /**
             * @brief Write entire value to the port data register
             * @param value  Value to write (32 bits)
             */
            void write(uint32_t value) const {
                if (valid_) port_->DR = value;
            }

            /**
             * @brief Set specific bits to HIGH
             * @param mask  Bit mask of pins to set
             */
            void setMask(uint32_t mask) const {
                if (valid_) port_->DR |= mask;
            }

            /**
             * @brief Clear specific bits to LOW
             * @param mask  Bit mask of pins to clear
             */
            void clearMask(uint32_t mask) const {
                if (valid_) port_->DR &= ~mask;
            }

            /**
             * @brief Toggle specific bits
             * @param mask  Bit mask of pins to toggle
             */
            void toggleMask(uint32_t mask) const {
                if (valid_) port_->DR ^= mask;
            }

            /**
             * @brief Atomic update: clear then set specific bits
             * @param mask   Bit mask of pins to update
             * @param value  New value for the masked pins
             */
            void writeMask(uint32_t mask, uint32_t value) const {
                if (valid_) {
                    port_->DR = (port_->DR & ~mask) | (value & mask);
                }
            }

            // ----- Read Operations -----

            /**
             * @brief Read entire port value
             * @return Current port state (32 bits)
             */
            uint32_t read() const {
                return valid_ ? port_->PSR : 0;
            }

            /**
             * @brief Read a specific pin from this port
             * @param pin  Pin number (0-31)
             * @return true if pin is HIGH
             */
            bool readBit(uint32_t pin) const {
                return valid_ ? ((port_->PSR >> pin) & 0x1) : false;
            }

            // ----- Direction Control -----

            /**
             * @brief Set pins to input mode
             * @param mask  Bit mask of pins to set as input
             */
            void setInput(uint32_t mask) const {
                if (valid_) port_->GDIR &= ~mask;
            }

            /**
             * @brief Set pins to output mode
             * @param mask  Bit mask of pins to set as output
             */
            void setOutput(uint32_t mask) const {
                if (valid_) port_->GDIR |= mask;
            }

            /**
             * @brief Set pin direction
             * @param mask    Bit mask of pins to configure
             * @param output  true for output, false for input
             */
            void setDirection(uint32_t mask, bool output) const {
                if (valid_) {
                    if (output) {
                        port_->GDIR |= mask;
                    } else {
                        port_->GDIR &= ~mask;
                    }
                }
            }

            // ----- Getters -----

            /**
             * @brief Get the port pointer
             * @return Port pointer (GPIO_Type*)
             */
            PortType port() const { return port_; }

            /**
             * @brief Check if this port is valid
             * @return true if the port was initialized with a valid pointer
             */
            bool valid() const { return valid_; }

        private:
            PortType port_;  /**< GPIO port pointer */
            bool valid_;     /**< Validity flag */
    };

    // ============================================================
    // IOMUXC Helper Namespace
    // ============================================================

    /**
     * @namespace iomuxc
     * @brief IOMUXC configuration helpers for pin multiplexing
     */
    namespace iomuxc {

        /**
         * @brief Configure pin as GPIO (ALT5)
         * @param muxRegister    IOMUXC multiplexing register address
         * @param configRegister IOMUXC configuration register address (optional)
         * @param configValue    Configuration value (optional)
         */
        inline void configGpio(uint32_t muxRegister, uint32_t configRegister = 0, 
                                uint32_t configValue = 0) {
            *(volatile uint32_t*)muxRegister = (*(volatile uint32_t*)muxRegister & ~0x7) | 5;
            if (configRegister) {
                *(volatile uint32_t*)configRegister = configValue;
            }
        }

        /**
         * @brief Generic pin multiplexing configuration
         */
        inline void setMux(uint32_t muxRegister, uint32_t muxMode,
                        uint32_t inputRegister = 0, uint32_t inputDaisy = 0,
                        uint32_t configRegister = 0, uint32_t configValue = 0) {
            *(volatile uint32_t*)muxRegister = (*(volatile uint32_t*)muxRegister & ~0x7) | muxMode;
            
            if (inputRegister) {
                *(volatile uint32_t*)inputRegister = (*(volatile uint32_t*)inputRegister & ~0x7) | inputDaisy;
            }
            
            if (configRegister) {
                *(volatile uint32_t*)configRegister = configValue;
            }
        }

        /**
         * @brief Configure AD_03 pin for LPSPI2_SDI (SPI MISO)
         */
        inline void configAD03_SPI() {
        #ifdef IOMUXC_GPIO_AD_03_LPSPI2_SDI
            IOMUXC_SetPinMux(IOMUXC_GPIO_AD_03_LPSPI2_SDI, 0U);
        #elif defined(IOMUXC_GPIO_AD_03_LPSPI2_SDO)
            IOMUXC_SetPinMux(IOMUXC_GPIO_AD_03_LPSPI2_SDO, 0U);
        #endif
        }

        /**
         * @brief Configure AD_06 pin for LPSPI2_SCK
         */
        inline void configAD06_SPI() {
        #ifdef IOMUXC_GPIO_AD_06_LPSPI2_SCK
            IOMUXC_SetPinMux(IOMUXC_GPIO_AD_06_LPSPI2_SCK, 0U);
        #endif
        }

    } // namespace iomuxc

} // namespace gpio

#endif // GPIO_RT1011_HPP