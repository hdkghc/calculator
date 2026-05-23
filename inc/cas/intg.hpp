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
                trim();
                if(i.empty())
                    return 0;
                if(i.back() & 0xF0) {
                    return (i.size() << 1);
                } else {
                    return (i.size() << 1) - 1;
                }
            }
            /** @name trim
             *  @brief Remove leading zeros from the integer
             */
            void trim(void) {
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
                trim();
            }
        public:
            Intg() : f(0) {}
            // Intg(uint64_t x) : f(0) {
            //     for(size_t i = 0; x; ++i) {
            //         sdg(i, x % 10);
            //         x /= 10;
            //     }
            // }
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
            /** @name abs
             *  @brief Get the absolute value of the integer
             *  @return Absolute value of the integer
            */
            Intg abs(void) {
                Intg ret = *this;
                ret.f &= 0xFE;
                return ret;
            }
            uint8_t operator[](size_t i) {
                return (gdg(i) == (uint8_t)(-1)) ? 0 : gdg(i);
            }
        protected:
            /** @name cmp
             *  @brief Compare two integers
             *  @param lhs Left-hand side integer
             *  @param rhs Right-hand side integer
             *  @return 1 if lhs > rhs, 0 if lhs < rhs, 2 if lhs == rhs
             */
            uint8_t cmp(Intg lhs, Intg rhs) const {
                if((lhs.f & 0x01) ^ (rhs.f & 0x01)) {
                    return (lhs.f & 0x01) ? 0 : 1;
                }
                if(lhs.gl() > rhs.gl()) {
                    return 1;
                } else if(lhs.gl() < rhs.gl()) {
                    return 0;
                }
                for(size_t i = lhs.gl() - 1; i >= 0; --i) {
                    if(lhs[i] > rhs[i]) {
                        return 1;
                    } else if(lhs[i] < rhs[i]) {
                        return 0;
                    }
                }
                return 2;
            }
        public:
            bool operator>(Intg rhs) {
                return cmp(*this, rhs) == 1;
            }
            bool operator<(Intg rhs) {
                return cmp(*this, rhs) == 0;
            }
            bool operator>=(Intg rhs) {
                return cmp(*this, rhs) >= 1;
            }
            bool operator<=(Intg rhs) {
                return cmp(*this, rhs) != 1;
            }
            bool operator==(Intg rhs) {
                return cmp(*this, rhs) == 2;
            }
            bool operator!=(Intg rhs) {
                return cmp(*this, rhs) != 2;
            }
        protected:
            /** @name digmul
             *  @brief Multiply two single digits (0-9) and return the result as a byte (0-99)
             *  @param a First digit (0-9)
             *  @param b Second digit (0-9)
             *  @return The product of the two digits as a byte (0-99)
             *  @usage uint8_t result = digmul(7, 8); // result will be 0x56 in hexadecimal
             */
            uint8_t digmul(uint8_t a, uint8_t b) {
                return makebyte((a * b) / 10, (a * b) % 10);
            }
            /** @name makebyte
             *  @brief Combine two single digits (0-9) into a byte (0-99)
             *  @param hi High digit (0-9)
             *  @param lo Low digit (0-9)
             *  @return The byte created from the two digits
             */
            uint8_t makebyte(uint8_t hi, uint8_t lo) {
                return (hi << 4) | (lo & 0x0F);
            }
            /** @name gethi
             *  @brief Get the high digit of a byte
             */
            uint8_t gethi(uint8_t b) {
                return b >> 4;
            }
            /** @name getlo
             *  @brief Get the low digit of a byte
             */
            uint8_t getlo(uint8_t b) {
                return b & 0x0F;
            }
            /** @name muldig
             *  @brief Multiply the integer by a single digit
             *  @param lhs The integer to be multiplied
             *  @param rhs The single digit multiplier (0-9)
             *  @return The product of the integer and the single digit
             */
            Intg muldig(Intg lhs, uint8_t rhs) {
                uint8_t c = 0;
                size_t l = lhs.gl();
                for(size_t i = 0; i < l; ++i) {
                    uint8_t tmp = lhs[i] * rhs + c;
                    if(tmp > 9) {
                        lhs.sdg(i, tmp % 10);
                        c = tmp / 10;
                    } else {
                        lhs.sdg(i, tmp);
                        c = 0;
                    }
                }
                if(c) {
                    lhs.sdg(l, c);
                }
                return lhs;
            }
            /** @name append0
             *  @brief Append n zeroes to the integer (multiply by 10^n)
             *  @param n Number of zeroes to append
             *  @return The integer after appending zeroes
             */
            Intg append0(size_t n) {
                if(gl() == 0) {
                    return *this;
                }
                if(n & 1) {
                    for(size_t i = gl() - 1; i >= 0; --i) {
                        sdg(i + n, this->operator[](i));
                        sdg(i, 0);
                    }
                } else {
                    i.insert(i.begin(), (n + 1) >> 1, 0);
                }
                return *this;
            }
        public:
            Intg operator*(Intg rhs) {
                Intg ret;
                for(size_t i = 0; i < this->gl(); ++i) {
                    Intg tmp = muldig(rhs, this->operator[](i));
                    tmp.append0(i);
                    ret = ret + tmp;
                }
                ret.f = (this->f ^ rhs.f) & 0x01;
                return ret;
            }
            Intg operator/(Intg rhs) {
                Intg tmp = *this;
                Intg ret;
                for(size_t i = tmp.gl() - rhs.gl(); i >= 0; --i) {
                    Intg t = rhs;
                    t.append0(i);
                    t.f &= 0xFE;
                    while(tmp >= t) {
                        tmp = tmp - t;
                        ret.sdg(i, ret[i] + 1);
                    }
                }
                ret.f = (this->f ^ rhs.f) & 0x01;
                return ret;
            }
            Intg operator%(Intg rhs) {
                Intg tmp = *this;
                for(size_t i = tmp.gl() - rhs.gl(); i >= 0; --i) {
                    Intg t = rhs;
                    t.append0(i);
                    t.f &= 0xFE;
                    while(tmp >= t) {
                        tmp = tmp - t;
                    }
                }
                return tmp;
            }
    };
}

#endif // _CAS_INTG_HPP_