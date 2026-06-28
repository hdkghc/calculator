/** @file /src/cas/simp/simp_complex.cpp
 *  @brief Complex number function simplifiers
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

    // ========== Helper: check if node is a sum with i terms ==========

    /**
     *  @brief Check if a sum contains i (complex) terms
     *  @param simp Reference to TreeSimplifier instance for calling simplifyAdd
     *  @param node Sum node
     *  @param realPart Output: tree of real terms, or nullptr if none
     *  @param imagPart Output: tree of imaginary terms (without i), or nullptr if none
     *  @return true if at least one imaginary term found
     */
    bool TreeSimplifier::splitComplexSum(TreeSimplifier& simp, Exptree* node,
                                 Exptree*& realPart, Exptree*& imagPart) {
        if (!SimpUtil::isFunction(node, "+")) return false;

        Exptree* realSum = nullptr;
        Exptree* imagSum = nullptr;
        bool hasI = false;

        for (size_t i = 0; i < node->child.size(); ++i) {
            Exptree* term = node->child[i];
            bool termHasI = false;
            Exptree* coeff = nullptr;

            if (SimpUtil::isConstantI(term)) {
                termHasI = true;
                coeff = SimpUtil::makeRational(Rational(Intg(1)));
            } else if (SimpUtil::isFunction(term, "*")) {
                Exptree* nonI = SimpUtil::makeFunction("*");
                for (size_t j = 0; j < term->child.size(); ++j) {
                    if (SimpUtil::isConstantI(term->child[j])) {
                        termHasI = true;
                    } else {
                        nonI->child.push_back(SimpUtil::deepCopy(term->child[j]));
                    }
                }
                if (termHasI) {
                    if (nonI->child.size() == 1) {
                        coeff = nonI->child[0];
                        nonI->child.clear();
                        SimpUtil::freeTree(nonI);
                    } else if (nonI->child.size() == 0) {
                        SimpUtil::freeTree(nonI);
                        coeff = SimpUtil::makeRational(Rational(Intg(1)));
                    } else {
                        coeff = nonI;
                    }
                } else {
                    SimpUtil::freeTree(nonI);
                }
            }

            if (termHasI) {
                hasI = true;
                if (imagSum) {
                    Exptree* newSum = SimpUtil::makeFunction("+");
                    newSum->child.push_back(imagSum);
                    newSum->child.push_back(coeff);
                    imagSum = newSum;
                } else {
                    imagSum = SimpUtil::makeFunction("+");
                    imagSum->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
                    imagSum->child.push_back(coeff);
                }
            } else {
                Exptree* termCopy = SimpUtil::deepCopy(term);
                if (realSum) {
                    Exptree* newSum = SimpUtil::makeFunction("+");
                    newSum->child.push_back(realSum);
                    newSum->child.push_back(termCopy);
                    realSum = newSum;
                } else {
                    realSum = SimpUtil::makeFunction("+");
                    realSum->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
                    realSum->child.push_back(termCopy);
                }
            }
        }

        if (!hasI) {
            if (realSum) SimpUtil::freeTree(realSum);
            if (imagSum) SimpUtil::freeTree(imagSum);
            return false;
        }

        realPart = realSum ? simp.simplifyAdd(realSum) : SimpUtil::makeRational(Rational(Intg(0)));
        imagPart = imagSum ? simp.simplifyAdd(imagSum) : SimpUtil::makeRational(Rational(Intg(0)));
        return true;
    }

    // ========== Realpart ==========

    Exptree* TreeSimplifier::simplifyRealpart(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // re(real) = real
        if (SimpUtil::isRational(arg) || SimpUtil::isVariable(arg)) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // re(a + b*i) = a
        Exptree* realPart = nullptr;
        Exptree* imagPart = nullptr;
        if (splitComplexSum(*this, arg, realPart, imagPart)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            if (imagPart) SimpUtil::freeTree(imagPart);
            return realPart;
        }

        // re(i) = 0
        if (SimpUtil::isConstantI(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // re(i * x) = 0 for any real x
        if (SimpUtil::isFunction(arg, "*")) {
            bool hasI = false;
            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isConstantI(arg->child[i])) {
                    hasI = true;
                    break;
                }
            }
            if (hasI) {
                // If all factors are i, or it's simply i * something
                // For pure imaginary: re(i*x) = 0
                if (arg->child.size() == 2) {
                    SimpUtil::freeTree(node);
                    SimpUtil::freeTree(arg);
                    return SimpUtil::makeRational(Rational(Intg(0)));
                }
                // Multiple factors: if i is present, entire thing is imaginary
                bool allHaveI = true;
                for (size_t i = 0; i < arg->child.size() && allHaveI; ++i) {
                    if (!SimpUtil::isConstantI(arg->child[i])) {
                        allHaveI = false;
                    }
                }
                if (allHaveI) {
                    SimpUtil::freeTree(node);
                    SimpUtil::freeTree(arg);
                    return SimpUtil::makeRational(Rational(Intg(0)));
                }
                // Conservative: assume any i factor makes it imaginary
                SimpUtil::freeTree(node);
                SimpUtil::freeTree(arg);
                return SimpUtil::makeRational(Rational(Intg(0)));
            }
        }

        return node;
    }

    // ========== Imagpart ==========

    Exptree* TreeSimplifier::simplifyImagpart(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // im(real) = 0
        if (SimpUtil::isRational(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // im(variable) = 0 (unless it's i itself)
        if (SimpUtil::isVariable(arg) && !SimpUtil::isConstantI(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // im(i) = 1
        if (SimpUtil::isConstantI(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // im(a + b*i) = b
        Exptree* realPart = nullptr;
        Exptree* imagPart = nullptr;
        if (splitComplexSum(*this, arg, realPart, imagPart)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            if (realPart) SimpUtil::freeTree(realPart);
            return imagPart;
        }

        // im(c * i) = c
        if (SimpUtil::isFunction(arg, "*")) {
            bool hasI = false;
            Exptree* coeff = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isConstantI(arg->child[i])) {
                    hasI = true;
                } else {
                    coeff->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                }
            }
            if (hasI) {
                SimpUtil::freeTree(node);
                SimpUtil::freeTree(arg);
                if (coeff->child.size() == 1) {
                    Exptree* result = coeff->child[0];
                    coeff->child.clear();
                    SimpUtil::freeTree(coeff);
                    return result;
                } else if (coeff->child.size() == 0) {
                    SimpUtil::freeTree(coeff);
                    return SimpUtil::makeRational(Rational(Intg(1)));
                }
                return simplifyMul(coeff);
            }
            SimpUtil::freeTree(coeff);
        }

        return node;
    }

    // ========== Conjg ==========

    Exptree* TreeSimplifier::simplifyConjg(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // conj(real) = real
        if (SimpUtil::isRational(arg) || (SimpUtil::isVariable(arg) && !SimpUtil::isConstantI(arg))) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // conj(i) = -i
        if (SimpUtil::isConstantI(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            result->child.push_back(SimpUtil::makeVariable(ConstName::i));
            return result;
        }

        // conj(a + b*i) = a - b*i
        Exptree* realPart = nullptr;
        Exptree* imagPart = nullptr;
        if (splitComplexSum(*this, arg, realPart, imagPart)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);

            // Build: realPart + (-1)*imagPart*i
            Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
            Exptree* negImag = SimpUtil::makeFunction("*");
            negImag->child.push_back(negOne);
            negImag->child.push_back(imagPart);
            negImag = simplifyMul(negImag);

            Exptree* negImagI = SimpUtil::makeFunction("*");
            negImagI->child.push_back(negImag);
            negImagI->child.push_back(SimpUtil::makeVariable(ConstName::i));

            Exptree* result = SimpUtil::makeFunction("+");
            result->child.push_back(realPart);
            result->child.push_back(negImagI);

            return simplifyAdd(result);
        }

        // conj(c * i) = -c * i = (-1)*c*i
        if (SimpUtil::isFunction(arg, "*")) {
            bool hasI = false;
            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isConstantI(arg->child[i])) {
                    hasI = true;
                    break;
                }
            }
            if (hasI) {
                SimpUtil::freeTree(node);
                Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(negOne);
                result->child.push_back(arg);
                return simplifyMul(result);
            }
        }

        return node;
    }

    // ========== Arg ==========

    Exptree* TreeSimplifier::simplifyArg(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // arg(positive real) = 0
        if (SimpUtil::isPositive(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // arg(negative real) = pi
        if (SimpUtil::isNegative(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            return SimpUtil::makeVariable(ConstName::pi);
        }

        // arg(0) = undefined, leave as-is
        if (SimpUtil::isZero(arg)) {
            return node;
        }

        // arg(i) = pi/2
        if (SimpUtil::isConstantI(arg)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);
            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(half);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }

        // arg(-i) = -pi/2
        if (SimpUtil::isFunction(arg, "*") && arg->child.size() == 2) {
            if (SimpUtil::isMinusOne(arg->child[0]) && SimpUtil::isConstantI(arg->child[1])) {
                SimpUtil::freeTree(node);
                SimpUtil::freeTree(arg);
                Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
                Exptree* negPi = SimpUtil::makeFunction("*");
                negPi->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                negPi->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(half);
                result->child.push_back(negPi);
                return simplifyMul(result);
            }
        }

        // arg(a + b*i) = atan(b/a)
        Exptree* realPart = nullptr;
        Exptree* imagPart = nullptr;
        if (splitComplexSum(*this, arg, realPart, imagPart)) {
            SimpUtil::freeTree(node);
            SimpUtil::freeTree(arg);

            Exptree* div = SimpUtil::makeFunction("/");
            div->child.push_back(imagPart);
            div->child.push_back(realPart);

            Exptree* atanNode = SimpUtil::makeFunction(FuncName::atan);
            atanNode->child.push_back(div);

            return simplifyAtan(atanNode);
        }

        return node;
    }

} // namespace CAS