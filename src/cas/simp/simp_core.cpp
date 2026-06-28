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

    Exptree* TreeSimplifier::simplifyNode(Exptree* node) {
        if (!node) return nullptr;
        if (node->valtp == Exptree::val_t::valRational ||
            node->valtp == Exptree::val_t::valVariable) {
            return node;
        }
        if (node->valtp != Exptree::val_t::valFunction) return node;

        for (size_t i = 0; i < node->child.size(); ++i) {
            node->child[i] = simplifyNode(node->child[i]);
        }

        const std::string& func = node->var;

        // Arithmetic
        if (func == "+") return simplifyAdd(node);
        if (func == "*") return simplifyMul(node);
        if (func == "^") return simplifyPow(node);

        // Trigonometric
        if (func == FuncName::sin)   return simplifySin(node);
        if (func == FuncName::cos)   return simplifyCos(node);
        if (func == FuncName::tan)   return simplifyTan(node);
        if (func == FuncName::asin)  return simplifyAsin(node);
        if (func == FuncName::acos)  return simplifyAcos(node);
        if (func == FuncName::atan)  return simplifyAtan(node);

        // Hyperbolic (now converted to exponential form in preTransform,
        // but we keep the handlers for cases where preTransform wasn't applied)
        if (func == FuncName::sinh)  return simplifySinh(node);
        if (func == FuncName::cosh)  return simplifyCosh(node);
        if (func == FuncName::tanh)  return simplifyTanh(node);
        if (func == FuncName::asinh) return simplifyAsinh(node);
        if (func == FuncName::acosh) return simplifyAcosh(node);
        if (func == FuncName::atanh) return simplifyAtanh(node);

        // Logarithmic
        if (func == FuncName::ln)    return simplifyLn(node);
        if (func == FuncName::log)   return simplifyLog(node);
        if (func == FuncName::log10) return simplifyLog10(node);

        // Other elementary functions
        if (func == FuncName::abs)   return simplifyAbs(node);
        if (func == FuncName::sign)  return simplifySignum(node);
        if (func == FuncName::fact)  return simplifyFact(node);

        // Angle conversion functions (if not pre-transformed)
        if (func == FuncName::deg)   return simplifyDeg(node);
        if (func == FuncName::rad)   return simplifyRad(node);

        return node;
    }

} // namespace CAS