/** @file inc/commu/commu_packet.hpp
 *  @brief Packet builder and parser for communication protocol
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

#ifndef COMMU_PACKET_HPP
#define COMMU_PACKET_HPP

#include "commu_types.hpp"
#include "commu_crc.hpp"
#include <cstring>

namespace commu {

    /**
     * @brief Packet builder with pluggable checksum
     * 
     * Packet format: [length(2)] [cmd(1)] [data(N)] [checksum(4)]
     * All multi-byte fields are big-endian.
     * 
     * @code
     * PacketBuilder builder;
     * uint8_t buf[MAX_PACKET];
     * size_t len;
     * uint8_t data[] = {0x01, 0x02, 0x03};
     * 
     * if (builder.build(buf, len, CMD_FILE_DATA, data, 3)) {
     *     // send buf, length len
     * }
     * @endcode
     */
    class PacketBuilder {
        public:
            /**
             * @brief Constructor
             * @param chk Checksum implementation (default CRC16)
             */
            PacketBuilder(Checksum *chk = nullptr)
                : checksum_(chk ? chk : &default_crc_) {}

            /**
             * @brief Build a packet into buffer
             * @param buf      Output buffer (must be at least MAX_PACKET)
             * @param out_len  Output: total packet length
             * @param cmd      Command code
             * @param data     Payload data (can be nullptr if data_len == 0)
             * @param data_len Payload length (must be <= MAX_PAYLOAD)
             * @return         true on success, false if data_len exceeds limit
             */
            bool build(uint8_t *buf, size_t &out_len,
                    uint8_t cmd, const uint8_t *data, size_t data_len) {
                if (data_len > MAX_PAYLOAD) return false;

                // Length field (big-endian)
                buf[0] = (data_len >> 8) & 0xFF;
                buf[1] = data_len & 0xFF;

                // Command
                buf[2] = cmd;

                // Payload
                if (data_len > 0) {
                    memcpy(buf + 3, data, data_len);
                }

                // Checksum (computed over payload only)
                uint32_t chk = checksum_->compute(data, data_len);
                buf[3 + data_len]     = (chk >> 24) & 0xFF;
                buf[3 + data_len + 1] = (chk >> 16) & 0xFF;
                buf[3 + data_len + 2] = (chk >> 8) & 0xFF;
                buf[3 + data_len + 3] = chk & 0xFF;

                out_len = 3 + data_len + 4;
                return true;
            }

            /**
             * @brief Parse a received packet
             * @param buf      Input buffer containing raw packet
             * @param len      Length of buffer
             * @param cmd      Output: command code
             * @param data     Output: pointer to payload inside buffer
             * @param data_len Output: payload length
             * @return         true if packet is valid and checksum matches
             */
            bool parse(const uint8_t *buf, size_t len,
                    uint8_t &cmd, const uint8_t *&data, size_t &data_len) {
                if (len < 7) return false;  // Minimum: 2+1+0+4

                uint16_t claimed_len = (buf[0] << 8) | buf[1];
                if (claimed_len > MAX_PAYLOAD) return false;
                if (len != 3 + claimed_len + 4) return false;

                cmd = buf[2];
                data = buf + 3;
                data_len = claimed_len;

                uint32_t recv_chk = (buf[3 + claimed_len] << 24) |
                                    (buf[3 + claimed_len + 1] << 16) |
                                    (buf[3 + claimed_len + 2] << 8) |
                                    buf[3 + claimed_len + 3];

                uint32_t calc_chk = checksum_->compute(data, data_len);
                return recv_chk == calc_chk;
            }

        private:
            Checksum *checksum_;
            CRC16 default_crc_;
    };

} // namespace commu

#endif // COMMU_PACKET_HPP