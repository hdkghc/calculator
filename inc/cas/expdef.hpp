/** @file /inc/cas/expdef.hpp
 *  @brief Expression tree node definition for the computer algebra system module of the calculator project
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
#ifndef _CAS_EXPDEF_HPP_
#define _CAS_EXPDEF_HPP_

namespace CAS {
    namespace FuncName {
        // \00x represents that they're built-in functions.

        // Trigonometric functions
        const char *const sin       = "\001si";
        const char *const cos       = "\001co";
        const char *const tan       = "\001ta";
        const char *const asin      = "\001Si";
        const char *const acos      = "\001Co";
        const char *const atan      = "\001At";
        const char *const sinh      = "\001sh";
        const char *const cosh      = "\001ch";
        const char *const tanh      = "\001th";
        const char *const asinh     = "\001Sh";
        const char *const acosh     = "\001Ch";
        const char *const atanh     = "\001Th";

        // Logarithmic and exponential functions
        const char *const ln        = "\001ln";
        const char *const log       = "\001lg";
        const char *const log10     = "\001lX";
        const char *const exp       = "\001ex";
        const char *const sqrt      = "\001sq";
        const char *const root      = "\001rt";

        // Numeric functions
        const char *const abs       = "\001ab";
        const char *const floor     = "\001fl";
        const char *const ceil      = "\001ce";
        const char *const round     = "\001ro";
        const char *const sign      = "\001sg";
        const char *const max       = "\001mx";
        const char *const min       = "\001mn";
        const char *const frac      = "\001fr";
        const char *const fact      = "\001f!";
        const char *const gcd       = "\001gc";
        const char *const lcm       = "\001lc";
        const char *const mod       = "\001md";
        const char *const permut    = "\001pm";
        const char *const combin    = "\001cb";

        // Summation and product functions
        const char *const sum       = "\001sm";
        const char *const prod      = "\001pd";
        const char *const defint    = "\001in"; // definite integral
        const char *const diff      = "\001df";
        const char *const indefint  = "\001id"; // indefinite integral

        // Coordinate transformation functions
        const char *const polar     = "\001pl";
        const char *const rect      = "\001rc";

        // Angle conversion functions
        const char *const deg       = "\001dg";
        const char *const rad       = "\001rd";
        
        // Random number generation functions
        const char *const randrat   = "\001rr";
        const char *const randint   = "\001ri";

        // Complex number functions
        const char *const realpart  = "\001re";
        const char *const imagpart  = "\001im";
        const char *const conjg     = "\001cj";
        const char *const arg       = "\001ar";

        // Vector and matrix functions
        const char *const vector    = "\001vc";
        const char *const dot       = "\001v.";
        const char *const angle     = "\001va";
        const char *const det       = "\001dt";
        const char *const matrix    = "\001mx";
        const char *const transpose = "\001tr";
        const char *const eigenval  = "\001Ex";
        const char *const eigenvec  = "\001Ev";
        const char *const adjoint   = "\001aj";
        const char *const rank      = "\001rk";

        // Statistical functions
        const char *const estx1     = "\001s1"; // estimate x1
        const char *const estx2     = "\001s2"; // estimate x2
        const char *const esty      = "\001sy"; // estimate y

        // Bitwise computation functions
        const char *const band      = "\001b&";
        const char *const bor       = "\001b|";
        const char *const bxor      = "\001b^";
        const char *const bnot      = "\001b~";
        const char *const blshift   = "\001b<";
        const char *const brshift   = "\001b>";
        const char *const bxnor     = "\001!^";
        const char *const bnand     = "\001!&";
        const char *const bnor      = "\001!|";
    } // namespace FuncName
} // namespace CAS

#endif // _CAS_EXPDEF_HPP_