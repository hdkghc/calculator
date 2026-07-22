/** @file inc/commu/commu_crc.hpp
 *  @brief CRC16 implementation for packet integrity
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

#ifndef COMMU_CRC_HPP
#define COMMU_CRC_HPP

#include "commu_types.hpp"

namespace commu {

    /**
     * @brief CRC-16/IBM (poly 0x8005, initial 0xFFFF)
     * 
     * Used for packet integrity verification. Returns 16-bit result
     * stored in lower 16 bits of uint32_t.
     */
    class CRC16 : public Checksum {
        public:
            /**
             * @brief Compute CRC16 over data
             * @param data Pointer to data buffer
             * @param len  Number of bytes
             * @return     CRC16 value (in lower 16 bits)
             */
            uint32_t compute(const uint8_t *data, size_t len) override {
                uint16_t crc = 0xFFFF;
                for (size_t i = 0; i < len; ++i) {
                    crc ^= data[i];
                    for (int j = 0; j < 8; ++j) {
                        if (crc & 1) {
                            crc = (crc >> 1) ^ 0xA001;
                        } else {
                            crc >>= 1;
                        }
                    }
                }
                return crc;
            }
    };

} // namespace commu

#endif // COMMU_CRC_HPP