/** @file /inc/cas/intg.hpp
 *  @brief Big integer calculation for the computer algebra system module of the calculator project
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
#ifndef _CAS_INTG_HPP_
#define _CAS_INTG_HPP_

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <utility>

namespace CAS {
    class Intg {
        protected:
            // Store digits in reverse order
            // 4 bits per digit, so each uint8_t can store 2 digits (0-99)
            // For example, the integer 543210 would be stored as:
            //      i[0] = 0x01 (4 in low 4 bits, 5 in high 4 bits)
            //      i[1] = 0x23 (2 in low 4 bits, 3 in high 4 bits)
            //      i[2] = 0x45 (0 in low 4 bits, 1 in high 4 bits)
            std::vector<uint8_t> i;
            // & 0x01 : 1 = negative, 0 = positive
            // & 0x02 : 1 = infinity
            // & 0x04 : 1 = NaN
            uint8_t f;
            /** @name gdg
             *  @brief get the nth digit (0-based) of the integer
             *  @param n Digit index (0-based)
             *  @return The nth digit (0-9)
             */
            uint8_t gdg(size_t n) const {
                if(isInf() || isNaN()) {
                    return InvalidDigit;
                }
                if(n >= (i.size() << 1)) {
                    return InvalidDigit;
                } else if(n == (i.size() << 1) - 1 && (i.back() & 0xF0) == 0) {
                    return InvalidDigit;
                }
                return (i[n >> 1] & (0x0F << ((n & 1) << 2))) >> ((n & 1) << 2);
            }
            /** @name gl
             *  @brief get the number of digits in the integer
             *  @return Number of digits
             */
            size_t gl(void) {
                trim();
                if(isInf() || isNaN()) {
                    return InvalidLength;
                }
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
                if(isInf() || isNaN()) {
                    i.clear();
                    return;
                }
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
                if(isInf() || isNaN()) {
                    trim();
                    return;
                }
                if(gl() == InvalidLength) {
                    return;
                }

                const size_t byteIdx = n >> 1;
                if(i.size() <= byteIdx) {
                    i.resize(byteIdx + 1, 0);
                }

                if(n & 1) {
                    i[byteIdx] &= 0x0F; // clear high nibble
                    i[byteIdx] |= static_cast<uint8_t>(x << 4); // set high nibble
                } else {
                    i[byteIdx] &= 0xF0; // clear low nibble
                    i[byteIdx] |= x;    // set low nibble
                }
                trim();
            }
        public:
            static const uint8_t InvalidDigit = 0xFF;
            static const size_t InvalidLength = SIZE_MAX;

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
                    sdg(i, std::llabs(x) % 10);
                    x /= 10;
                }
            }
            Intg(std::string s) {
                f = 0;
                if(s.find("inf") != std::string::npos) {
                    for(auto &u : s) {
                        if(u == '-') {
                            f ^= 0x01;
                        }
                    }
                    f |= 0x02;
                    return;
                }
                if(s.find("nan") != std::string::npos) {
                    f |= 0x04;
                    return;
                }
                if(s.find("NaN") != std::string::npos) {
                    f |= 0x04;
                    return;
                }

                size_t digitIdx = 0;
                for(auto it = s.rbegin(); it != s.rend(); ++it) {
                    const char u = *it;
                    if(u == '-') {
                        f ^= 0x01;
                    } else if(u >= '0' && u <= '9') {
                        sdg(digitIdx++, u - '0');
                    }
                }
            }
            /** @name isInf
             *  @brief Check if the integer is infinity
             *  @return true if the integer is infinity, false otherwise
             */
            bool isInf(void) const {
                return (f & 0x02) >> 1;
            }
            /** @name isNaN
             *  @brief Check if the integer is NaN
             *  @return true if the integer is NaN, false otherwise
             */
            bool isNaN(void) const {
                return (f & 0x04) >> 2;
            }
            /** @name isNeg
             *  @brief Check if the integer is negative
             *  @return true if the integer is negative, false otherwise
             */
            bool isNeg(void) const {
                return f & 0x01;
            }
            /** @name setInf
             *  @brief Set the integer to infinity
             */
            void setInf(void) {
                f |= 0x02;
            }
            /** @name unsetInf
             *  @brief Unset the infinity flag of the integer
             */
            void unsetInf(void) {
                f &= 0xFD;
            }
            /** @name setNaN
             *  @brief Set the integer to NaN
             */
            void setNaN(void) {
                f |= 0x04;
            }
            /** @name unsetNaN
             *  @brief Unset the NaN flag of the integer
             */
            void unsetNaN(void) {
                f &= 0xFB;
            }
            operator std::string() {
                if(isInf()) {
                    return std::string(isNeg() ? "-" : "+") + "\\infty";
                }
                if(isNaN()) {
                    return "\\color{red}{NaN}";
                }
                if(gl() == InvalidLength) {
                    return "\\color{red}{Invalid}";
                }
                if(gl() == 0) {
                    return "0";
                }
                std::string s;
                if(f & 0x01) s += '-';
                for(size_t i = gl(); i > 0; --i) {
                    s += '0' + gdg(i - 1);
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
                if(lhs.isInf() || lhs.isNaN() || rhs.isInf() || rhs.isNaN()) {
                    if(lhs.isNaN() || rhs.isNaN()) {
                        Intg ret;
                        ret.setNaN();
                        return ret;
                    }
                    if(lhs.isInf() && rhs.isInf() && (lhs.f & 0x01) != (rhs.f & 0x01)) {
                        Intg ret; // inf + (-inf) === inf - inf
                        ret.setNaN();
                        return ret;
                    }
                    return lhs.isInf() ? lhs : rhs; // inf + ? ; inf + inf
                }
                if(lhs.gl() == InvalidLength || rhs.gl() == InvalidLength) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }

                if(lhs.gl() < rhs.gl()) {
                    std::swap(lhs, rhs);
                }
                if((lhs.f & 0x01) ^ (rhs.f & 0x01)) { // minus
                    // & 0x02 : 1 = result is negative, 0 = result is positive
                    // & 0x01 : 1 = borrow, 0 = no borrow
                    uint8_t flg = 0;
                    size_t l = 0;
                    flg |= ((lhs.abs() < rhs.abs()) ? (rhs.f & 0x01) : (lhs.f & 0x01)) << 1;
                    if(lhs.abs() < rhs.abs()) {
                        std::swap(lhs, rhs);
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
                if(isNaN()) {
                    return *this;
                }
                Intg ret = *this;
                ret.f &= 0xFE;
                return ret;
            }
            uint8_t operator[](size_t i) {
                return (gdg(i) == InvalidDigit) ? 0 : gdg(i);
            }
        protected:
            /** @name cmp
             *  @brief Compare two integers
             *  @param lhs Left-hand side integer
             *  @param rhs Right-hand side integer
             *  @return 1 if lhs > rhs, 0 if lhs < rhs, 2 if lhs == rhs, 3 if error
             */
            uint8_t cmp(Intg lhs, Intg rhs) const {
                if(lhs.isNaN() || rhs.isNaN()) {
                    return 3;
                }
                if(lhs.isInf() || rhs.isInf()) {
                    if(lhs.isInf() && rhs.isInf()) {
                        if((lhs.f & 0x01) == (rhs.f & 0x01)) {
                            return 3; // inf - inf = NaN != 0
                        } else {
                            return (lhs.f & 0x01) ? 0 : 1;
                        }
                    }
                    return lhs.isInf() ? 
                        ((lhs.f & 0x01) ? 0 /* -inf < ? */: 1 /* inf > ? */) : 
                        ((rhs.f & 0x01) ? 1 /* ? > -inf */: 0 /* ? < inf */);
                }
                if(lhs.gl() == InvalidLength || rhs.gl() == InvalidLength) {
                    return 3;
                }
                if((lhs.f & 0x01) ^ (rhs.f & 0x01)) {
                    return (lhs.f & 0x01) ? 0 : 1;
                }
                if(lhs.gl() > rhs.gl()) {
                    return 1;
                } else if(lhs.gl() < rhs.gl()) {
                    return 0;
                }
                for(size_t i = lhs.gl(); i > 0; --i) {
                    if(lhs[i - 1] > rhs[i - 1]) {
                        return 1;
                    } else if(lhs[i - 1] < rhs[i - 1]) {
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
            /** @name can_compare
             *  @brief Check if two integers can be compared (not NaN)
             *  @param rhs Right-hand side integer
             *  @return true if both integers can be compared, false otherwise
             */
            bool can_compare(Intg rhs) {
                return cmp(*this, rhs) != 3;
            }
        protected:
            /** @name digmul
             *  @brief Multiply two single digits (0-9) and return the result as a byte (0-99)
             *  @param a First digit (0-9)
             *  @param b Second digit (0-9)
             *  @return The product of the two digits as a byte (0-99)
             *  @example uint8_t result = digmul(7, 8); // result will be 0x56 in hexadecimal
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
                if(lhs.isInf() || lhs.isNaN()) {
                    return lhs;
                }
                if(lhs.gl() == InvalidLength) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }
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
                if(isInf() || isNaN()) {
                    return *this;
                }
                if(gl() == 0 || gl() == InvalidLength) {
                    return *this;
                }
                if(n & 1) {
                    for(size_t i = gl(); i > 0; --i) {
                        sdg(i - 1 + n, this->operator[](i - 1));
                        sdg(i - 1, 0);
                    }
                } else {
                    i.insert(i.begin(), (n + 1) >> 1, 0);
                }
                return *this;
            }
        public:
            Intg operator*(Intg rhs) {
                if(isNaN() || rhs.isNaN()) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }
                if(isInf() || rhs.isInf()) {
                    if((isInf() && rhs == Intg(0)) || (rhs.isInf() && *this == Intg(0))) { // inf * 0
                        Intg ret;
                        ret.setNaN();
                        return ret;
                    }
                    Intg ret; // inf * ? ; ? * inf
                    ret.setInf();
                    ret.f |= (rhs.f ^ this->f) & 0x01;
                    return ret;
                }
                if(gl() == InvalidLength || rhs.gl() == InvalidLength) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }
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
                if(isInf() || rhs.isInf()) {
                    if(isInf() && rhs.isInf()) { // inf / inf
                        Intg ret;
                        ret.setNaN();
                        return ret;
                    }
                    return 
                        isInf() ? 
                            rhs == Intg(0) ? // inf / ?
                                Intg("nan") : // inf / 0
                                (rhs.isNeg() ? -*this : *this) : // inf / - ; inf / +
                            Intg(0); // ? / inf
                }
                if(isNaN() || rhs.isNaN() || rhs == Intg(0)) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }
                if(gl() == InvalidLength || rhs.gl() == InvalidLength) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }

                Intg tmp = this->abs();
                Intg r = rhs.abs();
                if(tmp.gl() < r.gl()) {
                    return Intg(0);
                }
                Intg ret;
                for(size_t i = tmp.gl() - r.gl() + 1; i > 0; --i) {
                    Intg t = r;
                    t.append0(i - 1);
                    while(tmp >= t) {
                        tmp = tmp - t;
                        ret.sdg(i - 1, ret[i - 1] + 1);
                    }
                }
                ret.f = (this->f ^ rhs.f) & 0x01;
                if(ret.gl() == 0) {
                    ret.f = 0;
                }
                return ret;
            }

            Intg operator%(Intg rhs) {
                if(isInf() || isNaN() || rhs.isNaN() || rhs == Intg(0)) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }
                if(rhs.isInf()) {
                    return *this;
                }
                if(gl() == InvalidLength || rhs.gl() == InvalidLength) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }

                Intg tmp = this->abs();
                Intg r = rhs.abs();
                if(tmp.gl() < r.gl()) {
                    return *this;
                }
                for(size_t i = tmp.gl() - r.gl() + 1; i > 0; --i) {
                    Intg t = r;
                    t.append0(i - 1);
                    while(tmp >= t) {
                        tmp = tmp - t;
                    }
                }
                tmp.f = this->f & 0x01;
                if(tmp.gl() == 0) {
                    tmp.f = 0;
                }
                return tmp;
            }
            /** @name divmod
             *  @brief Divide the integer by another integer and return both the quotient and remainder
             *  @param rhs The divisor integer
             *  @return A pair containing the quotient and remainder of the division
             *  @code
             *  auto [quotient, remainder] = a.divmod(b);
             *  @endcode
             */
            std::pair<Intg, Intg> divmod(Intg rhs) {
                if(isInf() || rhs.isInf()) {
                    if(isInf() && rhs.isInf()) { // inf / inf
                        Intg ret;
                        ret.setNaN();
                        return std::make_pair(ret, ret);
                    }
                    return 
                        isInf() ? 
                            rhs == Intg(0) ? // inf / ?
                                std::make_pair(Intg("nan"), Intg("nan")) : // inf / 0
                                (rhs.isNeg() ? std::make_pair(-*this, Intg(0)) : std::make_pair(*this, Intg(0))) : // inf / - ; inf / +
                            std::make_pair(Intg(0), *this); // ? / inf
                }
                if(isNaN() || rhs.isNaN() || rhs == Intg(0)) {
                    Intg ret;
                    ret.setNaN();
                    return std::make_pair(ret, ret);
                }
                if(gl() == InvalidLength || rhs.gl() == InvalidLength) {
                    Intg ret;
                    ret.setNaN();
                    return std::make_pair(ret, ret);
                }

                Intg tmp = this->abs();
                Intg r = rhs.abs();
                if(tmp.gl() < r.gl()) {
                    Intg rem = *this;
                    return std::make_pair(Intg(0), rem);
                }
                Intg ret;
                for(size_t i = tmp.gl() - r.gl() + 1; i > 0; --i) {
                    Intg t = r;
                    t.append0(i - 1);
                    while(tmp >= t) {
                        tmp = tmp - t;
                        ret.sdg(i - 1, ret[i - 1] + 1);
                    }
                }
                ret.f = (this->f ^ rhs.f) & 0x01;
                if(ret.gl() == 0) {
                    ret.f = 0;
                }
                tmp.f = this->f & 0x01;
                if(tmp.gl() == 0) {
                    tmp.f = 0;
                }
                return std::make_pair(ret, tmp);
            }
            /** @name pow
             *  @brief Raise the integer to the power of another integer
             *  @param rhs The exponent integer
             *  @return The result of raising the integer to the power of exp
             */
            Intg pow(Intg rhs) {
                if(rhs.isNaN() || isNaN()) {
                    Intg ret;
                    ret.setNaN();
                    return ret;
                }
                if(rhs.isInf()) { // ? ^ inf
                    if(*this == Intg(1)) { // 1 ^ inf
                        return Intg(1);
                    }
                    if(*this == Intg(-1)) { // (-1) ^ inf
                         return Intg("nan");
                    }
                    if(*this == Intg(0)) { // 0 ^ inf
                        return Intg(0);
                    }
                    if(this->isInf()) {
                        if(this->isNeg()) { // (-inf) ^ inf
                            return Intg("nan");
                        } else { // inf ^ inf
                            return Intg("inf");
                        }
                    }
                    if(rhs.isNeg()) { // ? ^ (-inf)
                        return Intg(0);
                    } else {
                        return isNeg() ? Intg("-inf") /* - ^ inf */: Intg("inf") /* + ^ inf */;
                    }
                }
                if(isInf()) { // inf ^ ?
                    if(rhs.isNeg()) {
                        return Intg(0);
                    } else {
                        return *this;
                    }
                }
                if(rhs.isNeg()) {
                    return *this == Intg(0) ? Intg("inf") : Intg(0); // integer power with negative exponent is 0
                }
                if(*this == Intg(0) && rhs == Intg(0)) { // 0 ^ 0
                    return Intg("nan");
                }
                if(rhs == Intg(0)) {
                    return Intg(1); // integer power with exponent 0 is 1
                }
                if(*this == Intg(0)) {
                    return Intg(0); // 0 ^ positive integer is 0
                }
                Intg ret(1);
                Intg base = *this;
                while(rhs > Intg(0)) {
                    if(rhs[0] & 1) { // odd exponent
                        ret = ret * base;
                    }
                    base = base * base;
                    rhs = rhs / Intg(2);
                }
                return ret;
            }
            /** @name sqrt
             *  @brief Calculate the square root of the integer
             *  @param rhs The exponent integer
             *  @return The result of the square root
             */
            Intg sqrt(Intg rhs) {
                Intg g0, g1;

                g0 = Intg(2);
                g1 = (g0 + (rhs / g0)) / 2;
                while(g1 < g0) {
                    g0 = g1;
                    g1 = (g0 + rhs / g0) / 2;
                }
                return g0;
            }
            /** @name toInt
             *  @brief Turn the integer into int64 (precision loss)
             */
            int64_t toInt() {
                int64_t ret = 0;
                for(size_t i = gl() + 1; i >= 1; i--) {
                    ret = ret * 10 + (*this)[i - 1];
                }
                return ret;
            }
    }; // class Intg
} // namespace CAS

#endif // _CAS_INTG_HPP_