/** @file /src/cas/simp/simp_number.cpp
 *  @brief Number theory and rounding function simplifiers
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

    // ========== Mod ==========

    Exptree* TreeSimplifier::simplifyMod(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* a = node->child[0];
        Exptree* b = node->child[1];

        // mod(a, 0) -> NaN
        if (SimpUtil::isZero(b)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg("NaN")));
        }

        // mod(0, b) = 0
        if (SimpUtil::isZero(a)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // mod(a, 1) = 0
        if (SimpUtil::isOne(b)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // mod(a, -1) = 0
        if (SimpUtil::isMinusOne(b)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // Compute for integer arguments
        if (SimpUtil::isInteger(a) && SimpUtil::isInteger(b)) {
            Intg aVal = a->value.numerator();
            Intg bVal = b->value.numerator();

            // Handle negative modulus
            if (bVal < Intg(0)) bVal = Intg(0) - bVal;

            Intg result = aVal % bVal;
            if (result < Intg(0)) result = result + bVal;

            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        return node;
    }

    // ========== Gcd ==========

    Exptree* TreeSimplifier::simplifyGcd(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* a = node->child[0];
        Exptree* b = node->child[1];

        // gcd(a, 0) = |a|
        if (SimpUtil::isZero(b)) {
            if (SimpUtil::isInteger(a)) {
                Intg aVal = a->value.numerator();
                if (aVal < Intg(0)) aVal = Intg(0) - aVal;
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(aVal));
            }
            return node;
        }

        // gcd(0, b) = |b|
        if (SimpUtil::isZero(a)) {
            if (SimpUtil::isInteger(b)) {
                Intg bVal = b->value.numerator();
                if (bVal < Intg(0)) bVal = Intg(0) - bVal;
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(bVal));
            }
            return node;
        }

        // gcd(a, 1) = 1
        if (SimpUtil::isOne(b) || SimpUtil::isMinusOne(b)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // gcd(1, b) = 1
        if (SimpUtil::isOne(a) || SimpUtil::isMinusOne(a)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // Compute for integer arguments using Euclidean algorithm
        if (SimpUtil::isInteger(a) && SimpUtil::isInteger(b)) {
            Intg aVal = a->value.numerator();
            Intg bVal = b->value.numerator();
            if (aVal < Intg(0)) aVal = Intg(0) - aVal;
            if (bVal < Intg(0)) bVal = Intg(0) - bVal;

            while (!(bVal == Intg(0))) {
                Intg temp = bVal;
                bVal = aVal % bVal;
                aVal = temp;
            }

            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(aVal));
        }

        return node;
    }

    // ========== Lcm ==========

    Exptree* TreeSimplifier::simplifyLcm(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* a = node->child[0];
        Exptree* b = node->child[1];

        // lcm(a, 0) = 0
        if (SimpUtil::isZero(b)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // lcm(0, b) = 0
        if (SimpUtil::isZero(a)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // lcm(a, 1) = |a|
        if (SimpUtil::isOne(b)) {
            if (SimpUtil::isInteger(a)) {
                Intg aVal = a->value.numerator();
                if (aVal < Intg(0)) aVal = Intg(0) - aVal;
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(aVal));
            }
            return node;
        }

        // lcm(1, b) = |b|
        if (SimpUtil::isOne(a)) {
            if (SimpUtil::isInteger(b)) {
                Intg bVal = b->value.numerator();
                if (bVal < Intg(0)) bVal = Intg(0) - bVal;
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(bVal));
            }
            return node;
        }

        // lcm = |a*b| / gcd(a,b)
        if (SimpUtil::isInteger(a) && SimpUtil::isInteger(b)) {
            Intg aVal = a->value.numerator();
            Intg bVal = b->value.numerator();
            if (aVal < Intg(0)) aVal = Intg(0) - aVal;
            if (bVal < Intg(0)) bVal = Intg(0) - bVal;

            Intg aa = aVal, bb = bVal;
            while (!(bb == Intg(0))) {
                Intg temp = bb;
                bb = aa % bb;
                aa = temp;
            }
            Intg gcdVal = aa;

            if (!(gcdVal == Intg(0))) {
                Intg lcmVal = (aVal / gcdVal) * bVal;
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(lcmVal));
            }
        }

        return node;
    }

    // ========== Floor ==========

    Exptree* TreeSimplifier::simplifyFloor(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // floor(integer) = integer
        if (SimpUtil::isInteger(arg)) {
            Exptree* result = SimpUtil::deepCopy(arg);
            SimpUtil::freeTree(node);
            return result;
        }

        // floor(positive rational) = integer part
        if (SimpUtil::isRational(arg) && SimpUtil::isPositive(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;
            Intg result = num / den;
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        // floor(negative rational)
        if (SimpUtil::isRational(arg) && SimpUtil::isNegative(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;
            Intg result = num / den;
            if (!(num % den == Intg(0)) && num < Intg(0)) {
                result = result - Intg(1);
            }
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        return node;
    }

    // ========== Ceil ==========

    Exptree* TreeSimplifier::simplifyCeil(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // ceil(integer) = integer
        if (SimpUtil::isInteger(arg)) {
            Exptree* result = SimpUtil::deepCopy(arg);
            SimpUtil::freeTree(node);
            return result;
        }

        // ceil(positive rational)
        if (SimpUtil::isRational(arg) && SimpUtil::isPositive(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;
            Intg result = num / den;
            if (!(num % den == Intg(0))) {
                result = result + Intg(1);
            }
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        // ceil(negative rational)
        if (SimpUtil::isRational(arg) && SimpUtil::isNegative(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;
            Intg result = num / den;
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        return node;
    }

    // ========== Round ==========

    Exptree* TreeSimplifier::simplifyRound(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // round(integer) = integer
        if (SimpUtil::isInteger(arg)) {
            Exptree* result = SimpUtil::deepCopy(arg);
            SimpUtil::freeTree(node);
            return result;
        }

        // round(rational) = nearest integer
        if (SimpUtil::isRational(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;
            bool isNeg = (num < Intg(0));
            if (isNeg) num = Intg(0) - num;

            Intg halfDen = den / Intg(2);
            Intg result = (num + halfDen) / den;
            if (isNeg) result = Intg(0) - result;

            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        return node;
    }

    // ========== Frac ==========

    Exptree* TreeSimplifier::simplifyFrac(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // frac(integer) = 0
        if (SimpUtil::isInteger(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // frac(rational) = rational - floor(rational)
        if (SimpUtil::isRational(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;

            Intg intPart = num / den;
            if (!(num % den == Intg(0)) && num < Intg(0)) {
                intPart = intPart - Intg(1);
            }

            Rational floorVal(intPart, Intg(1));
            Rational fracVal = arg->value - floorVal;

            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(fracVal);
        }

        return node;
    }

    // ========== Min / Max ==========

    Exptree* TreeSimplifier::simplifyMax(Exptree* node) {
        if (node->child.size() < 1) return node;
        if (node->child.size() == 1) {
            Exptree* result = SimpUtil::deepCopy(node->child[0]);
            SimpUtil::freeTree(node);
            return result;
        }

        // Check for inf: max(anything, inf) = inf
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (SimpUtil::isRational(node->child[i]) && node->child[i]->value.num.isInf()) {
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(Intg("inf")));
            }
        }

        // Check if all children are rational
        bool allRational = true;
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (!SimpUtil::isRational(node->child[i])) {
                allRational = false;
                break;
            }
        }

        if (allRational) {
            Rational maxVal = node->child[0]->value;
            for (size_t i = 1; i < node->child.size(); ++i) {
                if (node->child[i]->value > maxVal) {
                    maxVal = node->child[i]->value;
                }
            }
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(maxVal);
        }

        // Collect max rational, deduplicate non-rationals
        Rational maxRational;
        bool hasRational = false;
        std::vector<Exptree*> nonRational;
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (SimpUtil::isRational(node->child[i])) {
                if (!hasRational || node->child[i]->value > maxRational) {
                    maxRational = node->child[i]->value;
                    hasRational = true;
                }
            } else {
                bool dup = false;
                for (size_t j = 0; j < nonRational.size(); ++j) {
                    if (SimpUtil::compareNodes(node->child[i], nonRational[j]) == 0) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) nonRational.push_back(node->child[i]);
            }
        }

        if (nonRational.empty()) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(maxRational);
        }

        Exptree* result = SimpUtil::makeFunction(FuncName::max);
        if (hasRational) {
            result->child.push_back(SimpUtil::makeRational(maxRational));
        }
        for (size_t i = 0; i < nonRational.size(); ++i) {
            result->child.push_back(SimpUtil::deepCopy(nonRational[i]));
        }

        if (result->child.size() == 1) {
            Exptree* single = result->child[0];
            result->child.clear();
            SimpUtil::freeTree(result);
            SimpUtil::freeTree(node);
            return single;
        }

        SimpUtil::freeTree(node);
        return result;
    }

    Exptree* TreeSimplifier::simplifyMin(Exptree* node) {
        if (node->child.size() < 1) return node;
        if (node->child.size() == 1) {
            Exptree* result = SimpUtil::deepCopy(node->child[0]);
            SimpUtil::freeTree(node);
            return result;
        }

        // Check for -inf: min(anything, -inf) = -inf
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (SimpUtil::isRational(node->child[i]) && node->child[i]->value.num.isInf() && node->child[i]->value.num.isNeg()) {
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(Intg("-inf")));
            }
        }

        bool allRational = true;
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (!SimpUtil::isRational(node->child[i])) {
                allRational = false;
                break;
            }
        }

        if (allRational) {
            Rational minVal = node->child[0]->value;
            for (size_t i = 1; i < node->child.size(); ++i) {
                if (node->child[i]->value < minVal) {
                    minVal = node->child[i]->value;
                }
            }
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(minVal);
        }

        Rational minRational;
        bool hasRational = false;
        std::vector<Exptree*> nonRational;
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (SimpUtil::isRational(node->child[i])) {
                if (!hasRational || node->child[i]->value < minRational) {
                    minRational = node->child[i]->value;
                    hasRational = true;
                }
            } else {
                bool dup = false;
                for (size_t j = 0; j < nonRational.size(); ++j) {
                    if (SimpUtil::compareNodes(node->child[i], nonRational[j]) == 0) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) nonRational.push_back(node->child[i]);
            }
        }

        if (nonRational.empty()) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(minRational);
        }

        Exptree* result = SimpUtil::makeFunction(FuncName::min);
        if (hasRational) {
            result->child.push_back(SimpUtil::makeRational(minRational));
        }
        for (size_t i = 0; i < nonRational.size(); ++i) {
            result->child.push_back(SimpUtil::deepCopy(nonRational[i]));
        }

        if (result->child.size() == 1) {
            Exptree* single = result->child[0];
            result->child.clear();
            SimpUtil::freeTree(result);
            SimpUtil::freeTree(node);
            return single;
        }

        SimpUtil::freeTree(node);
        return result;
    }

    // ========== Permut ==========

    Exptree* TreeSimplifier::simplifyPermut(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* n = node->child[0];
        Exptree* k = node->child[1];

        // P(n, 0) = 1
        if (SimpUtil::isZero(k)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // P(n, 1) = n
        if (SimpUtil::isOne(k)) {
            Exptree* result = SimpUtil::deepCopy(n);
            SimpUtil::freeTree(node);
            return result;
        }

        // Compute for small non-negative integers
        if (SimpUtil::isInteger(n) && SimpUtil::isInteger(k) &&
            SimpUtil::isPositive(n) && !SimpUtil::isNegative(k)) {
            Intg nVal = n->value.numerator();
            Intg kVal = k->value.numerator();

            if (kVal > nVal) {
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(Intg(0)));
            }

            // P(n,k) = n*(n-1)*...*(n-k+1)
            Intg result(1);
            for (Intg i(0); i < kVal; i = i + Intg(1)) {
                result = result * (nVal - i);
            }
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        return node;
    }

    // ========== Combin ==========

    Exptree* TreeSimplifier::simplifyCombin(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* n = node->child[0];
        Exptree* k = node->child[1];

        // C(n, 0) = 1
        if (SimpUtil::isZero(k)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // C(n, 1) = n
        if (SimpUtil::isOne(k)) {
            Exptree* result = SimpUtil::deepCopy(n);
            SimpUtil::freeTree(node);
            return result;
        }

        // C(n, n) = 1
        if (SimpUtil::isInteger(n) && SimpUtil::isInteger(k)) {
            if (n->value == k->value) {
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(Intg(1)));
            }
        }

        // Compute for non-negative integers
        if (SimpUtil::isInteger(n) && SimpUtil::isInteger(k) &&
            SimpUtil::isPositive(n) && !SimpUtil::isNegative(k)) {
            Intg nVal = n->value.numerator();
            Intg kVal = k->value.numerator();

            if (kVal > nVal) {
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(Intg(0)));
            }

            // Use smaller k for efficiency
            if (kVal > nVal - kVal) {
                kVal = nVal - kVal;
            }

            Intg result(1);
            for (Intg i(0); i < kVal; i = i + Intg(1)) {
                result = result * (nVal - i);
                result = result / (i + Intg(1));
            }
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(result));
        }

        return node;
    }
    
    // ======== Factorial =========
    
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

        // n! for integers
        if (SimpUtil::isInteger(arg)) {
            Intg n = arg->value.numerator();

            if (n > Intg(0)) {
                Intg result(1);
                for (Intg i(2); i <= n; i = i + Intg(1)) {
                    result = result * i;
                }
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(result));
            }

            // Negative integer: Gamma pole -> NaN
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg("NaN")));
        }

        // Non-integer: x! = Gamma(x+1) = integral from 0 to inf of t^x * e^(-t) dt
        // Build: defint(t, 0, inf, *(^(t, x), ^(e, *(-1, t))))
        Exptree* t = SimpUtil::makeVariable("t");

        // x + 1
        Exptree* xPlus1 = SimpUtil::makeFunction("+");
        xPlus1->child.push_back(SimpUtil::deepCopy(arg));
        xPlus1->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
        xPlus1 = simplifyAdd(xPlus1);

        // t^x
        Exptree* tPow = SimpUtil::makeFunction("^");
        tPow->child.push_back(SimpUtil::deepCopy(t));
        tPow->child.push_back(xPlus1);

        // -t
        Exptree* negT = SimpUtil::makeFunction("*");
        negT->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
        negT->child.push_back(SimpUtil::deepCopy(t));

        // e^(-t)
        Exptree* eNegT = SimpUtil::makeFunction("^");
        eNegT->child.push_back(SimpUtil::makeVariable(ConstName::e));
        eNegT->child.push_back(negT);

        // t^x * e^(-t)
        Exptree* integrand = SimpUtil::makeFunction("*");
        integrand->child.push_back(tPow);
        integrand->child.push_back(eNegT);

        // defint(t, 0, inf, integrand)
        Exptree* result = SimpUtil::makeFunction(FuncName::defint);
        result->child.push_back(SimpUtil::deepCopy(t));
        result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
        result->child.push_back(SimpUtil::makeRational(Rational(Intg("inf"))));
        result->child.push_back(integrand);

        SimpUtil::freeTree(node);
        return result;
    }
} // namespace CAS