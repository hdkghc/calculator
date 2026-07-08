/** @file inc/charmap.hpp
 *  @brief Char mapping
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
#ifndef _CHARMAP_HPP_
#define _CHARMAP_HPP_

#include "expdef.hpp"
#include "keypad.hpp"
#include <cstdint>
#include <string>
#include <vector>

const char *const C_INF = "inf";

inline std::vector<uint16_t> getCharIndex(std::string s) {
    std::vector<uint16_t> r;
    r.clear();
    if(s == C_INF) {
        r.push_back(1148);
        return r;
    }
    if(s.size() == 1) {
        if(s[0] == '*') { // multiply, U+00D7
            r.push_back(123);
            return r;
        }
        if(s[0] == '/') { // divide, U+00F7
            r.push_back(155);
            return r;
        }
        if(s[0] >= 0x20 && s[0] <= 0x7f) { // is ASCII
            r.push_back(s[0]);
            return r;
        }
        r.push_back(461); // illegal
        return r;
    }
}

#endif