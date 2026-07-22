/** @file inc/commu/commu_audio.hpp
 *  @brief Audio jack I2C communication with master/slave negotiation
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

#ifndef COMMU_AUDIO_HPP
#define COMMU_AUDIO_HPP

#include "commu_types.hpp"
#include "commu_packet.hpp"
#include "commu_queue.hpp"
#include "commu_i2c_link.hpp"
#include <cstdint>

// Forward declaration for SDCard::FATFS (if using file transfer)
// You can include your sdcard.hpp here if needed, or use a forward declaration.
// For this example, we'll keep it minimal.

namespace commu {

    /**
     * @brief Audio jack communication manager
     * 
     * Implements a state machine for:
     *   - Cable insertion detection (via TN pin)
     *   - Dynamic master/slave role negotiation (via SDA/SCL)
     *   - Handshake procedure
     *   - I2C communication establishment
     *   - File transfer (send/receive)
     *   - Graceful disconnection
     * 
     * Both Picos run identical code. The first to insert the cable
     * becomes the master.
     * 
     * @note All send/receive operations check cable connection (GPIO7/TN)
     *       before proceeding. If cable is removed, operation fails and
     *       state transitions to DISCONNECTING.
     * 
     * @code
     * // Example usage
     * #include "commu/commu_audio.hpp"
     * #include "commu/commu_i2c_link.hpp"
     * 
     * int main() {
     *     stdio_init_all();
     * 
     *     // Communication link
     *     commu::I2CLink link(i2c0, 0x42);
     *     commu::AudioCommu comm(link);
     *     comm.init();
     * 
     *     while (true) {
     *         comm.process();
     * 
     *         if (comm.is_connected()) {
     *             if (comm.is_master()) {
     *                 // Send a simple packet
     *                 uint8_t data[] = {0x12, 0x34};
     *                 comm.send_packet(commu::CMD_STATUS, data, 2);
     *             } else {
     *                 // Slave: receive and process
     *                 uint8_t cmd;
     *                 uint8_t buf[32];
     *                 size_t len;
     *                 if (comm.recv_packet(cmd, buf, len, 10)) {
     *                     if (cmd == commu::CMD_STATUS) {
     *                         printf("Status: %02X %02X\n", buf[0], buf[1]);
     *                     }
     *                 }
     *             }
     *         }
     *         sleep_ms(10);
     *     }
     * }
     * @endcode
     */
    class AudioCommu {
        public:
            /**
             * @brief Constructor
             * @param link Reference to I2CLink instance
             * @param chk  Checksum implementation (optional, defaults to CRC16)
             */
            AudioCommu(I2CLink &link, Checksum *chk = nullptr)
                : link_(link), builder_(chk),
                state_(State::IDLE), role_(Role::UNKNOWN),
                connected_(false), handshake_step_(0) {}

            /**
             * @brief Initialize the communication module
             * 
             * Sets up GPIO pins and resets internal state.
             * Must be called once before process().
             */
            void init() {
                link_.init_pins();
                state_ = State::IDLE;
                connected_ = false;
                role_ = Role::UNKNOWN;
                handshake_step_ = 0;
            }

            /**
             * @brief Main state machine handler
             * 
             * Must be called frequently (e.g., in main loop) to drive the
             * negotiation and communication process.
             */
            void process() {
                // Check for disconnection (TN high means cable removed)
                if (state_ != State::IDLE && state_ != State::DISCONNECTING) {
                    if (!link_.is_cable_inserted()) {
                        enter_state(State::DISCONNECTING);
                        return;
                    }
                }

                switch (state_) {
                    case State::IDLE:            handle_idle(); break;
                    case State::DETECTING:       handle_detecting(); break;
                    case State::MASTER_CLAIM:    handle_master_claim(); break;
                    case State::SLAVE_CLAIM:     handle_slave_claim(); break;
                    case State::HANDSHAKE:       handle_handshake(); break;
                    case State::ESTABLISHED:     handle_established(); break;
                    case State::DISCONNECTING:   handle_disconnecting(); break;
                    default: break;
                }
            }

            /** @brief Check if the I2C link is established */
            bool is_connected() const { return connected_; }

            /** @brief Check if this device is the master */
            bool is_master() const { return role_ == Role::MASTER; }

            /**
             * @brief Send a packet (master only)
             * 
             * This function checks cable connection before sending.
             * If cable is removed during send, the connection is torn down.
             * 
             * @param cmd      Command code
             * @param data     Payload data (can be nullptr)
             * @param data_len Payload length
             * @return         true on success
             */
            bool send_packet(uint8_t cmd, const uint8_t *data, size_t data_len) {
                // Check connection before sending
                if (!connected_ || role_ != Role::MASTER) return false;

                // Check cable is still inserted
                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                    return false;
                }

                uint8_t buf[MAX_PACKET];
                size_t len;
                if (!builder_.build(buf, len, cmd, data, data_len)) return false;

                bool result = link_.master_write(buf, len);

                // Double-check cable after operation
                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                    return false;
                }

                return result;
            }

            /**
             * @brief Receive a packet (blocking)
             * 
             * This function checks cable connection before receiving.
             * If cable is removed during receive, the connection is torn down.
             * 
             * @param cmd         Output: command code
             * @param buf         Output buffer for payload
             * @param data_len    Output: payload length
             * @param timeout_ms  Timeout in milliseconds
             * @return            true on success
             */
            bool recv_packet(uint8_t &cmd, uint8_t *buf,
                            size_t &data_len, uint32_t timeout_ms = 100) {
                if (!connected_) return false;

                // Check cable is still inserted
                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                    return false;
                }

                bool result = false;

                if (role_ == Role::MASTER) {
                    // Master: read directly from slave via I2C
                    uint8_t raw[MAX_PACKET];
                    if (link_.master_read(raw, sizeof(raw), timeout_ms)) {
                        size_t packet_len = (raw[0] << 8) | raw[1] + 7;
                        if (packet_len <= MAX_PACKET) {
                            const uint8_t *data_ptr;
                            if (builder_.parse(raw, packet_len, cmd, data_ptr, data_len)) {
                                if (data_len > 0) memcpy(buf, data_ptr, data_len);
                                result = true;
                            }
                        }
                    }
                } else {
                    // Slave: read from RX queue (filled by I2C callback)
                    uint8_t raw[MAX_PACKET];
                    size_t len;
                    if (rx_queue_.dequeue(raw, len)) {
                        const uint8_t *data_ptr;
                        if (builder_.parse(raw, len, cmd, data_ptr, data_len)) {
                            if (data_len > 0) memcpy(buf, data_ptr, data_len);
                            result = true;
                        }
                    }
                }

                // Double-check cable after operation
                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                    return false;
                }

                return result;
            }

            /**
             * @brief Send a key event (simple command)
             * @param key_code Key code to send (one byte)
             * @return         true on success
             */
            bool send_key_event(uint8_t key_code) {
                return send_packet(CMD_KEY_EVENT, &key_code, 1);
            }

            /**
             * @brief Get reference to the RX queue (for slave callback)
             * @return Reference to RX PacketQueue
             */
            PacketQueue<8>& rx_queue() { return rx_queue_; }

            /**
             * @brief Get reference to the TX queue (for slave callback)
             * @return Reference to TX PacketQueue
             */
            PacketQueue<8>& tx_queue() { return tx_queue_; }

        private:
            /** @brief Internal state enumeration */
            enum class State : uint8_t {
                IDLE,           ///< Waiting for cable insertion
                DETECTING,      ///< Cable inserted, determining role
                MASTER_CLAIM,   ///< Claiming master role (holding SCL low)
                SLAVE_CLAIM,    ///< Claiming slave role (holding SDA low)
                HANDSHAKE,      ///< Performing handshake
                ESTABLISHED,    ///< I2C link active
                DISCONNECTING   ///< Tearing down
            };

            /** @brief Role enumeration */
            enum class Role : uint8_t {
                UNKNOWN,
                MASTER,
                SLAVE
            };

            I2CLink &link_;
            PacketBuilder builder_;
            PacketQueue<8> rx_queue_;
            PacketQueue<8> tx_queue_;

            State state_;
            Role role_;
            bool connected_;
            uint32_t handshake_step_;
            uint32_t last_event_;

            // Friend callback function
            friend void i2c_audio_slave_callback(i2c_inst_t *i2c, i2c_slave_event_t event);

            /** @brief Transition to a new state */
            void enter_state(State new_state) {
                state_ = new_state;
                last_event_ = time_us_32();
            }

            /** @brief Handle IDLE state: wait for cable */
            void handle_idle() {
                if (link_.is_cable_inserted()) {
                    enter_state(State::DETECTING);
                }
            }

            /** @brief Handle DETECTING state: determine role by SCL level */
            void handle_detecting() {
                sleep_ms(20);  // Debounce

                if (link_.read_pin(link_.scl_pin())) {
                    // SCL high → we are first → become master
                    role_ = Role::MASTER;
                    link_.set_pin_low(link_.scl_pin());
                    enter_state(State::MASTER_CLAIM);
                } else {
                    // SCL low → other is master → become slave
                    role_ = Role::SLAVE;
                    link_.set_pin_low(link_.sda_pin());
                    enter_state(State::SLAVE_CLAIM);
                }
            }

            /** @brief Handle MASTER_CLAIM: wait for slave to respond */
            void handle_master_claim() {
                if (link_.read_pin(link_.sda_pin()) == 0) {
                    // Slave responded (pulled SDA low)
                    link_.set_pin_high(link_.scl_pin());
                    handshake_step_ = 0;
                    enter_state(State::HANDSHAKE);
                } else if (time_us_32() - last_event_ > 500000) {
                    // Timeout: no slave present
                    enter_state(State::IDLE);
                }

                // Check cable during negotiation
                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                }
            }

            /** @brief Handle SLAVE_CLAIM: wait for master to release SCL */
            void handle_slave_claim() {
                if (link_.read_pin(link_.scl_pin()) == 1) {
                    // Master released SCL
                    link_.set_pin_high(link_.sda_pin());
                    handshake_step_ = 0;
                    enter_state(State::HANDSHAKE);
                } else if (time_us_32() - last_event_ > 500000) {
                    enter_state(State::IDLE);
                }

                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                }
            }

            /** @brief Handle HANDSHAKE: 4-step pulse exchange */
            void handle_handshake() {
                if (role_ == Role::MASTER) {
                    handle_handshake_master();
                } else {
                    handle_handshake_slave();
                }

                if (time_us_32() - last_event_ > 5000000) {
                    enter_state(State::IDLE);  // 5s handshake timeout
                }

                // Check cable during handshake
                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                }
            }

            /** @brief Handshake from master side */
            void handle_handshake_master() {
                switch (handshake_step_) {
                    case 0:
                        link_.set_pin_low(link_.scl_pin());
                        handshake_step_ = 1;
                        last_event_ = time_us_32();
                        break;
                    case 1:
                        if (time_us_32() - last_event_ >= 1000) {
                            link_.set_pin_high(link_.scl_pin());
                            handshake_step_ = 2;
                            last_event_ = time_us_32();
                        }
                        break;
                    case 2:
                        if (link_.read_pin(link_.sda_pin()) == 0) {
                            last_event_ = time_us_32();
                            handshake_step_ = 3;
                        }
                        break;
                    case 3:
                        if (link_.read_pin(link_.sda_pin()) == 1) {
                            link_.set_pin_low(link_.scl_pin());
                            handshake_step_ = 4;
                            last_event_ = time_us_32();
                        }
                        break;
                    case 4:
                        if (time_us_32() - last_event_ >= 1000) {
                            link_.set_pin_high(link_.scl_pin());
                            enter_state(State::ESTABLISHED);
                        }
                        break;
                    default:
                        enter_state(State::IDLE);
                        break;
                }
            }

            /** @brief Handshake from slave side */
            void handle_handshake_slave() {
                switch (handshake_step_) {
                    case 0:
                        if (link_.read_pin(link_.scl_pin()) == 0) {
                            last_event_ = time_us_32();
                            handshake_step_ = 1;
                        }
                        break;
                    case 1:
                        if (time_us_32() - last_event_ >= 1000) {
                            link_.set_pin_low(link_.sda_pin());
                            handshake_step_ = 2;
                            last_event_ = time_us_32();
                        }
                        break;
                    case 2:
                        if (time_us_32() - last_event_ >= 1000) {
                            link_.set_pin_high(link_.sda_pin());
                            handshake_step_ = 3;
                            last_event_ = time_us_32();
                        }
                        break;
                    case 3:
                        if (link_.read_pin(link_.scl_pin()) == 0) {
                            last_event_ = time_us_32();
                            handshake_step_ = 4;
                        }
                        break;
                    case 4:
                        if (time_us_32() - last_event_ >= 1000) {
                            enter_state(State::ESTABLISHED);
                        }
                        break;
                    default:
                        enter_state(State::IDLE);
                        break;
                }
            }

            /** @brief Handle ESTABLISHED: I2C link active */
            void handle_established() {
                if (!connected_) {
                    // First entry to established state: configure I2C
                    if (role_ == Role::MASTER) {
                        link_.enable_master();
                    } else {
                        // Slave mode: register callback
                        link_.enable_slave(i2c_audio_slave_callback);
                    }
                    connected_ = true;
                }

                // Check cable in established state
                if (!link_.is_cable_inserted()) {
                    enter_state(State::DISCONNECTING);
                }
            }

            /** @brief Handle DISCONNECTING: graceful teardown */
            void handle_disconnecting() {
                if (time_us_32() - last_event_ > 100000) {
                    // Release pins to input with pull-up
                    gpio_set_dir(link_.sda_pin(), GPIO_IN);
                    gpio_set_dir(link_.scl_pin(), GPIO_IN);
                    gpio_pull_up(link_.sda_pin());
                    gpio_pull_up(link_.scl_pin());

                    connected_ = false;
                    role_ = Role::UNKNOWN;
                    enter_state(State::IDLE);
                }
            }
    };

    /**
     * @brief Global I2C slave callback function for AudioCommu
     * 
     * This is called by the Pico SDK I2C slave driver on bus events.
     * It forwards events to the active AudioCommu instance.
     * 
     * @param i2c   I2C instance
     * @param event Event type (RECEIVE, REQUEST, FINISH)
     */
    static inline void i2c_audio_slave_callback(i2c_inst_t *i2c, i2c_slave_event_t event) {
        // Get the global AudioCommu instance
        // We need a global pointer accessible from this function.
        // In practice, you can use a global variable or singleton pattern.
        // For this example, we assume a global pointer is set elsewhere.
        //
        // For production code, consider using a singleton or passing context.
        //
        // @note The actual implementation of this callback needs access to the
        //       AudioCommu instance's queues. Since this is a static function,
        //       we use a global pointer `g_audio_commu` which must be defined
        //       in the application code.
        extern AudioCommu *g_audio_commu;

        if (!g_audio_commu) return;

        switch (event) {
            case I2C_SLAVE_RECEIVE: {
                // Master is writing to us (data packet)
                uint8_t buf[MAX_PACKET];
                size_t len = 0;
                while (i2c_get_read_available(i2c)) {
                    if (len < MAX_PACKET) {
                        buf[len++] = i2c_read_byte(i2c);
                    } else {
                        i2c_read_byte(i2c);  // discard extra
                    }
                }
                if (len > 0) {
                    g_audio_commu->rx_queue().enqueue(buf, len);
                }
                break;
            }
            case I2C_SLAVE_REQUEST: {
                // Master is reading from us: send next packet from tx queue
                uint8_t buf[MAX_PACKET];
                size_t len;
                if (g_audio_commu->tx_queue().dequeue(buf, len)) {
                    for (size_t i = 0; i < len; ++i) {
                        i2c_write_byte(i2c, buf[i]);
                    }
                }
                break;
            }
            case I2C_SLAVE_FINISH:
                // Transaction done
                break;
        }
    }

} // namespace commu

#endif // COMMU_AUDIO_HPP