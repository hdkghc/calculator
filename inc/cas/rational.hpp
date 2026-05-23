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
        protected:
            /** @name num
             *  @brief The numerator of the rational number
             */
            Intg num;
            /** @name den
             *  @brief The denominator of the rational number
             */
            Intg den;
        public:
            /** @name simplify
             *  @brief Simplifies the rational number
             */
            void simplify(void) {
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
    };
}

#endif // _CAS_RATIONAL_HPP_