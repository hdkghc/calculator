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

    constexpr uint8_t M_SHIFT  = 0x01;
    constexpr uint8_t M_ALPHA  = 0x02;
    constexpr uint8_t M_CTRL   = 0x04;
    constexpr uint8_t M_LOCK   = 0x08;
    constexpr uint8_t M_INSERT = 0x10;
    constexpr uint8_t M_RCL    = 0x20;
    constexpr uint8_t M_STO    = 0x40;


    constexpr uint8_t B_SUCCESS = 0x00;
    constexpr uint8_t B_PLOT2D  = 0x01;
    constexpr uint8_t B_PLOT3D  = 0x02;
    constexpr uint8_t B_SOLVE   = 0x03;
    constexpr uint8_t B_CONV    = 0x04;
    constexpr uint8_t B_CONST   = 0x05;
    constexpr uint8_t B_EXEC    = 0x06;
    constexpr uint8_t B_OPTN    = 0x07;
    constexpr uint8_t B_MENU    = 0x08;
    constexpr uint8_t B_MODE    = 0x09;
    constexpr uint8_t B_FMT     = 0x0A;
    constexpr uint8_t B_ABOUT   = 0x0B;
    constexpr uint8_t B_SET     = 0x0C;
    constexpr uint8_t B_CLEAR   = 0x0D;
    constexpr uint8_t B_CALC    = 0x0E;
    constexpr uint8_t B_FACTOR  = 0x0F;
    constexpr uint8_t B_EXPAND  = 0x10;
    constexpr uint8_t B_OFF     = 0xFE;

    constexpr uint8_t B_ERROR   = 0xFF;
    
    class Expbuild {
        protected:
            /** @brief The internal expression string */
            std::string exp;
            /** @brief The status flag */
            uint8_t flg;
            /** @brief The cursor position */
            int16_t cp;
        public:
            Expbuild() : flg(0), cp(0) {}
        protected:
            /** @brief Handle a modifier key press */
            void _Modifier(std::string k) {
                if(k == Ctrl::SHIFT) {
                    flg ^= M_SHIFT;
                    return;
                }
                if(k == Ctrl::ALPHA) {
                    flg ^= M_ALPHA;
                    return;
                }
                if(k == Ctrl::CTRL) {
                    flg ^= M_CTRL;
                    flg &= 0xFF & (~M_LOCK);
                    return;
                }
                if(k == Ctrl::LOCK) {
                    flg ^= M_LOCK;
                    return;
                }
                if(k == Ctrl::INS) {
                    flg ^= M_INSERT;
                    return;
                }
                if(k == Ctrl::RCL) {
                    flg ^= M_RCL;
                    return;
                }
                if(k == Ctrl::STO) {
                    flg ^= M_STO;
                    return;
                }
            }
            /** @brief Check invalid status */
            void _Check() {
                if((flg & M_LOCK) && !(flg & M_CTRL)) {
                    flg &= 0xFF & (~M_LOCK);
                }
                if((flg & M_RCL) && (flg & M_STO)) {
                    flg &= 0xFF & (~M_RCL);
                    flg &= 0xFF & (~M_STO);
                }
            }
            /** @brief Process AC key */
            void _AC() {
                exp.clear();
                cp = 0;
                flg = 0;
            }
            /** @brief Process DEL key */
            void _DEL() {
                if(cp < 0 || cp >= exp.size()) return;
                if(cp == 0 && exp.size() > 0) { // Head, delete the first character
                    _Move(Ctrl::X_PLUS); // Move right
                    _DEL(); // and delete it
                    return;
                }
                if((cp - 4 >= 0) && exp[cp - 4] == '\x01') {
                    /**
                     * function '\x01??(', listed below:
                     * sin cos tan asin acos atan ...
                     * ln log10 exp round sign max min gcd lcm
                     * polar rect deg rad randint
                     * realpart imagpart conjg arg
                     * transpose eigenval eigenvec adjoint rank det
                    */
                    exp.erase(cp - 4, 4);
                    cp -= 4;
                }
            }
            /** @brief Move the cursor */
            void _Move(std::string k) {
                ;
            }
    };
} // namespace Keypad

#endif // _EXPBUILD_HPP_