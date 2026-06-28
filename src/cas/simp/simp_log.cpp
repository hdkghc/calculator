/** @file /src/cas/simp/simp_log.cpp
 *  @brief Logarithmic function simplifiers
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

    // ========== Ln ==========

    Exptree* TreeSimplifier::simplifyLn(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // ln(0) -> undefined, leave as-is
        if (SimpUtil::isZero(arg)) {
            return node;
        }

        // ln(1) = 0 (exact)
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // ln(e) = 1 (exact, uses ConstName::e)
        if (SimpUtil::isConstantE(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // ln(e^x) = x
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isConstantE(arg->child[0])) {
                Exptree* inner = SimpUtil::deepCopy(arg->child[1]);
                SimpUtil::freeTree(node);
                return inner;
            }
        }

        // ln(x^a) = a*ln(x) for x > 0
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isPositive(arg->child[0])) {
                Exptree* a = SimpUtil::deepCopy(arg->child[1]);
                Exptree* lnBase = SimpUtil::makeFunction(FuncName::ln);
                lnBase->child.push_back(SimpUtil::deepCopy(arg->child[0]));

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(a);
                result->child.push_back(lnBase);

                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        return node;
    }

    // ========== Log ==========

    Exptree* TreeSimplifier::simplifyLog(Exptree* node) {
        if (node->child.size() < 1) return node;

        Exptree* arg = node->child[0];
        Exptree* base = (node->child.size() >= 2) ? node->child[1] : nullptr;

        // log(1) = 0 for any base
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // log(x, x) = 1
        if (base && SimpUtil::compareNodes(arg, base) == 0) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // log(x, e) = ln(x)
        if (base && SimpUtil::isConstantE(base)) {
            Exptree* lnNode = SimpUtil::makeFunction(FuncName::ln);
            lnNode->child.push_back(SimpUtil::deepCopy(arg));
            SimpUtil::freeTree(node);
            return lnNode;
        }

        // log(x, 10) -> log10(x)
        if (base && SimpUtil::isRational(base) && base->value == Rational(Intg(10))) {
            Exptree* log10Node = SimpUtil::makeFunction(FuncName::log10);
            log10Node->child.push_back(SimpUtil::deepCopy(arg));
            SimpUtil::freeTree(node);
            return log10Node;
        }

        // Convert to natural log: log_b(a) = ln(a)/ln(b)
        if (base) {
            Exptree* lnArg = SimpUtil::makeFunction(FuncName::ln);
            lnArg->child.push_back(SimpUtil::deepCopy(arg));

            Exptree* lnBase = SimpUtil::makeFunction(FuncName::ln);
            lnBase->child.push_back(SimpUtil::deepCopy(base));

            Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
            Exptree* invLnBase = SimpUtil::makeFunction("^");
            invLnBase->child.push_back(lnBase);
            invLnBase->child.push_back(negOne);

            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(lnArg);
            result->child.push_back(invLnBase);

            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        return node;
    }

    // ========== Log10 ==========

    Exptree* TreeSimplifier::simplifyLog10(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // log10(1) = 0
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // log10(10) = 1
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(10))) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // log10(100) = 2
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(100))) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(2)));
        }

        // log10(1000) = 3
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1000))) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(3)));
        }

        // log10(1/10) = -1
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1), Intg(10))) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(-1)));
        }

        // Convert to natural log: log10(x) = ln(x)/ln(10)
        Exptree* lnArg = SimpUtil::makeFunction(FuncName::ln);
        lnArg->child.push_back(SimpUtil::deepCopy(arg));
        Exptree* ln10 = SimpUtil::makeFunction(FuncName::ln);
        ln10->child.push_back(SimpUtil::makeRational(Rational(Intg(10))));
        Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
        Exptree* invLn10 = SimpUtil::makeFunction("^");
        invLn10->child.push_back(ln10);
        invLn10->child.push_back(negOne);
        Exptree* result = SimpUtil::makeFunction("*");
        result->child.push_back(lnArg);
        result->child.push_back(invLn10);
        SimpUtil::freeTree(node);
        return simplifyMul(result);
    }

} // namespace CAS