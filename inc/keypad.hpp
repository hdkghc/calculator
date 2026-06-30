/** @file /inc/keypad.hpp
 *  @brief Keypad definition of the calculator project
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
#ifndef _KEYPAD_HPP_
#define _KEYPAD_HPP_

#include <string>
#include <cstdint>
#include "cas/expdef.hpp"

namespace Keypad {

    // Modifier bitmask
    constexpr uint8_t MOD_NONE  = 0;
    constexpr uint8_t MOD_SHIFT = 1;
    constexpr uint8_t MOD_ALPHA = 2;
    constexpr uint8_t MOD_CTRL  = 4;

    // Control key identifiers: \003\000 ~ \003\037
    namespace Ctrl {
        const char *const SHIFT   = "\x03\x00";
        const char *const ALPHA   = "\x03\x01";
        const char *const CTRL    = "\x03\x02";
        const char *const ON      = "\x03\x03";
        const char *const OFF     = "\x03\x04";
        const char *const AC      = "\x03\x05";
        const char *const DEL     = "\x03\x06";
        const char *const INS     = "\x03\x07";
        const char *const EXE     = "\x03\x08";
        const char *const LOCK    = "\x03\x09";
        const char *const ABOUT   = "\x03\x0A";
        const char *const MENU    = "\x03\x0B";
        const char *const SET     = "\x03\x0C";
        const char *const CLR     = "\x03\x0D";
        const char *const MODE    = "\x03\x0E";
        const char *const OPTN    = "\x03\x0F";
        const char *const FMT     = "\x03\x10";
        const char *const SOLV    = "\x03\x11";
        const char *const CALC    = "\x03\x12";
        const char *const CONST   = "\x03\x13";
        const char *const CONV    = "\x03\x14";
        const char *const OK      = "\x03\x15";
        // Graph controls
        const char *const ZOOM_P  = "\x03\x16";  // Zoom+
        const char *const ZOOM_M  = "\x03\x17";  // Zoom-
        const char *const FACTOR  = "\x03\x18";
        const char *const EXPAND  = "\x03\x19";
        const char *const Z_PLUS  = "\x03\x1A";  // Z+
        const char *const Z_MINUS = "\x03\x1B";  // Z-
        const char *const Y_PLUS  = "\x03\x1C";  // Y+ (up)
        const char *const Y_MINUS = "\x03\x1D";  // Y- (down)
        const char *const X_PLUS  = "\x03\x1E";  // X+ (right)
        const char *const X_MINUS = "\x03\x1F";  // X- (left)
        // 
        const char *const BLOCKL  = "\x03\x20";  // Input block left mark
        const char *const BLOCKR  = "\x03\x21";  // Input block right mark
        const char *const STO     = "\x03\x22";
        const char *const RCL     = "\x03\x23";
    }

    // Variable identifiers
    namespace VarName {
        const char *const Ans    = "\x04\x01";  // Ans
        const char *const PAns   = "\x04\x02";  // PreAns
        const char *const theta  = "\x04\x03";  // theta
        const char *const lambda = "\x04\x04";  // lambda
        const char *const mu     = "\x04\x05";  // mu
        const char *const alpha  = "\x04\x06";  // alpha
    }

    // Special symbols
    namespace Spec {
        const char *const EE     = "\x05\x01";  // EE (scientific notation)
        const char *const SQ     = "\x05\x02";  // square ^2
        const char *const CB     = "\x05\x03";  // cube ^3
        const char *const INV    = "\x05\x04";  // inverse ^-1
        const char *const CBRT   = "\x05\x05";  // cube root
    }

    // 6x6 keypad, 6 layers
    //
    // Layer index:
    //   0: MOD_NONE               (Base)
    //   1: MOD_SHIFT              (SHIFT)
    //   2: MOD_ALPHA              (ALPHA)
    //   3: MOD_SHIFT | MOD_ALPHA  (SHIFT + ALPHA)
    //   4: MOD_CTRL               (CTRL)
    //   5: MOD_SHIFT | MOD_CTRL   (SHIFT + CTRL)
    //
    // ALPHA|CTRL (6) and SHIFT|ALPHA|CTRL (7) are ALPHA-lock variants,
    // mapped to layer 2 and 3 by getKey().

    // Layer remap table: mod bits 0-7 -> array index 0-5
    constexpr uint8_t layer_index[8] = {0, 1, 2, 3, 4, 5, 2, 3};

    inline const std::string keypad[6][6][6] = {
        // ==================== Row A ====================
        {
            // A1: SHIFT
            {
                Ctrl::SHIFT,        // 0: Base
                Ctrl::SHIFT,        // 1: SHIFT
                Ctrl::SHIFT,        // 2: ALPHA
                Ctrl::SHIFT,        // 3: SHIFT|ALPHA
                Ctrl::SHIFT,        // 4: CTRL
                Ctrl::SHIFT,        // 5: SHIFT|CTRL
            },
            // A2: ALPHA
            {
                Ctrl::ALPHA,        // 0
                Ctrl::ALPHA,        // 1: SHIFT
                Ctrl::ALPHA,        // 2: ALPHA
                Ctrl::ALPHA,        // 3: SHIFT|ALPHA
                Ctrl::ALPHA,        // 4: CTRL
                Ctrl::ALPHA,        // 5: SHIFT|CTRL
            },
            // A3: CTRL
            {
                Ctrl::CTRL,         // 0
                Ctrl::CTRL,         // 1: SHIFT
                Ctrl::CTRL,         // 2: ALPHA
                Ctrl::CTRL,         // 3: SHIFT|ALPHA
                Ctrl::CTRL,         // 4: CTRL
                Ctrl::CTRL,         // 5: SHIFT|CTRL
            },
            // A4: x / SOLV
            {
                "x",                // 0
                Ctrl::SOLV,         // 1: SHIFT
                "",                 // 2: ALPHA
                "",                 // 3: SHIFT|ALPHA
                Ctrl::STO,          // 4: CTRL
                Ctrl::RCL,          // 5: SHIFT|CTRL
            },
            // A5: OPTN / MODE / SET
            {
                Ctrl::OPTN,         // 0
                Ctrl::MODE,         // 1: SHIFT
                "",                 // 2: ALPHA
                "",                 // 3: SHIFT|ALPHA
                Ctrl::SET,          // 4: CTRL
                "",                 // 5: SHIFT|CTRL
            },
            // A6: ON / ABOUT / CLR / ON
            {
                Ctrl::ON,           // 0
                Ctrl::ABOUT,        // 1: SHIFT
                "",                 // 2: ALPHA
                "",                 // 3: SHIFT|ALPHA
                Ctrl::CLR,          // 4: CTRL
                Ctrl::ON,           // 5: SHIFT|CTRL
            }
        },

        // ==================== Row B ====================
        {
            // B1: sqrt / cbrt / frac
            {
                CAS::FuncName::sqrt,    // 0
                Spec::CBRT,             // 1: SHIFT
                "a",                    // 2: ALPHA
                "A",                    // 3: SHIFT|ALPHA
                CAS::FuncName::frac,    // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // B2: ^ / root
            {
                "^",                    // 0
                CAS::FuncName::root,    // 1: SHIFT
                "b",                    // 2: ALPHA
                "B",                    // 3: SHIFT|ALPHA
                "",                     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // B3: SQ / CB / INV
            {
                Spec::SQ,               // 0
                Spec::CB,               // 1: SHIFT
                "c",                    // 2: ALPHA
                "C",                    // 3: SHIFT|ALPHA
                Spec::INV,              // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // B4: ( / dot
            {
                "(",                    // 0
                CAS::FuncName::dot,     // 1: SHIFT
                "d",                    // 2: ALPHA
                "D",                    // 3: SHIFT|ALPHA
                "",                     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // B5: ) / , / ;
            {
                ")",                    // 0
                ",",                    // 1: SHIFT
                "e",                    // 2: ALPHA
                "E",                    // 3: SHIFT|ALPHA
                ";",                    // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // B6: lg / ln / logab
            {
                CAS::FuncName::log10,   // 0
                CAS::FuncName::ln,      // 1: SHIFT
                "f",                    // 2: ALPHA
                "F",                    // 3: SHIFT|ALPHA
                CAS::FuncName::log,     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            }
        },

        // ==================== Row C ====================
        {
            // C1: 7 / vector
            {
                "7",                    // 0
                CAS::FuncName::vector,  // 1: SHIFT
                "g",                    // 2: ALPHA
                "G",                    // 3: SHIFT|ALPHA
                "",                     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // C2: 8 / mtrx / Y+
            {
                "8",                    // 0
                CAS::FuncName::matrix,  // 1: SHIFT
                "h",                    // 2: ALPHA
                "H",                    // 3: SHIFT|ALPHA
                Ctrl::Y_PLUS,           // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // C3: 9 / CALC
            {
                "9",                    // 0
                Ctrl::CALC,             // 1: SHIFT
                "i",                    // 2: ALPHA
                "I",                    // 3: SHIFT|ALPHA
                "",                     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // C4: tan / atan / tanh / atanh
            {
                CAS::FuncName::tan,     // 0
                CAS::FuncName::atan,    // 1: SHIFT
                "j",                    // 2: ALPHA
                "J",                    // 3: SHIFT|ALPHA
                CAS::FuncName::tanh,    // 4: CTRL
                CAS::FuncName::atanh,   // 5: SHIFT|CTRL
            },
            // C5: DEL / INS
            {
                Ctrl::DEL,              // 0
                Ctrl::INS,              // 1: SHIFT
                "k",                    // 2: ALPHA
                "K",                    // 3: SHIFT|ALPHA
                "",                     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // C6: AC / OFF / LOCK
            {
                Ctrl::AC,               // 0
                Ctrl::OFF,              // 1: SHIFT
                "l",                    // 2: ALPHA
                "L",                    // 3: SHIFT|ALPHA
                Ctrl::LOCK,             // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            }
        },

        // ==================== Row D ====================
        {
            // D1: 4 / CONST / X-
            {
                "4",                    // 0
                Ctrl::CONST,            // 1: SHIFT
                "m",                    // 2: ALPHA
                "M",                    // 3: SHIFT|ALPHA
                Ctrl::X_MINUS,          // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // D2: 5 / CONV / OK
            {
                "5",                    // 0
                Ctrl::CONV,             // 1: SHIFT
                "n",                    // 2: ALPHA
                "N",                    // 3: SHIFT|ALPHA
                Ctrl::OK,               // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // D3: 6 / MENU / X+
            {
                "6",                    // 0
                Ctrl::MENU,             // 1: SHIFT
                "o",                    // 2: ALPHA
                "O",                    // 3: SHIFT|ALPHA
                Ctrl::X_PLUS,           // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // D4: cos / acos / cosh / acosh
            {
                CAS::FuncName::cos,     // 0
                CAS::FuncName::acos,    // 1: SHIFT
                "p",                    // 2: ALPHA
                "P",                    // 3: SHIFT|ALPHA
                CAS::FuncName::cosh,    // 4: CTRL
                CAS::FuncName::acosh,   // 5: SHIFT|CTRL
            },
            // D5: MUL / P / !
            {
                "*",                    // 0
                CAS::FuncName::permut,  // 1: SHIFT
                "q",                    // 2: ALPHA
                "Q",                    // 3: SHIFT|ALPHA
                CAS::FuncName::fact,    // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // D6: DIV / C / mod
            {
                "/",                    // 0
                CAS::FuncName::combin,  // 1: SHIFT
                "r",                    // 2: ALPHA
                "R",                    // 3: SHIFT|ALPHA
                CAS::FuncName::mod,     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            }
        },

        // ==================== Row E ====================
        {
            // E1: 1 / deg
            {
                "1",                    // 0
                CAS::FuncName::deg,     // 1: SHIFT
                "s",                    // 2: ALPHA
                "S",                    // 3: SHIFT|ALPHA
                "",                     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // E2: 2 / rad / Y-
            {
                "2",                    // 0
                CAS::FuncName::rad,     // 1: SHIFT
                "t",                    // 2: ALPHA
                "T",                    // 3: SHIFT|ALPHA
                Ctrl::Y_MINUS,          // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // E3: 3 / Z+
            {
                "3",                    // 0
                "",                     // 1: SHIFT
                "u",                    // 2: ALPHA
                "U",                    // 3: SHIFT|ALPHA
                Ctrl::Z_PLUS,           // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // E4: sin / asin / sinh / asinh
            {
                CAS::FuncName::sin,     // 0
                CAS::FuncName::asin,    // 1: SHIFT
                "v",                    // 2: ALPHA
                "V",                    // 3: SHIFT|ALPHA
                CAS::FuncName::sinh,    // 4: CTRL
                CAS::FuncName::asinh,   // 5: SHIFT|CTRL
            },
            // E5: + / Pol / lcm
            {
                "+",                    // 0
                CAS::FuncName::polar,   // 1: SHIFT
                "w",                    // 2: ALPHA
                "W",                    // 3: SHIFT|ALPHA
                CAS::FuncName::lcm,     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // E6: - / Rec / gcd
            {
                "-",                    // 0
                CAS::FuncName::rect,    // 1: SHIFT
                "x",                    // 2: ALPHA
                "X",                    // 3: SHIFT|ALPHA
                CAS::FuncName::gcd,     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            }
        },

        // ==================== Row F ====================
        {
            // F1: 0 / abs / ZOOM+
            {
                "0",                    // 0
                CAS::FuncName::abs,     // 1: SHIFT
                "y",                    // 2: ALPHA
                "Y",                    // 3: SHIFT|ALPHA
                Ctrl::ZOOM_P,           // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // F2: DOT / i / ZOOM-
            {
                ".",                    // 0
                CAS::ConstName::i,      // 1: SHIFT
                "z",                    // 2: ALPHA
                "Z",                    // 3: SHIFT|ALPHA
                Ctrl::ZOOM_M,           // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // F3: EE / pi / Z-
            {
                Spec::EE,               // 0
                CAS::ConstName::pi,     // 1: SHIFT
                VarName::theta,         // 2: ALPHA
                "",                     // 3: SHIFT|ALPHA
                Ctrl::Z_MINUS,          // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // F4: FMT / e / exp
            {
                Ctrl::FMT,              // 0
                CAS::ConstName::e,      // 1: SHIFT
                VarName::lambda,        // 2: ALPHA
                "",                     // 3: SHIFT|ALPHA
                CAS::FuncName::exp,     // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // F5: Ans / PAns / plt3
            {
                VarName::Ans,           // 0
                VarName::PAns,          // 1: SHIFT
                VarName::mu,            // 2: ALPHA
                "",                     // 3: SHIFT|ALPHA
                CAS::FuncName::plot3d,  // 4: CTRL
                "",                     // 5: SHIFT|CTRL
            },
            // F6: EXE / = / plt2
            {
                Ctrl::EXE,              // 0
                "=",                    // 1: SHIFT
                VarName::alpha,         // 2: ALPHA
                "",                     // 3: SHIFT|ALPHA
                CAS::FuncName::plot2d,  // 4: CTRL
                Ctrl::EXPAND,           // 5: SHIFT|CTRL
            }
        }
    };

    /**
     * @brief Map row, column and modifier to the corresponding key string.
     *
     * @param row   Row index: A=0 .. F=5
     * @param col   Column index: 1=0 .. 6=5
     * @param mod   Modifier bitmask (MOD_SHIFT=1, MOD_ALPHA=2, MOD_CTRL=4)
     * @return      Reference to the key string
     *
     * ALPHA|CTRL (6) and SHIFT|ALPHA|CTRL (7) are treated as ALPHA-lock,
     * mapping to the ALPHA and SHIFT|ALPHA layers respectively.
     */
    inline const std::string& getKey(int row, int col, uint8_t mod) {
        return keypad[row][col][layer_index[mod & 0x7]];
    }
} // namespace Keypad

#endif // _KEYPAD_HPP_