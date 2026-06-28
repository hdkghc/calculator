/** @file /src/cas/simp/simp_core.cpp
 *  @brief Core recursive simplification dispatcher
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

#include "cas/treesimp.hpp"

namespace CAS {

    Exptree* TreeSimplifier::simplifyNodeOnce(Exptree* node) {
        if (!node) return nullptr;
        if (node->valtp == Exptree::val_t::valRational ||
            node->valtp == Exptree::val_t::valVariable) {
            return node;
        }
        if (node->valtp != Exptree::val_t::valFunction) return node;

        const std::string& func = node->var;

        // All nodes that need preTransform before recursive simplification
        if (func == "-" || func == "/" ||
            func == FuncName::sqrt || func == FuncName::exp ||
            func == FuncName::root ||
            func == FuncName::sinh || func == FuncName::cosh || func == FuncName::tanh ||
            func == FuncName::asinh || func == FuncName::acosh || func == FuncName::atanh ||
            func == FuncName::deg || func == FuncName::rad ||
            SimpUtil::isConstantPhi(node)) {
            preTransform(node);
            return simplifyNode(node);
        }

        for (size_t i = 0; i < node->child.size(); ++i) {
            node->child[i] = simplifyNode(node->child[i]);
        }

        // Arithmetic
        if (func == "+") return simplifyAdd(node);
        if (func == "*") return simplifyMul(node);
        if (func == "^") return simplifyPow(node);

        // Trigonometric
        if (func == FuncName::sin)          return simplifySin(node);
        if (func == FuncName::cos)          return simplifyCos(node);
        if (func == FuncName::tan)          return simplifyTan(node);
        if (func == FuncName::asin)         return simplifyAsin(node);
        if (func == FuncName::acos)         return simplifyAcos(node);
        if (func == FuncName::atan)         return simplifyAtan(node);

        // Hyperbolic
        if (func == FuncName::sinh)         return simplifySinh(node);
        if (func == FuncName::cosh)         return simplifyCosh(node);
        if (func == FuncName::tanh)         return simplifyTanh(node);
        if (func == FuncName::asinh)        return simplifyAsinh(node);
        if (func == FuncName::acosh)        return simplifyAcosh(node);
        if (func == FuncName::atanh)        return simplifyAtanh(node);

        // Logarithmic
        if (func == FuncName::ln)           return simplifyLn(node);
        if (func == FuncName::log)          return simplifyLog(node);
        if (func == FuncName::log10)        return simplifyLog10(node);

        // Other elementary functions
        if (func == FuncName::abs)          return simplifyAbs(node);
        if (func == FuncName::sign)         return simplifySignum(node);
        if (func == FuncName::fact)         return simplifyFact(node);

        // Angle conversion
        if (func == FuncName::deg)          return simplifyDeg(node);
        if (func == FuncName::rad)          return simplifyRad(node);

        // Complex number functions
        if (func == FuncName::realpart)     return simplifyRealpart(node);
        if (func == FuncName::imagpart)     return simplifyImagpart(node);
        if (func == FuncName::conjg)        return simplifyConjg(node);
        if (func == FuncName::arg)          return simplifyArg(node);

        // Number theory
        if (func == FuncName::mod)          return simplifyMod(node);
        if (func == FuncName::gcd)          return simplifyGcd(node);
        if (func == FuncName::lcm)          return simplifyLcm(node);

        // Rounding
        if (func == FuncName::floor)        return simplifyFloor(node);
        if (func == FuncName::ceil)         return simplifyCeil(node);
        if (func == FuncName::frac)         return simplifyFrac(node);
        if (func == FuncName::round)        return simplifyRound(node);

        // Combinatorics
        if (func == FuncName::permut)       return simplifyPermut(node);
        if (func == FuncName::combin)       return simplifyCombin(node);

        // Coordinate transformation
        if (func == FuncName::polar)        return simplifyPolar(node);
        if (func == FuncName::rect)         return simplifyRect(node);

        // Min/Max
        if (func == FuncName::max)          return simplifyMax(node);
        if (func == FuncName::min)          return simplifyMin(node);

        // Random (pass-through)
        if (func == FuncName::randrat)      return simplifyRandrat(node);
        if (func == FuncName::randint)      return simplifyRandint(node);

        // Vector/Matrix
        if (func == FuncName::vector)       return simplifyVector(node);
        if (func == FuncName::matrix)       return simplifyMatrix(node);
        if (func == FuncName::dot)          return simplifyDot(node);
        if (func == FuncName::angle)        return simplifyAngle(node);
        if (func == FuncName::det)          return simplifyDet(node);
        if (func == FuncName::transpose)    return simplifyTranspose(node);

        return node;
    }
    
    Exptree* TreeSimplifier::simplifyNode(Exptree* node) {
        if (!node) return nullptr;

        Exptree* result = node;
        // Exptree* prev;
        for (int8_t i = 0; i < 3; ++i) {
            // prev = result;
            result = simplifyNodeOnce(result);
        }

        return result;
    }

} // namespace CAS