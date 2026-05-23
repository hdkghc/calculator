/** @file /inc/cas/gcd.hpp
 *  @brief GCD & LCM calculation for the computer algebra system module of the calculator project
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
#ifndef _CAS_GCD_HPP_
#define _CAS_GCD_HPP_

#include "cas/intg.hpp"

namespace CAS {
    /** @name gcd
     *  @brief Compute the greatest common divisor of two integers
     *  @param a First integer
     *  @param b Second integer
     *  @return GCD of a and b
     */
    Intg gcd(Intg a, Intg b) {
        if (b > a) std::swap(a, b);
        if (b == Intg(0)) return a;
        return gcd(b, a % b);
    }
    /** @name lcm
     *  @brief Compute the least common multiple of two integers
     *  @param a First integer
     *  @param b Second integer
     *  @return LCM of a and b
     */
    Intg lcm(Intg a, Intg b) {
        if (a == Intg(0) || b == Intg(0)) return Intg(0);
        return (a / gcd(a, b)) * b;
    }
}

#endif // _CAS_GCD_HPP_