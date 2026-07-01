/** @file /inc/expbuild.hpp
 *  @brief Builds internal expression strings from key presses
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

#ifndef _EXPBUILD_HPP_
#define _EXPBUILD_HPP_

#include "keypad.hpp"
#include <string>
#include <cstddef>
#include <cstdint>

namespace Keypad {

    constexpr uint8_t MASK_SHIFT  = 0x01;
    constexpr uint8_t MASK_ALPHA  = 0x02;
    constexpr uint8_t MASK_CTRL   = 0x04;
    constexpr uint8_t MASK_LOCK   = 0x08;
    constexpr uint8_t MASK_INSERT = 0x10;


    constexpr uint8_t BUILD_SUCCESS = 0x00;
    constexpr uint8_t BUILD_PLOT2D  = 0x01;
    constexpr uint8_t BUILD_PLOT3D  = 0x02;
    constexpr uint8_t BUILD_SOLVE   = 0x03;
    constexpr uint8_t BUILD_CONV    = 0x04;
    constexpr uint8_t BUILD_CONST   = 0x05;
    constexpr uint8_t BUILD_EXEC    = 0x06;
    constexpr uint8_t BUILD_OPTN    = 0x07;
    constexpr uint8_t BUILD_MENU    = 0x08;
    constexpr uint8_t BUILD_MODE    = 0x09;
    constexpr uint8_t BUILD_FMT     = 0x0A;
    constexpr uint8_t BUILD_ABOUT   = 0x0B;
    constexpr uint8_t BUILD_SET     = 0x0C;
    constexpr uint8_t BUILD_CLEAR   = 0x0D;
    constexpr uint8_t BUILD_CALC    = 0x0E;
    constexpr uint8_t BUILD_FACTOR  = 0x0F;
    constexpr uint8_t BUILD_EXPAND  = 0x10;
    constexpr uint8_t BUILD_OFF     = 0xFE;

    constexpr uint8_t BUILD_ERROR   = 0xFF;
    
    
    
} // namespace Keypad

#endif // _EXPBUILD_HPP_