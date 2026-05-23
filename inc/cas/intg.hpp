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
#include <cmath>

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
                return i[n >> 1] & (0x0F << (n & 1 << 2)) >> (n & 1 << 2);
            }
            /** @name gl
             *  @brief get the number of digits in the integer
             *  @return Number of digits
             */
            size_t gl(void) {
                aremove();
                if(i.empty())
                    return 0;
                if(i.back() & 0xF0) {
                    return (i.size() << 1);
                } else {
                    return (i.size() << 1) - 1;
                }
            }
            /** @name aremove
             *  @brief Remove leading zeros from the integer
             */
            void aremove(void) {
                while(!i.empty() && i.back() == 0) {
                    i.pop_back();
                }
            }
            /** @name sdg
             *  @brief Set the nth digit (0-based) of the integer
             *  @param n Digit index (0-based)
             *  @param x Digit value (0-9)
             */
            void sdg(size_t n, uint8_t x) {
                if(x > 9) {
                    return;
                }
                while(n + 1 > gl()) {
                    i.push_back(0);
                }
                i[n >> 1] &= 0xF0 >> (n & 1 << 2); // clear digit
                i[n >> 1] |= x << (n & 1 << 2);    // set digit
                aremove();
            }
        public:
            Intg() : f(0) {}
            Intg(uint64_t x) : f(0) {
                for(size_t i = 0; x; ++i) {
                    sdg(i, x % 10);
                    x /= 10;
                }
            }
            Intg(int64_t x) {
                if(x < 0) f = 1;
                else f = 0;
                for(size_t i = 0; x; ++i) {
                    sdg(i, std::abs(x) % 10);
                    x /= 10;
                }
            }
            Intg(std::string s) {
                f = 0;
                size_t i = 0;
                for(auto u : s) {
                    if(u == '-') {
                        f ^= 0x01;
                    } else if(u >= '0' && u <= '9') {
                        sdg(i++, u - '0');
                    }
                }
            }
            operator std::string() {
                std::string s;
                if(f & 0x01) s += '-';
                for(size_t i = gl() - 1; i >= 0; --i) {
                    s += '0' + gdg(i);
                }
                return s.empty() || (s.size() == 1 && s[0] == '-') ? "0" : s;
            }
            Intg operator-() const {
                Intg r = *this;
                r.f ^= 0x01;
                return r;
            }
            Intg operator+(Intg rhs) const {
                Intg lhs = *this;
                if(lhs.gl() > rhs.gl()) {
                    std::swap(lhs, rhs);
                }
                if((lhs.f & 0x01) ^ (rhs.f & 0x01)) { // minus
                    // & 0x02 : 1 = result is negative, 0 = result is positive
                    // & 0x01 : 1 = borrow, 0 = no borrow
                    uint8_t flg = 0;
                    size_t l = 0;
                    if(lhs.abs() > rhs.abs()) {
                        std::swap(lhs, rhs);
                        flg |= 0x02;
                    }
                    l = lhs.gl();
                    for(size_t i = 0; i < l; ++i) {
                        if(lhs[i] < rhs[i] + (flg & 0x01)) {
                            lhs.sdg(i, (uint8_t)(10 + lhs[i] - rhs[i] - (flg & 0x01)));
                            flg |= 0x01;
                        } else {
                            lhs.sdg(i, (uint8_t)(lhs[i] - rhs[i] - (flg & 0x01)));
                            flg &= 0xFE;
                        }
                    }
                    lhs.f = (flg & 0x02) >> 1;
                    return lhs;
                } else { // plus
                    // & 0x01 : 1 = carry, 0 = no carry
                    uint8_t c = 0;
                    size_t l = lhs.gl();
                    for(size_t i = 0; i < l; ++i) {
                        if(lhs[i] + rhs[i] + c > 9) {
                            lhs.sdg(i, (uint8_t)(lhs[i] + rhs[i] + c - 10));
                            c |= 0x01;
                        } else {
                            lhs.sdg(i, (uint8_t)(lhs[i] + rhs[i] + c));
                            c &= 0xFE;  
                        }
                    }
                    if(c) {
                        lhs.sdg(l, 1);
                    }
                    return lhs;
                }
            } // operator+
            Intg operator-(Intg rhs) const {
                return *this + (-rhs);
            } // operator-
            
    };
}

#endif // _CAS_INTG_HPP_