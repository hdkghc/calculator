/** @file /inc/cas/intg.hpp
 *  @brief Big integer calculation for the computer algebra system module of the calculator project
 *  @author hdkghc
 *  @date 2026.05.23
 *  @version 0.1
 *  License: GNU General Public License v3.0
 */
#ifndef _CAS_INTG_HPP_
#define _CAS_INTG_HPP_

#include <vector>
#include <string>
#include <cstdint>

namespace CAS {
    class Intg {
        protected:
            // Store digits in reverse order
            // 4 bits per digit, so each uint8_t can store 2 digits (0-99)
            std::vector<uint8_t> i;
            // & 0x01 : 1 = negative, 0 = positive
            uint8_t f;
            /** @name gdg
             *  @brief get the nth digit (0-based) of the integer
             *  @param n Digit index (0-based)
             *  @return The nth digit (0-9)
             */
            uint8_t gdg(size_t n) const {
                if(n >= i.size() << 1) {
                    return -1;
                } else if(n == (i.size() << 1) - 1 && i.back() & 0xF0 == 0) {
                    return -1;
                }
                return i[n >> 1] & (0xF << (n & 1 << 2)) >> (n & 1 << 2);
            }
            
    };
}

#endif // _CAS_INTG_HPP_