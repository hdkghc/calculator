/** @file /inc/cas/rational.hpp
 *  @brief Rational number calculation for the computer algebra system module of the calculator project
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
#ifndef _CAS_RATIONAL_HPP_
#define _CAS_RATIONAL_HPP_

#include "cas/intg.hpp"
#include "cas/gcd.hpp"

namespace CAS {
    class Rational {
        public:
            /** @name num
             *  @brief The numerator of the rational number
             */
            Intg num;
            /** @name den
             *  @brief The denominator of the rational number
             */
            Intg den;
            /** @name simplify
             *  @brief Simplifies the rational number
             */
            void simplify(void) {
                if(den < Intg(0)) {
                    num = -num;
                    den = -den;
                }
                if(num.isNaN() || den.isNaN()) {
                    num.setNaN();
                    den.setNaN();
                    return;
                }
                if(num.isInf() && den.isInf()) {
                    num.setNaN();
                    den.setNaN();
                    return;
                }
                if(num.isInf()) {
                    den = Intg(1);
                    return;
                }
                if(den.isInf()) {
                    num = Intg(0);
                    den = Intg(1);
                    return;
                }
                if(den == Intg(0)) {
                    den = Intg(1);
                    num.setInf();
                    return;
                }
                if(num == Intg(0)) {
                    den = Intg(1);
                    return;
                }
                Intg g = gcd(num.abs(), den.abs());
                num = num / g;
                den = den / g;
                if(den < Intg(0)) {
                    num = -num;
                    den = -den;
                }
            }
            Rational(Intg num, Intg den) : num(num), den(den) {
                simplify();
            }
            Rational(Intg num) : num(num), den(Intg(1)) {}
            Rational() : num(Intg(0)), den(Intg(1)) {}
            Rational operator+(Rational rhs) {
                Rational ret(num * rhs.den + rhs.num * den, den * rhs.den);
                ret.simplify();
                return ret;
            }
            Rational operator-(Rational rhs) {
                Rational ret(num * rhs.den - rhs.num * den, den * rhs.den);
                ret.simplify();
                return ret;
            }
            Rational operator*(Rational rhs) {
                Rational ret(num * rhs.num, den * rhs.den);
                ret.simplify();
                return ret;
            }
            Rational operator/(Rational rhs) {
                Rational ret(num * rhs.den, den * rhs.num);
                ret.simplify();
                return ret;
            }
            /** @name can_compare
             *  @brief Check if two rational numbers can be compared (not NaN)
             *  @param rhs Right-hand side rational number
             *  @return true if both numbers can be compared, false otherwise
             */
            bool can_compare(Rational rhs) {
                this->simplify();
                rhs.simplify();
                return !num.isNaN() && !rhs.num.isNaN();
            }
            bool operator==(Rational rhs) {
                return (num == rhs.num) && (den == rhs.den);
            }
            bool operator!=(Rational rhs) {
                return !(*this == rhs);
            }
            bool operator<(Rational rhs) {
                return (num * rhs.den) < (rhs.num * den);
            }
            bool operator<=(Rational rhs) {
                return (num * rhs.den) <= (rhs.num * den);
            }
            bool operator>(Rational rhs) {
                return (num * rhs.den) > (rhs.num * den);
            }
            bool operator>=(Rational rhs) {
                return (num * rhs.den) >= (rhs.num * den);
            }
            operator std::string() {
                if(den == Intg(1)) {
                    return std::string(num);
                } else {
                    return "\\frac{" + std::string(num) + "}{" + std::string(den) + "}";
                }
            }
            /** @name isZero
             *  @return True if this is zero
             */
            bool isZero() {
                simplify();
                return num == 0;
            }
            /** @name isInteger
             *  @return True if this is integer
             */
            bool isInteger() {
                simplify();
                return den == 1;
            }
            /** @name numerator
             *  @return The numerator of the rational number
             */
            Intg numerator() {
                simplify();
                return num;
            }
    };
}

#endif // _CAS_RATIONAL_HPP_