/** @file /src/cas/simp/simp_arith.cpp
 *  @brief Arithmetic operation simplifiers (+, *, ^)
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

    // ========== Addition ==========

    void TreeSimplifier::collectAddTerms(Exptree* node, WorkBuffer& buf) {
        if (!node) return;

        if (SimpUtil::isRational(node)) {
            buf.constantAccum = buf.constantAccum + node->value;
            return;
        }

        // Recursively flatten nested addition
        if (SimpUtil::isFunction(node, "+")) {
            for (size_t i = 0; i < node->child.size(); ++i) {
                collectAddTerms(node->child[i], buf);
            }
            return;
        }

        buf.addItem(SimpUtil::deepCopy(node));
    }

    void TreeSimplifier::mergeAddTerms(WorkBuffer& buf) {
        for (size_t i = 0; i < buf.count; ++i) {
            if (!buf.items[i]) continue;

            Exptree* term_i = buf.items[i];
            Rational coeff_i(Intg(1));
            Exptree* base_i = term_i;

            // Extract coefficient from c * rest
            if (SimpUtil::isFunction(term_i, "*")) {
                if (!term_i->child.empty() && SimpUtil::isRational(term_i->child[0])) {
                    coeff_i = term_i->child[0]->value;
                    if (term_i->child.size() == 2) {
                        base_i = term_i->child[1];
                    } else {
                        Exptree* newMul = SimpUtil::makeFunction("*");
                        for (size_t j = 1; j < term_i->child.size(); ++j) {
                            newMul->child.push_back(SimpUtil::deepCopy(term_i->child[j]));
                        }
                        base_i = newMul;
                    }
                }
            }

            // Search for matching terms
            for (size_t j = i + 1; j < buf.count; ++j) {
                if (!buf.items[j]) continue;

                Exptree* term_j = buf.items[j];
                Rational coeff_j(Intg(1));
                Exptree* base_j = term_j;

                if (SimpUtil::isFunction(term_j, "*")) {
                    if (!term_j->child.empty() && SimpUtil::isRational(term_j->child[0])) {
                        coeff_j = term_j->child[0]->value;
                        if (term_j->child.size() == 2) {
                            base_j = term_j->child[1];
                        } else {
                            Exptree* newMul = SimpUtil::makeFunction("*");
                            for (size_t k = 1; k < term_j->child.size(); ++k) {
                                newMul->child.push_back(SimpUtil::deepCopy(term_j->child[k]));
                            }
                            base_j = newMul;
                        }
                    }
                }

                if (SimpUtil::compareNodes(base_i, base_j) == 0) {
                    coeff_i = coeff_i + coeff_j;
                    SimpUtil::freeTree(buf.items[j]);
                    buf.items[j] = nullptr;
                    if (base_j != term_j->child[1] && base_j != term_j) {
                        SimpUtil::freeTree(base_j);
                    }
                } else {
                    if (base_j != term_j->child[1] && base_j != term_j) {
                        SimpUtil::freeTree(base_j);
                    }
                }
            }

            // Rebuild term with merged coefficient
            if (coeff_i.isZero()) {
                SimpUtil::freeTree(buf.items[i]);
                buf.items[i] = nullptr;
                if (base_i != term_i->child[1] && base_i != term_i) {
                    SimpUtil::freeTree(base_i);
                }
            } else if (coeff_i == Rational(Intg(1))) {
                if (base_i != term_i->child[1]) {
                    SimpUtil::freeTree(buf.items[i]);
                    buf.items[i] = base_i;
                }
            } else {
                Exptree* newTerm = SimpUtil::makeFunction("*");
                newTerm->child.push_back(SimpUtil::makeRational(coeff_i));
                if (base_i != term_i->child[1]) {
                    newTerm->child.push_back(base_i);
                } else {
                    newTerm->child.push_back(SimpUtil::deepCopy(base_i));
                }
                SimpUtil::freeTree(buf.items[i]);
                buf.items[i] = newTerm;
            }
        }

        // Compact array (remove null entries)
        size_t writeIdx = 0;
        for (size_t i = 0; i < buf.count; ++i) {
            if (buf.items[i]) {
                if (writeIdx != i) {
                    buf.items[writeIdx] = buf.items[i];
                }
                writeIdx++;
            }
        }
        buf.count = writeIdx;
    }

    Exptree* TreeSimplifier::rebuildAdd(WorkBuffer& buf) {
        if (!buf.constantAccum.isZero()) {
            if (!buf.addItem(SimpUtil::makeRational(buf.constantAccum))) {
                return SimpUtil::makeRational(buf.constantAccum);
            }
        }

        if (buf.count == 0) {
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        if (buf.count == 1) {
            return buf.items[0];
        }

        sortItems(buf.items, buf.count);

        Exptree* result = SimpUtil::makeFunction("+");
        for (size_t i = 0; i < buf.count; ++i) {
            result->child.push_back(buf.items[i]);
        }

        return result;
    }

    Exptree* TreeSimplifier::simplifyAdd(Exptree* node) {
        addBuf_.resetAdd();
        collectAddTerms(node, addBuf_);
        mergeAddTerms(addBuf_);
        Exptree* result = rebuildAdd(addBuf_);

        if (result != node) {
            SimpUtil::freeTree(node);
        }

        return result;
    }

    // ========== Multiplication ==========

    void TreeSimplifier::collectMulFactors(Exptree* node, WorkBuffer& buf) {
        if (!node) return;

        if (SimpUtil::isRational(node)) {
            buf.constantAccum = buf.constantAccum * node->value;
            return;
        }

        if (SimpUtil::isFunction(node, "*")) {
            for (size_t i = 0; i < node->child.size(); ++i) {
                collectMulFactors(node->child[i], buf);
            }
            return;
        }

        buf.addItem(SimpUtil::deepCopy(node));
    }

    void TreeSimplifier::mergeMulFactors(WorkBuffer& buf) {
        for (size_t i = 0; i < buf.count; ++i) {
            if (!buf.items[i]) continue;

            Exptree* factor_i = buf.items[i];
            Exptree* base_i = factor_i;
            Exptree* exp_i = nullptr;

            if (SimpUtil::isFunction(factor_i, "^") && factor_i->child.size() == 2) {
                base_i = factor_i->child[0];
                exp_i = factor_i->child[1];
            }

            for (size_t j = i + 1; j < buf.count; ++j) {
                if (!buf.items[j]) continue;

                Exptree* factor_j = buf.items[j];
                Exptree* base_j = factor_j;
                Exptree* exp_j = nullptr;

                if (SimpUtil::isFunction(factor_j, "^") && factor_j->child.size() == 2) {
                    base_j = factor_j->child[0];
                    exp_j = factor_j->child[1];
                }

                if (SimpUtil::compareNodes(base_i, base_j) == 0) {
                    if (!exp_i) exp_i = SimpUtil::makeRational(Rational(Intg(1)));
                    if (!exp_j) exp_j = SimpUtil::makeRational(Rational(Intg(1)));

                    Exptree* sumExp = SimpUtil::makeFunction("+");
                    sumExp->child.push_back(SimpUtil::deepCopy(exp_i));
                    sumExp->child.push_back(SimpUtil::deepCopy(exp_j));
                    sumExp = simplifyAdd(sumExp);

                    if (SimpUtil::isZero(sumExp)) {
                        SimpUtil::freeTree(buf.items[i]);
                        SimpUtil::freeTree(buf.items[j]);
                        SimpUtil::freeTree(sumExp);
                        buf.items[i] = SimpUtil::makeRational(Rational(Intg(1)));
                        buf.items[j] = nullptr;
                    } else {
                        Exptree* newPow = SimpUtil::makeFunction("^");
                        newPow->child.push_back(SimpUtil::deepCopy(base_i));
                        newPow->child.push_back(sumExp);
                        SimpUtil::freeTree(buf.items[i]);
                        SimpUtil::freeTree(buf.items[j]);
                        buf.items[i] = newPow;
                        buf.items[j] = nullptr;
                    }
                    break;
                }
            }
        }

        // Compact and remove factors of 1
        size_t writeIdx = 0;
        for (size_t i = 0; i < buf.count; ++i) {
            if (buf.items[i]) {
                if (SimpUtil::isOne(buf.items[i])) {
                    SimpUtil::freeTree(buf.items[i]);
                    continue;
                }
                if (writeIdx != i) {
                    buf.items[writeIdx] = buf.items[i];
                }
                writeIdx++;
            }
        }
        buf.count = writeIdx;
    }

    Exptree* TreeSimplifier::rebuildMul(WorkBuffer& buf) {
        if (buf.constantAccum != Rational(Intg(1))) {
            if (buf.constantAccum.isZero()) {
                for (size_t i = 0; i < buf.count; ++i) {
                    SimpUtil::freeTree(buf.items[i]);
                }
                return SimpUtil::makeRational(Rational(Intg(0)));
            }
            // Insert constant at front
            for (size_t i = buf.count; i > 0; --i) {
                buf.items[i] = buf.items[i - 1];
            }
            buf.items[0] = SimpUtil::makeRational(buf.constantAccum);
            buf.count++;
        }

        if (buf.count == 0) {
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        if (buf.count == 1) {
            return buf.items[0];
        }

        sortItems(buf.items, buf.count);

        Exptree* result = SimpUtil::makeFunction("*");
        for (size_t i = 0; i < buf.count; ++i) {
            result->child.push_back(buf.items[i]);
        }

        return result;
    }

    Exptree* TreeSimplifier::simplifyMul(Exptree* node) {
        mulBuf_.resetMul();
        collectMulFactors(node, mulBuf_);
        mergeMulFactors(mulBuf_);
        Exptree* result = rebuildMul(mulBuf_);

        if (result != node) {
            SimpUtil::freeTree(node);
        }

        return result;
    }

    // ========== Power ==========

    Exptree* TreeSimplifier::foldRationalPower(Exptree* base, Exptree* exp) {
        if (!SimpUtil::isRational(base) || !SimpUtil::isRational(exp)) {
            return nullptr;
        }

        if (exp->value.isInteger()) {
            Intg expNum = exp->value.numerator();
            if (expNum < Intg(0)) {
                // Negative exponent: result is rarely integer, skip folding
                return nullptr;
            }
            // base^exp for integer exponent
            Intg baseNum = base->value.numerator();
            Intg baseDen = base->value.den;
            Intg resultNum = baseNum.pow(expNum);
            Intg resultDen = baseDen.pow(expNum);
            return SimpUtil::makeRational(Rational(resultNum, resultDen));
        }

        return nullptr;
    }

    bool TreeSimplifier::isPerfectSquare(Rational r, Rational& root) {
        if (r < Rational(Intg(0))) return false;

        Intg num = r.numerator();
        Intg den = r.den;

        Intg sqrtNum = num.sqrt(num);
        Intg sqrtDen = den.sqrt(den);

        // Verify: sqrtNum^2 == num
        if (sqrtNum * sqrtNum != num) return false;
        if (sqrtDen * sqrtDen != den) return false;

        root = Rational(sqrtNum, sqrtDen);
        return true;
    }

    Exptree* TreeSimplifier::simplifyEulerForm(Exptree* expArg) {
        if (!SimpUtil::isFunction(expArg, "*")) return nullptr;

        bool hasI = false;
        Exptree* thetaPart = nullptr;

        for (size_t i = 0; i < expArg->child.size(); ++i) {
            if (SimpUtil::isConstantI(expArg->child[i])) {
                hasI = true;
            } else {
                if (!thetaPart) {
                    thetaPart = expArg->child[i];
                } else {
                    return nullptr; // Too many non-i factors
                }
            }
        }

        if (hasI && thetaPart) {
            // e^(i*theta) = cos(theta) + i*sin(theta)
            Exptree* cosTerm = SimpUtil::makeFunction(FuncName::cos);
            cosTerm->child.push_back(SimpUtil::deepCopy(thetaPart));

            Exptree* sinTerm = SimpUtil::makeFunction(FuncName::sin);
            sinTerm->child.push_back(SimpUtil::deepCopy(thetaPart));

            Exptree* iSin = SimpUtil::makeFunction("*");
            iSin->child.push_back(SimpUtil::makeVariable(ConstName::i));
            iSin->child.push_back(sinTerm);

            Exptree* result = SimpUtil::makeFunction("+");
            result->child.push_back(cosTerm);
            result->child.push_back(iSin);

            return simplifyAdd(result);
        }

        return nullptr;
    }

    Exptree* TreeSimplifier::handleComplexSqrt(Exptree* node) {
        if (!SimpUtil::isFunction(node, "^") || node->child.size() != 2) return nullptr;
        if (!SimpUtil::isRational(node->child[1])) return nullptr;
        if (node->child[1]->value != Rational(Intg(1), Intg(2))) return nullptr;

        Exptree* base = node->child[0];

        // sqrt(-1) = i
        if (SimpUtil::isMinusOne(base)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeVariable(ConstName::i);
        }

        // sqrt(-a) = i*sqrt(a) where a > 0
        if (SimpUtil::isFunction(base, "*")) {
            for (size_t i = 0; i < base->child.size(); ++i) {
                if (SimpUtil::isMinusOne(base->child[i])) {
                    Exptree* remaining = SimpUtil::makeFunction("*");
                    for (size_t j = 0; j < base->child.size(); ++j) {
                        if (j != i) {
                            remaining->child.push_back(SimpUtil::deepCopy(base->child[j]));
                        }
                    }
                    remaining = simplifyMul(remaining);

                    Exptree* sqrtRem = SimpUtil::makeFunction("^");
                    sqrtRem->child.push_back(remaining);
                    sqrtRem->child.push_back(SimpUtil::makeRational(Rational(Intg(1), Intg(2))));

                    Exptree* result = SimpUtil::makeFunction("*");
                    result->child.push_back(SimpUtil::makeVariable(ConstName::i));
                    result->child.push_back(sqrtRem);

                    SimpUtil::freeTree(node);
                    return simplifyMul(result);
                }
            }
        }

        return nullptr;
    }

    Exptree* TreeSimplifier::simplifyPow(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* base = node->child[0];
        Exptree* exp = node->child[1];

        // 0^0 -> undefined, leave as-is
        if (SimpUtil::isZero(base) && SimpUtil::isZero(exp)) {
            return node;
        }

        // 0^x = 0 (x != 0)
        if (SimpUtil::isZero(base)) {
            SimpUtil::freeTree(exp);
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // x^0 = 1
        if (SimpUtil::isZero(exp)) {
            SimpUtil::freeTree(base);
            SimpUtil::freeTree(exp);
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // x^1 = x
        if (SimpUtil::isOne(exp)) {
            SimpUtil::freeTree(exp);
            SimpUtil::freeTree(node);
            return base;
        }

        // 1^x = 1
        if (SimpUtil::isOne(base)) {
            SimpUtil::freeTree(base);
            SimpUtil::freeTree(exp);
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // Constant folding for rational base and exponent
        Exptree* folded = foldRationalPower(base, exp);
        if (folded) {
            SimpUtil::freeTree(node);
            return folded;
        }

        // (-1)^integer cycle
        if (SimpUtil::isMinusOne(base) && SimpUtil::isInteger(exp)) {
            Intg n = exp->value.numerator();
            Intg modRes = n % Intg(2);
            SimpUtil::freeTree(node);
            if (modRes == Intg(0)) {
                return SimpUtil::makeRational(Rational(Intg(1)));
            } else {
                return SimpUtil::makeRational(Rational(Intg(-1)));
            }
        }

        // i^n cycle
        if (SimpUtil::isConstantI(base) && SimpUtil::isInteger(exp)) {
            Intg n = exp->value.numerator();
            Intg mod4 = n % Intg(4);
            if (mod4 < Intg(0)) mod4 = mod4 + Intg(4);

            SimpUtil::freeTree(node);

            if (mod4 == Intg(0)) {
                return SimpUtil::makeRational(Rational(Intg(1)));
            } else if (mod4 == Intg(1)) {
                return SimpUtil::makeVariable(ConstName::i);
            } else if (mod4 == Intg(2)) {
                return SimpUtil::makeRational(Rational(Intg(-1)));
            } else { // mod4 == 3
                Exptree* negI = SimpUtil::makeFunction("*");
                negI->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                negI->child.push_back(SimpUtil::makeVariable(ConstName::i));
                return negI;
            }
        }

        // sqrt(-1) = i, sqrt(-a) = i*sqrt(a)
        Exptree* complexSqrt = handleComplexSqrt(node);
        if (complexSqrt) {
            return complexSqrt;
        }

        // (x^a)^b -> x^(a*b) when safe
        if (SimpUtil::isFunction(base, "^") && base->child.size() == 2) {
            bool canCombine = SimpUtil::isPositive(base->child[0]);
            if (!canCombine && SimpUtil::isRational(base->child[1]) && SimpUtil::isRational(exp)) {
                canCombine = base->child[1]->value.isInteger() && exp->value.isInteger();
            }

            if (canCombine) {
                Exptree* newExp = SimpUtil::makeFunction("*");
                newExp->child.push_back(SimpUtil::deepCopy(base->child[1]));
                newExp->child.push_back(SimpUtil::deepCopy(exp));
                newExp = simplifyMul(newExp);

                Exptree* result = SimpUtil::makeFunction("^");
                result->child.push_back(SimpUtil::deepCopy(base->child[0]));
                result->child.push_back(newExp);

                SimpUtil::freeTree(node);
                return result;
            }
        }

        // Euler's formula: e^(i*theta) -> cos(theta) + i*sin(theta)
        if (SimpUtil::isConstantE(base)) {
            Exptree* eulerResult = simplifyEulerForm(exp);
            if (eulerResult) {
                SimpUtil::freeTree(node);
                return eulerResult;
            }
        }

        // (a*b)^n -> a^n * b^n for integer n
        if (SimpUtil::isFunction(base, "*") && SimpUtil::isInteger(exp)) {
            Exptree* result = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < base->child.size(); ++i) {
                Exptree* powChild = SimpUtil::makeFunction("^");
                powChild->child.push_back(SimpUtil::deepCopy(base->child[i]));
                powChild->child.push_back(SimpUtil::deepCopy(exp));
                result->child.push_back(powChild);
            }
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        return node;
    }

    Exptree* TreeSimplifier::simplifyNeg(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // -0 = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // -(-x) = x
        if (SimpUtil::isFunction(arg, "*") && arg->child.size() == 2) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner = SimpUtil::deepCopy(arg->child[1]);
                SimpUtil::freeTree(node);
                return inner;
            }
        }

        // -n = -n for rational
        if (SimpUtil::isRational(arg)) {
            Rational negVal = Rational(Intg(0)) - arg->value;
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(negVal);
        }

        // Standardize: -x -> (-1)*x
        Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
        Exptree* result = SimpUtil::makeFunction("*");
        result->child.push_back(negOne);
        result->child.push_back(SimpUtil::deepCopy(arg));
        SimpUtil::freeTree(node);
        return result;
    }

} // namespace CAS