/** @file inc/commu/commu_types.hpp
 *  @brief Common types, command codes and checksum interface
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

#ifndef COMMU_TYPES_HPP
#define COMMU_TYPES_HPP

#include <cstdint>
#include <cstddef>

namespace commu {

    /** @brief Maximum payload size per packet */
    static constexpr uint16_t MAX_PAYLOAD = 480;

    /** @brief Maximum packet size: len(2) + cmd(1) + payload + checksum(4) */
    static constexpr uint16_t MAX_PACKET = MAX_PAYLOAD + 7;

    /**
     * @brief Enhanced command codes for communication protocol
     */
    enum Command : uint8_t {
        // ---- Basic control (0x01–0x0F) ----
        CMD_HANDSHAKE   = 0x01,  ///< Handshake initiation
        CMD_ACK         = 0x02,  ///< Positive acknowledgment
        CMD_NACK        = 0x03,  ///< Negative acknowledgment
        CMD_PING        = 0x04,  ///< Heartbeat / keep-alive
        CMD_STATUS      = 0x05,  ///< Query device status
        CMD_RESET       = 0x06,  ///< Reset remote device

        // ---- File transfer (0x10–0x1F) ----
        CMD_FILE_DATA   = 0x10,  ///< File data chunk
        CMD_FILE_END    = 0x11,  ///< End of file transmission
        CMD_FILE_REQ    = 0x12,  ///< Request a file by name
        CMD_FILE_RESP   = 0x13,  ///< File request response (accept/reject)
        CMD_FILE_INFO   = 0x14,  ///< File metadata (size, name)
        CMD_FILE_START  = 0x15,  ///< Start file transfer with metadata

        // ---- Keypad events (0x20–0x2F) ----
        CMD_KEY_EVENT   = 0x20,  ///< Single key press/release event
        CMD_KEY_SCAN    = 0x21,  ///< Scan request / response
        CMD_KEY_MATRIX  = 0x22,  ///< Full matrix state dump

        // ---- GPIO control (0x30–0x3F) ----
        CMD_GPIO_SET    = 0x30,  ///< Set remote GPIO output
        CMD_GPIO_GET    = 0x31,  ///< Read remote GPIO input
        CMD_GPIO_CFG    = 0x32,  ///< Configure remote GPIO direction

        // ---- System commands (0x40–0x4F) ----
        CMD_GET_TIME    = 0x40,  ///< Get remote timestamp
        CMD_SET_TIME    = 0x41,  ///< Set remote timestamp
        CMD_GET_INFO    = 0x42,  ///< Get device info (name, version, UID)
        CMD_REBOOT      = 0x43,  ///< Reboot remote device

        // ---- Special ----
        CMD_DISCONNECT  = 0xFF   ///< Graceful disconnect
    };

    struct __attribute__((packed)) FileMeta {
        uint32_t total_size;   // 总文件大小（字节）
        uint8_t  filename[32]; // 文件名（8.3格式，最多31字符）
        uint8_t  reserved[3];  // 对齐
    };

    /**
     * @brief Abstract checksum interface
     * 
     * Allows pluggable checksum algorithms (CRC16, CRC32, etc.)
     */
    class Checksum {
        public:
            virtual ~Checksum() = default;

            /**
             * @brief Compute checksum over given data
             * @param data Pointer to data buffer
             * @param len  Number of bytes
             * @return     32-bit checksum value (lower bits used for 16-bit algorithms)
             */
            virtual uint32_t compute(const uint8_t *data, size_t len) = 0;
    };

} // namespace commu

#endif // COMMU_TYPES_HPP