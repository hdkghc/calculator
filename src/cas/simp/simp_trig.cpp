/** @file /src/cas/simp/simp_trig.cpp
 *  @brief Trigonometric function simplifiers
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

    // ========== Special angle values ==========

    Exptree* TreeSimplifier::simplifyTrigSpecialAngles(Exptree* node, const char* funcName) {
        if (node->child.size() != 1) return nullptr;
        Exptree* arg = node->child[0];

        // Handle zero
        if (SimpUtil::isZero(arg)) {
            if (std::strcmp(funcName, "sin") == 0 || std::strcmp(funcName, "tan") == 0)
                return SimpUtil::makeRational(Rational(Intg(0)));
            if (std::strcmp(funcName, "cos") == 0)
                return SimpUtil::makeRational(Rational(Intg(1)));
            return nullptr;
        }

        // Handle pi
        if (SimpUtil::isConstantPi(arg)) {
            if (std::strcmp(funcName, "sin") == 0 || std::strcmp(funcName, "tan") == 0)
                return SimpUtil::makeRational(Rational(Intg(0)));
            if (std::strcmp(funcName, "cos") == 0)
                return SimpUtil::makeRational(Rational(Intg(-1)));
            return nullptr;
        }

        // Handle k*pi patterns
        if (SimpUtil::isFunction(arg, "*")) {
            bool hasPi = false;
            bool hasCoeff = false;
            Rational coeff(Intg(1));

            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isConstantPi(arg->child[i])) {
                    hasPi = true;
                } else if (SimpUtil::isRational(arg->child[i])) {
                    if (hasCoeff) {
                        coeff = coeff * arg->child[i]->value;
                    } else {
                        coeff = arg->child[i]->value;
                        hasCoeff = true;
                    }
                }
            }

            if (hasPi) {
                bool isSin = (std::strcmp(funcName, "sin") == 0);
                bool isCos = (std::strcmp(funcName, "cos") == 0);
                bool isTan = (std::strcmp(funcName, "tan") == 0);

                // Handle negative coefficient using odd/even properties
                bool negateResult = false;
                if (coeff < Rational(Intg(0))) {
                    if (isCos) {
                        // cos(-x) = cos(x)
                        coeff = Rational(Intg(0)) - coeff;
                    } else if (isSin || isTan) {
                        // sin(-x) = -sin(x), tan(-x) = -tan(x)
                        negateResult = true;
                        coeff = Rational(Intg(0)) - coeff;
                    }
                }

                if (coeff > Rational(Intg(0))) {
                    // Normalize coefficient
                    Intg k = coeff.numerator();
                    Intg d = coeff.den;

                    if (isTan) {
                        k = k % d;
                        if (k < Intg(0)) k = k + d;
                        coeff = Rational(k, d);
                    } else {
                        Intg twoD = Intg(2) * d;
                        k = k % twoD;
                        if (k < Intg(0)) k = k + twoD;
                        coeff = Rational(k, d);
                    }

                    if (coeff.isZero()) {
                        if (isSin || isTan) {
                            Exptree* r = SimpUtil::makeRational(Rational(Intg(0)));
                            if (negateResult) {
                                Exptree* neg = SimpUtil::makeFunction("*");
                                neg->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                                neg->child.push_back(r);
                                return neg;
                            }
                            return r;
                        }
                        if (isCos) {
                            Exptree* r = SimpUtil::makeRational(Rational(Intg(1)));
                            if (negateResult) {
                                Exptree* neg = SimpUtil::makeFunction("*");
                                neg->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                                neg->child.push_back(r);
                                return neg;
                            }
                            return r;
                        }
                    }

                    bool negate = false;

                    if (isTan) {
                        if (coeff > Rational(Intg(1), Intg(2))) {
                            negate = true;
                            coeff = Rational(Intg(1)) - coeff;
                        }
                    } else {
                        if (coeff > Rational(Intg(1))) {
                            if (isSin || isCos) negate = true;
                            coeff = coeff - Rational(Intg(1));
                        }
                        if (coeff > Rational(Intg(1), Intg(2))) {
                            if (isCos) negate = !negate;
                            coeff = Rational(Intg(1)) - coeff;
                        }
                    }

                    // Combine negateResult with negate
                    if (negateResult) negate = !negate;

                    // Now coeff is in [0, 1/2]
                    Exptree* result = nullptr;

                    if (coeff.isZero()) {
                        if (isSin || isTan) result = SimpUtil::makeRational(Rational(Intg(0)));
                        if (isCos) result = SimpUtil::makeRational(Rational(Intg(1)));
                    }
                    else if (coeff == Rational(Intg(1), Intg(2))) {
                        if (isSin) result = SimpUtil::makeRational(Rational(Intg(1)));
                        if (isCos) result = SimpUtil::makeRational(Rational(Intg(0)));
                    }
                    else if (coeff == Rational(Intg(1), Intg(3))) {
                        if (isSin) {
                            Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                            sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                            result = SimpUtil::makeFunction("/");
                            result->child.push_back(sqrt3);
                            result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                        }
                        if (isCos) result = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
                        if (isTan) {
                            Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                            sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                            result = sqrt3;
                        }
                    }
                    else if (coeff == Rational(Intg(1), Intg(4))) {
                        if (isSin || isCos) {
                            Exptree* sqrt2 = SimpUtil::makeFunction(FuncName::sqrt);
                            sqrt2->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                            result = SimpUtil::makeFunction("/");
                            result->child.push_back(sqrt2);
                            result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                        }
                        if (isTan) result = SimpUtil::makeRational(Rational(Intg(1)));
                    }
                    else if (coeff == Rational(Intg(1), Intg(6))) {
                        if (isSin) result = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
                        if (isCos) {
                            Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                            sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                            result = SimpUtil::makeFunction("/");
                            result->child.push_back(sqrt3);
                            result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                        }
                        if (isTan) {
                            Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                            sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                            result = SimpUtil::makeFunction("/");
                            result->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                            result->child.push_back(sqrt3);
                        }
                    }
                    else if (coeff == Rational(Intg(1), Intg(5))) {
                        if (isCos) {
                            Exptree* sqrt5 = SimpUtil::makeFunction(FuncName::sqrt);
                            sqrt5->child.push_back(SimpUtil::makeRational(Rational(Intg(5))));
                            Exptree* one = SimpUtil::makeRational(Rational(Intg(1)));
                            Exptree* sum = SimpUtil::makeFunction("+");
                            sum->child.push_back(sqrt5);
                            sum->child.push_back(one);
                            result = SimpUtil::makeFunction("/");
                            result->child.push_back(sum);
                            result->child.push_back(SimpUtil::makeRational(Rational(Intg(4))));
                        }
                    }
                    else if (coeff == Rational(Intg(1), Intg(10))) {
                        if (isSin) {
                            Exptree* sqrt5 = SimpUtil::makeFunction(FuncName::sqrt);
                            sqrt5->child.push_back(SimpUtil::makeRational(Rational(Intg(5))));
                            Exptree* negOne = SimpUtil::makeFunction("*");
                            negOne->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                            negOne->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                            Exptree* sum = SimpUtil::makeFunction("+");
                            sum->child.push_back(sqrt5);
                            sum->child.push_back(negOne);
                            result = SimpUtil::makeFunction("/");
                            result->child.push_back(sum);
                            result->child.push_back(SimpUtil::makeRational(Rational(Intg(4))));
                        }
                    }

                    // Apply negation
                    if (result && negate) {
                        Exptree* negResult = SimpUtil::makeFunction("*");
                        negResult->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                        negResult->child.push_back(result);
                        return negResult;
                    }

                    return result;
                }
            }
        }

        return nullptr;
    }

    // ========== Helper: detect i*x pattern ==========

    /**
     *  @brief Check if expression is i * something, extract the something
     *  @param node Node to check
     *  @param inner Output: the non-i part if pattern matches
     *  @return true if node is i * inner
     */
    static bool extractImaginaryArg(Exptree* node, Exptree*& inner) {
        if (SimpUtil::isConstantI(node)) {
            inner = SimpUtil::makeRational(Rational(Intg(1)));
            return true;
        }
        if (SimpUtil::isFunction(node, "*")) {
            bool hasI = false;
            Exptree* nonI = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < node->child.size(); ++i) {
                if (SimpUtil::isConstantI(node->child[i])) {
                    hasI = true;
                } else {
                    nonI->child.push_back(SimpUtil::deepCopy(node->child[i]));
                }
            }
            if (hasI) {
                if (nonI->child.size() == 1) {
                    inner = nonI->child[0];
                    nonI->child.clear();
                    SimpUtil::freeTree(nonI);
                } else if (nonI->child.size() == 0) {
                    SimpUtil::freeTree(nonI);
                    inner = SimpUtil::makeRational(Rational(Intg(1)));
                } else {
                    inner = nonI;
                }
                return true;
            }
            SimpUtil::freeTree(nonI);
        }
        return false;
    }

    // ========== Sin ==========

    Exptree* TreeSimplifier::simplifySin(Exptree* node) {
        Exptree* special = simplifyTrigSpecialAngles(node, "sin");
        if (special) {
            SimpUtil::freeTree(node);
            return special;
        }

        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // sin(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // sin(i*x) = i*sinh(x)
        // The sinh node will be further simplified because simplifyNode
        // calls simplifySinh which calls preTransform + simplifyNode
        Exptree* inner = nullptr;
        if (extractImaginaryArg(arg, inner)) {
            inner = simplifyNode(inner);
            Exptree* sinhNode = SimpUtil::makeFunction(FuncName::sinh);
            sinhNode->child.push_back(inner);
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(SimpUtil::makeVariable(ConstName::i));
            result->child.push_back(sinhNode);
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // sin(-x) = -sin(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner2 = nullptr;
                if (arg->child.size() == 2) {
                    inner2 = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner2 = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner2->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }
                Exptree* innerSin = SimpUtil::makeFunction(FuncName::sin);
                innerSin->child.push_back(inner2);
                innerSin = simplifySin(innerSin);
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                result->child.push_back(innerSin);
                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        return node;
    }

    // ========== Cos ==========

    Exptree* TreeSimplifier::simplifyCos(Exptree* node) {
        Exptree* special = simplifyTrigSpecialAngles(node, "cos");
        if (special) {
            SimpUtil::freeTree(node);
            return special;
        }

        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // cos(i*x) = cosh(x)
        // The cosh node will be further simplified because simplifyNode
        // calls simplifyCosh which calls preTransform + simplifyNode
        Exptree* inner = nullptr;
        if (extractImaginaryArg(arg, inner)) {
            inner = simplifyNode(inner);
            Exptree* coshNode = SimpUtil::makeFunction(FuncName::cosh);
            coshNode->child.push_back(inner);
            // Simplify the cosh node to trigger exponential conversion
            SimpUtil::freeTree(node);
            return coshNode;
        }

        // cos(-x) = cos(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner2 = nullptr;
                if (arg->child.size() == 2) {
                    inner2 = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner2 = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner2->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }
                Exptree* newCos = SimpUtil::makeFunction(FuncName::cos);
                newCos->child.push_back(inner2);
                SimpUtil::freeTree(node);
                return simplifyCos(newCos);
            }
        }

        return node;
    }

    // ========== Tan ==========

    Exptree* TreeSimplifier::simplifyTan(Exptree* node) {
        Exptree* special = simplifyTrigSpecialAngles(node, "tan");
        if (special) {
            SimpUtil::freeTree(node);
            return special;
        }

        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // tan(i*x) = i*tanh(x)
        // The tanh node will be further simplified because simplifyNode
        // calls simplifyTanh which calls preTransform + simplifyNode
        Exptree* inner = nullptr;
        if (extractImaginaryArg(arg, inner)) {
            inner = simplifyNode(inner);
            Exptree* tanhNode = SimpUtil::makeFunction(FuncName::tanh);
            tanhNode->child.push_back(inner);
            // Simplify the tanh node to trigger exponential conversion
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(SimpUtil::makeVariable(ConstName::i));
            result->child.push_back(tanhNode);
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // tan(-x) = -tan(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner2 = nullptr;
                if (arg->child.size() == 2) {
                    inner2 = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner2 = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner2->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }
                Exptree* innerTan = SimpUtil::makeFunction(FuncName::tan);
                innerTan->child.push_back(inner2);
                innerTan = simplifyTan(innerTan);
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                result->child.push_back(innerTan);
                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        return node;
    }

    // ========== Asin ==========

    Exptree* TreeSimplifier::simplifyAsin(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // asin(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // asin(1) = pi/2
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(half);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // asin(1/2) = pi/6
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1), Intg(2))) {
            SimpUtil::freeTree(node);
            Exptree* sixth = SimpUtil::makeRational(Rational(Intg(1), Intg(6)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(sixth);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // asin(-x) = -asin(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty() && SimpUtil::isMinusOne(arg->child[0])) {
            Exptree* inner;
            if (arg->child.size() == 2) {
                inner = SimpUtil::deepCopy(arg->child[1]);
            } else {
                inner = SimpUtil::makeFunction("*");
                for (size_t i = 1; i < arg->child.size(); ++i) {
                    inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                }
            }
            Exptree* asinInner = SimpUtil::makeFunction(FuncName::asin);
            asinInner->child.push_back(inner);
            asinInner = simplifyAsin(asinInner);
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            result->child.push_back(asinInner);
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        return node;
    }

    // ========== Acos ==========

    Exptree* TreeSimplifier::simplifyAcos(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // acos(0) = pi/2
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(half);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // acos(1) = 0
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // acos(1/2) = pi/3
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1), Intg(2))) {
            SimpUtil::freeTree(node);
            Exptree* third = SimpUtil::makeRational(Rational(Intg(1), Intg(3)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(third);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // acos(-x) = pi - acos(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty() && SimpUtil::isMinusOne(arg->child[0])) {
            Exptree* inner;
            if (arg->child.size() == 2) {
                inner = SimpUtil::deepCopy(arg->child[1]);
            } else {
                inner = SimpUtil::makeFunction("*");
                for (size_t i = 1; i < arg->child.size(); ++i) {
                    inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                }
            }
            Exptree* acosInner = SimpUtil::makeFunction(FuncName::acos);
            acosInner->child.push_back(inner);
            acosInner = simplifyAcos(acosInner);

            // pi - acos(inner)
            Exptree* negAcos = SimpUtil::makeFunction("*");
            negAcos->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negAcos->child.push_back(acosInner);

            Exptree* result = SimpUtil::makeFunction("+");
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            result->child.push_back(negAcos);

            SimpUtil::freeTree(node);
            return simplifyAdd(result);
        }

        return node;
    }

    // ========== Atan ==========

    Exptree* TreeSimplifier::simplifyAtan(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // atan(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // atan(1) = pi/4
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            Exptree* quarter = SimpUtil::makeRational(Rational(Intg(1), Intg(4)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(quarter);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // atan(-1) = -pi/4
        if (SimpUtil::isMinusOne(arg)) {
            SimpUtil::freeTree(node);
            Exptree* negQuarter = SimpUtil::makeRational(Rational(Intg(-1), Intg(4)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(negQuarter);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // atan(-x) = -atan(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty() && SimpUtil::isMinusOne(arg->child[0])) {
            Exptree* inner;
            if (arg->child.size() == 2) {
                inner = SimpUtil::deepCopy(arg->child[1]);
            } else {
                inner = SimpUtil::makeFunction("*");
                for (size_t i = 1; i < arg->child.size(); ++i) {
                    inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                }
            }
            Exptree* atanInner = SimpUtil::makeFunction(FuncName::atan);
            atanInner->child.push_back(inner);
            atanInner = simplifyAtan(atanInner);
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            result->child.push_back(atanInner);
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // atan(sqrt(3)) = pi/3
        if (SimpUtil::isFunction(arg, FuncName::sqrt) && arg->child.size() == 1) {
            if (SimpUtil::isRational(arg->child[0]) && arg->child[0]->value == Rational(Intg(3))) {
                SimpUtil::freeTree(node);
                Exptree* third = SimpUtil::makeRational(Rational(Intg(1), Intg(3)));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(third);
                result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                return result;
            }
        }

        // atan(^(3,1/2)) = pi/3  (sqrt(3) after simplification)
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2 &&
            SimpUtil::isRational(arg->child[0]) && arg->child[0]->value == Rational(Intg(3)) &&
            SimpUtil::isRational(arg->child[1]) && arg->child[1]->value == Rational(Intg(1), Intg(2))) {
            SimpUtil::freeTree(node);
            Exptree* third = SimpUtil::makeRational(Rational(Intg(1), Intg(3)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(third);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // atan(-sqrt(3)) = -pi/3
        if (SimpUtil::isFunction(arg, "*") && arg->child.size() == 2) {
            if (SimpUtil::isMinusOne(arg->child[0]) &&
                SimpUtil::isFunction(arg->child[1], FuncName::sqrt) &&
                arg->child[1]->child.size() == 1 &&
                SimpUtil::isRational(arg->child[1]->child[0]) &&
                arg->child[1]->child[0]->value == Rational(Intg(3))) {
                SimpUtil::freeTree(node);
                Exptree* negThird = SimpUtil::makeRational(Rational(Intg(-1), Intg(3)));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(negThird);
                result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                return result;
            }
        }

        // atan(1/sqrt(3)) = pi/6
        if (SimpUtil::isFunction(arg, "/") && arg->child.size() == 2) {
            if (SimpUtil::isOne(arg->child[0]) &&
                SimpUtil::isFunction(arg->child[1], FuncName::sqrt) &&
                arg->child[1]->child.size() == 1 &&
                SimpUtil::isRational(arg->child[1]->child[0]) &&
                arg->child[1]->child[0]->value == Rational(Intg(3))) {
                SimpUtil::freeTree(node);
                Exptree* sixth = SimpUtil::makeRational(Rational(Intg(1), Intg(6)));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(sixth);
                result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                return result;
            }
        }

        // atan(^(3,-1/2)) = pi/6
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isRational(arg->child[0]) && arg->child[0]->value == Rational(Intg(3)) &&
                SimpUtil::isRational(arg->child[1]) &&
                arg->child[1]->value == Rational(Intg(-1), Intg(2))) {
                SimpUtil::freeTree(node);
                Exptree* sixth = SimpUtil::makeRational(Rational(Intg(1), Intg(6)));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(sixth);
                result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                return result;
            }
        }

        // atan(sqrt(3)/3) = atan(1/sqrt(3)) = pi/6
        if (SimpUtil::isFunction(arg, "*") && arg->child.size() == 2) {
            Exptree* a = arg->child[0];
            Exptree* b = arg->child[1];
            if ((SimpUtil::isRational(a) && a->value == Rational(Intg(1), Intg(3)) &&
                 SimpUtil::isFunction(b, FuncName::sqrt) && b->child.size() == 1 &&
                 SimpUtil::isRational(b->child[0]) && b->child[0]->value == Rational(Intg(3))) ||
                (SimpUtil::isRational(b) && b->value == Rational(Intg(1), Intg(3)) &&
                 SimpUtil::isFunction(a, FuncName::sqrt) && a->child.size() == 1 &&
                 SimpUtil::isRational(a->child[0]) && a->child[0]->value == Rational(Intg(3)))) {
                SimpUtil::freeTree(node);
                Exptree* sixth = SimpUtil::makeRational(Rational(Intg(1), Intg(6)));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(sixth);
                result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                return result;
            }
        }

        return node;
    }

} // namespace CAS