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
    constexpr uint8_t B_VECTOR  = 0x11;
    constexpr uint8_t B_MATRIX  = 0x12;
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

            /** 
             * @brief find the right block matches the lblock
             * @param lpos the position of the left block's \x03
             */
            size_t _findRblock(size_t lpos) {
                uint16_t lCnt = 0;
                for(size_t i = lpos; i < exp.size() - 1; ++i) {
                    if(exp[i] == '\x03') {
                        // is a control (block)
                        if(exp[i + 1] == Ctrl::BLOCKL[1]) {
                            // left block
                            ++lCnt;
                        } else if(exp[i + 1] == Ctrl::BLOCKR[1]) {
                            // right block
                            --lCnt;
                        }
                    }
                    if(!lCnt) {
                        // Perfect!
                        return i; // return the position of the '\x03' mark
                    }
                }
                // cannot find
                return std::string::npos;
            }

            /** 
             * @brief find the left block matches the rblock
             * @param rpos the position of the right block's \x03
             */
            size_t _findLblock(size_t rpos) {
                uint16_t rCnt = 0;
                for(size_t i = rpos + 1; i > 0; --i) {
                    if(exp[i - 1] == '\x03') {
                        // is a control (block)
                        if(exp[i] == Ctrl::BLOCKL[1]) {
                            // left block
                            --rCnt;
                        } else if(exp[i] == Ctrl::BLOCKR[1]) {
                            // right block
                            ++rCnt;
                        }
                    }
                    if(!rCnt) {
                        // Perfect!
                        return i; // return the position of the '\x03' mark
                    }
                }
                // cannot find
                return std::string::npos;
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
                    return;
                }
                if((cp - 3 >= 0) && exp[cp - 3] == '\x01') {
                    /** 
                     * like ...\x01??...
                     * function: fact, dot, estx1, estx2, esty,
                     *           band, bor, bxor, bnot, blshift, brshift, bxnor, bnand, bnor
                     */
                    exp.erase(cp - 3, 3);
                    cp -= 3;
                    return;
                }
                if((cp - 2 >= 0) && exp[cp - 2] == '\x03') {
                    // The only control keys that can be inserted into the expression are
                    // BLOCKL and BLOCKR
                    if(exp[cp - 1] == Ctrl::BLOCKL[1]) {
                        // delete a left block...
                        
                        if(cp - 5 >= 0 && exp[cp - 5] == '\x01') {
                            // is a function \x01??\x03\x20{|\x03}\x21...
                            std::string func(exp.substr(cp - 5, 3));

                            // sqrt, abs (single block)
                            if(func == CAS::FuncName::sqrt || func == CAS::FuncName::abs) {
                                // single block

                                // find right block, _pos = \x03
                                size_t _pos = _findRblock(cp - 2);
                                if(_pos != std::string::npos) {
                                    exp.erase(_pos, 2); // erase the right block first
                                }

                                // then delete the left block and the function
                                exp.erase(cp - 5, 5);
                                return;
                            }

                            // double block
                            if (
                                func == CAS::FuncName::root ||
                                func == CAS::FuncName::log ||
                                func == CAS::FuncName::permut ||
                                func == CAS::FuncName::combin ||
                                func == CAS::FuncName::randint ||
                                func == CAS::FuncName::randrat
                            ) {
                                if(func == CAS::FuncName::log) {
                                    // log [|a] [b] === log_b(|a)
                                    //    ↓
                                    // log [a] [b|] === log_b|(a)
                                    // \x01lg\x03\x20|...\x03\x21\x03\x20...|\x03\x21
                                    //               |--------------------->^

                                    //                   ^~~~---------------|
                                    // find the second right block          |
                                    size_t _pos = _findRblock(cp - 2); // --|
                                    if (_pos == std::string::npos) {
                                        return;
                                    }
                                    _pos = _findRblock(_pos + 2);
                                    if (_pos != std::string::npos) {
                                        cp = _pos;
                                    }
                                    return;
                                } else {
                                    // root, permutation, combination
                                    // delete the function and the blocks but save params
                                    // \x01??\x03\x20|...\x03\x21\x03\x20...\x03\x21
                                    //     ↓       ↑       ↑                  ↑
                                    // ... ... (cp - 1)  pos[0]             pos[1]

                                    size_t _pos[2]{0};
                                    _pos[0] = _findRblock(cp - 2);
                                    if(_pos[0] == std::string::npos) {
                                        // no matching rblock, exit
                                        return;
                                    }
                                    // the second input block
                                    _pos[1] = _findRblock(_pos[0] + 2);
                                    if(_pos[1] != std::string::npos) {
                                        // erase
                                        // 2nd rblock
                                        exp.erase(_pos[1], 2);
                                        // 1st rblock & 2nd lblock
                                        exp.erase(_pos[0], 4);
                                        // function & 1st lblock
                                        exp.erase(cp - 5, 5);
                                    }
                                }
                            }

                            // multi block
                            if (
                                func == CAS::FuncName::sum ||
                                func == CAS::FuncName::prod ||
                                func == CAS::FuncName::defint ||
                                func == CAS::FuncName::diff ||
                                func == CAS::FuncName::indefint
                            ) {
                                // \x01??\x03\x20|...\x03\x21\x03\x20...\x03\x21\x03\x20...\x03\x21\x03\x20...\x03\x21
                                //               cp   0                   1                  2                  3 
                                size_t _pos[2]{0};
                                _pos[0] = _findRblock(cp - 2);
                                if(_pos[0] == std::string::npos) {
                                    return;
                                }
                                _pos[1] = _findRblock(_pos[0] + 2); ///< position "1"
                                if(_pos[1] == std::string::npos) {
                                    return;
                                }
                                if(func != CAS::FuncName::diff && func != CAS::FuncName::indefint) {
                                    _pos[1] = _findRblock(_pos[1] + 2); ///< position "2"
                                    if(_pos[1] == std::string::npos) {
                                        return;
                                    }
                                    _pos[1] = _findRblock(_pos[1] + 2); ///< position "3"
                                    if(_pos[1] == std::string::npos) {
                                        return;
                                    }
                                }
                                exp.erase(_pos[0], _pos[1] - _pos[0] + 2);
                                exp.erase(cp - 5, 5);
                                return;
                            }
                        } else {
                            // a single block
                            size_t _pos = _findRblock(cp - 2);
                            if(_pos != std::string::npos) {
                                exp.erase(_pos, 2);
                                exp.erase(cp - 2, 2);
                            }
                            return;
                        }
                    }
                    if(exp[cp - 1] == Ctrl::BLOCKR[1]) {
                        _Move(Ctrl::X_MINUS); // Move left
                        return;
                    }
                }
            }
            /** @brief Move the cursor */
            void _Move(std::string k) {
                ;
            }
    };
} // namespace Keypad

#endif // _EXPBUILD_HPP_