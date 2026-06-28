/** @file /src/cas/simp/simp_misc.cpp
 *  @brief Other function simplifiers (sqrt, abs, sign, fact, deg, rad)
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

    // ========== Sqrt ==========

    Exptree* TreeSimplifier::simplifySqrt(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // sqrt(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // sqrt(1) = 1
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // sqrt of perfect square rational
        if (SimpUtil::isRational(arg)) {
            Rational root;
            if (isPerfectSquare(arg->value, root)) {
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(root);
            }

            // Extract perfect square factors from numerator and denominator
            if (SimpUtil::isPositive(arg)) {
                Intg num = arg->value.numerator();
                Intg den = arg->value.den;
                Rational outside(Intg(1));
                Rational inside(Intg(1));

                // Try to extract squares from numerator
                Intg sqrtNum = num.sqrt(num);
                if (sqrtNum * sqrtNum == num) {
                    outside = Rational(sqrtNum, Intg(1));
                    num = Intg(1);
                }

                // Try to extract squares from denominator
                Intg sqrtDen = den.sqrt(den);
                if (sqrtDen * sqrtDen == den) {
                    Rational denFactor(Intg(1), sqrtDen);
                    outside = outside * denFactor;
                    den = Intg(1);
                }

                if (outside != Rational(Intg(1))) {
                    Rational remaining(num, den);
                    if (remaining == Rational(Intg(1))) {
                        SimpUtil::freeTree(node);
                        return SimpUtil::makeRational(outside);
                    }

                    Exptree* result = SimpUtil::makeFunction("*");
                    result->child.push_back(SimpUtil::makeRational(outside));

                    Exptree* innerSqrt = SimpUtil::makeFunction(FuncName::sqrt);
                    innerSqrt->child.push_back(SimpUtil::makeRational(remaining));
                    result->child.push_back(innerSqrt);

                    SimpUtil::freeTree(node);
                    return simplifyMul(result);
                }
            }
        }

        // sqrt(x^2) = abs(x)
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isRational(arg->child[1]) && arg->child[1]->value == Rational(Intg(2))) {
                Exptree* absNode = SimpUtil::makeFunction(FuncName::abs);
                absNode->child.push_back(SimpUtil::deepCopy(arg->child[0]));
                SimpUtil::freeTree(node);
                return simplifyAbs(absNode);
            }
        }

        // sqrt(-1) = i, sqrt(-a) = i*sqrt(a)
        // Build as power and let simplifyPow handle it
        Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
        node->var = "^";
        node->child.clear();
        node->child.push_back(SimpUtil::deepCopy(arg));
        node->child.push_back(half);
        return simplifyPow(node);
    }

    // ========== Abs ==========

    Exptree* TreeSimplifier::simplifyAbs(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // abs(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // abs(abs(x)) = abs(x)
        if (SimpUtil::isFunction(arg, FuncName::abs)) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // abs(n) = n for n > 0
        if (SimpUtil::isPositive(arg)) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // abs(-n) = n for n < 0
        if (SimpUtil::isNegative(arg)) {
            Rational posVal = Rational(Intg(0)) - arg->value;
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(posVal);
        }

        // abs(-x) = abs(x) where -x is represented as (-1)*x
        if (SimpUtil::isFunction(arg, "*")) {
            if (!arg->child.empty() && SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* remaining = SimpUtil::makeFunction("*");
                for (size_t i = 1; i < arg->child.size(); ++i) {
                    remaining->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                }
                if (arg->child.size() == 2) {
                    SimpUtil::freeTree(remaining);
                    remaining = SimpUtil::deepCopy(arg->child[1]);
                }

                Exptree* newAbs = SimpUtil::makeFunction(FuncName::abs);
                newAbs->child.push_back(remaining);
                SimpUtil::freeTree(node);
                return simplifyAbs(newAbs);
            }
        }

        // abs(x*y) = abs(x)*abs(y)
        if (SimpUtil::isFunction(arg, "*")) {
            Exptree* result = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isRational(arg->child[i])) {
                    Rational val = arg->child[i]->value;
                    if (val < Rational(Intg(0))) {
                        val = Rational(Intg(0)) - val;
                    }
                    result->child.push_back(SimpUtil::makeRational(val));
                } else {
                    Exptree* absChild = SimpUtil::makeFunction(FuncName::abs);
                    absChild->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    result->child.push_back(absChild);
                }
            }
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // abs(x^n) = abs(x)^n for even integer n
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isEvenInteger(arg->child[1])) {
                Exptree* absBase = SimpUtil::makeFunction(FuncName::abs);
                absBase->child.push_back(SimpUtil::deepCopy(arg->child[0]));

                Exptree* result = SimpUtil::makeFunction("^");
                result->child.push_back(absBase);
                result->child.push_back(SimpUtil::deepCopy(arg->child[1]));

                SimpUtil::freeTree(node);
                return result;
            }
        }

        return node;
    }

    // ========== Signum ==========

    Exptree* TreeSimplifier::simplifySignum(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // signum(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // signum(n) = 1 for n > 0
        if (SimpUtil::isPositive(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // signum(-n) = -1 for n < 0
        if (SimpUtil::isNegative(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(-1)));
        }

        // signum(signum(x)) = signum(x)
        if (SimpUtil::isFunction(arg, FuncName::sign)) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // signum(-x) = -signum(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner = nullptr;
                if (arg->child.size() == 2) {
                    inner = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }

                Exptree* innerSign = SimpUtil::makeFunction(FuncName::sign);
                innerSign->child.push_back(inner);
                innerSign = simplifySignum(innerSign);

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                result->child.push_back(innerSign);

                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        // signum(x*y) = signum(x)*signum(y)
        if (SimpUtil::isFunction(arg, "*")) {
            Exptree* result = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < arg->child.size(); ++i) {
                Exptree* signChild = SimpUtil::makeFunction(FuncName::sign);
                signChild->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                result->child.push_back(signChild);
            }
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // signum(abs(x)) = 1 (for x != 0)
        if (SimpUtil::isFunction(arg, FuncName::abs)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        return node;
    }

    // ========== Factorial ==========

    Exptree* TreeSimplifier::simplifyFact(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // 0! = 1
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // 1! = 1
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // n! for small positive integers
        if (SimpUtil::isInteger(arg) && SimpUtil::isPositive(arg)) {
            Intg n = arg->value.numerator();
            if (n <= Intg(20)) {
                Intg result(1);
                for (Intg i(2); i <= n; i = i + Intg(1)) {
                    result = result * i;
                }
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(result));
            }
        }

        return node;
    }

    // ========== Deg ==========

    Exptree* TreeSimplifier::simplifyDeg(Exptree* node) {
        preTransform(node);
        return simplifyNode(node);
    }

    // ========== Rad ==========

    Exptree* TreeSimplifier::simplifyRad(Exptree* node) {
        preTransform(node);
        return simplifyNode(node);
    }

} // namespace CAS