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
            bool base_i_owned = false;

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
                        base_i_owned = true;
                    }
                }
            }

            // Search for matching terms
            for (size_t j = i + 1; j < buf.count; ++j) {
                if (!buf.items[j]) continue;

                Exptree* term_j = buf.items[j];
                Rational coeff_j(Intg(1));
                Exptree* base_j = term_j;
                bool base_j_owned = false;

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
                            base_j_owned = true;
                        }
                    }
                }

                if (SimpUtil::compareNodes(base_i, base_j) == 0) {
                    // Same base: add coefficients
                    coeff_i = coeff_i + coeff_j;
                    SimpUtil::freeTree(buf.items[j]);
                    buf.items[j] = nullptr;
                    if (base_j_owned) {
                        SimpUtil::freeTree(base_j);
                    }
                } else if (SimpUtil::isFunction(base_i, "^") && base_i->child.size() == 2 &&
                           SimpUtil::isFunction(base_j, "^") && base_j->child.size() == 2 &&
                           SimpUtil::isRational(base_i->child[1]) && SimpUtil::isRational(base_j->child[1])) {

                    Exptree* powBase_i = base_i->child[0];
                    Exptree* powBase_j = base_j->child[0];

                    if (SimpUtil::compareNodes(powBase_i, powBase_j) == 0) {
                        Rational exp_i = base_i->child[1]->value;
                        Rational exp_j = base_j->child[1]->value;

                        // base^exp_i + base^exp_j = base^minExp * (base^(exp_i-minExp) + base^(exp_j-minExp))
                        Rational minExp = (exp_i < exp_j) ? exp_i : exp_j;
                        Rational diff_i = exp_i - minExp;
                        Rational diff_j = exp_j - minExp;

                        // Build inner sum without calling simplifyAdd (avoids addBuf_ reentrancy)
                        Exptree* sum = SimpUtil::makeFunction("+");

                        // Term from i
                        if (diff_i.isZero()) {
                            sum->child.push_back(SimpUtil::makeRational(coeff_i));
                        } else {
                            Exptree* pow_i = SimpUtil::makeFunction("^");
                            pow_i->child.push_back(SimpUtil::deepCopy(powBase_i));
                            pow_i->child.push_back(SimpUtil::makeRational(diff_i));
                            if (coeff_i == Rational(Intg(1))) {
                                sum->child.push_back(pow_i);
                            } else {
                                Exptree* term = SimpUtil::makeFunction("*");
                                term->child.push_back(SimpUtil::makeRational(coeff_i));
                                term->child.push_back(pow_i);
                                sum->child.push_back(term);
                            }
                        }

                        // Term from j
                        if (diff_j.isZero()) {
                            sum->child.push_back(SimpUtil::makeRational(coeff_j));
                        } else {
                            Exptree* pow_j = SimpUtil::makeFunction("^");
                            pow_j->child.push_back(SimpUtil::deepCopy(powBase_i));
                            pow_j->child.push_back(SimpUtil::makeRational(diff_j));
                            if (coeff_j == Rational(Intg(1))) {
                                sum->child.push_back(pow_j);
                            } else {
                                Exptree* term = SimpUtil::makeFunction("*");
                                term->child.push_back(SimpUtil::makeRational(coeff_j));
                                term->child.push_back(pow_j);
                                sum->child.push_back(term);
                            }
                        }

                        Exptree* newTerm;
                        if (minExp.isZero()) {
                            newTerm = sum;
                        } else {
                            // factor = base^minExp
                            Exptree* factor = SimpUtil::makeFunction("^");
                            factor->child.push_back(SimpUtil::deepCopy(powBase_i));
                            factor->child.push_back(SimpUtil::makeRational(minExp));
                            // newTerm = factor * sum
                            newTerm = SimpUtil::makeFunction("*");
                            newTerm->child.push_back(factor);
                            newTerm->child.push_back(sum);
                        }

                        if (term_i != base_i) {
                            term_i->child.clear();
                        }
                        if (term_j != base_j) {
                            term_j->child.clear();
                        }
                        SimpUtil::freeTree(buf.items[i]);
                        SimpUtil::freeTree(buf.items[j]);
                        buf.items[i] = newTerm;
                        buf.items[j] = nullptr;

                        coeff_i = Rational(Intg(1));
                        base_i = newTerm;
                        base_i_owned = false;
                    }
                }
                if (base_j_owned) {
                    SimpUtil::freeTree(base_j);
                }
            }

            // Rebuild term with merged coefficient
            if (coeff_i.isZero()) {
                SimpUtil::freeTree(buf.items[i]);
                buf.items[i] = nullptr;
                if (base_i_owned) {
                    SimpUtil::freeTree(base_i);
                }
            } else if (coeff_i == Rational(Intg(1))) {
                if (base_i_owned) {
                    SimpUtil::freeTree(buf.items[i]);
                    buf.items[i] = base_i;
                }
            } else {
                Exptree* newTerm = SimpUtil::makeFunction("*");
                newTerm->child.push_back(SimpUtil::makeRational(coeff_i));
                if (base_i_owned) {
                    newTerm->child.push_back(base_i);
                } else {
                    newTerm->child.push_back(SimpUtil::deepCopy(base_i));
                }
                SimpUtil::freeTree(buf.items[i]);
                buf.items[i] = newTerm;
            }
        }

        // Compact array
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
        // Check if all children are vectors or all are matrices
        if (node->child.size() > 0) {
            bool allVec = true, allMat = true;
            for (size_t i = 0; i < node->child.size(); ++i) {
                if (!SimpUtil::isVectorNode(node->child[i])) allVec = false;
                if (!SimpUtil::isMatrixNode(node->child[i])) allMat = false;
            }
            if (allVec || allMat) {
                return simplifyAddVM(node);
            }
        }

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
                        // Immediately simplify the power node
                        // This handles i^2 -> -1, i^3 -> -i, etc.
                        newPow = simplifyPow(newPow);
                        SimpUtil::freeTree(buf.items[i]);
                        SimpUtil::freeTree(buf.items[j]);
                        buf.items[i] = newPow;
                        buf.items[j] = nullptr;
                    }
                    break;
                }
            }
        }

        // Try to absorb rational constant into a power base
        // e.g. (1/3) * 3^(1/2) -> 3^(-1/2)
        if (buf.constantAccum != Rational(Intg(1)) && !buf.constantAccum.isZero()) {
            for (size_t i = 0; i < buf.count; ++i) {
                if (!buf.items[i]) continue;
                if (SimpUtil::isFunction(buf.items[i], "^") && buf.items[i]->child.size() == 2) {
                    Exptree* powBase = buf.items[i]->child[0];
                    Exptree* powExp = buf.items[i]->child[1];
                    if (SimpUtil::isRational(powBase) && SimpUtil::isRational(powExp)) {
                        Intg bNum = powBase->value.numerator();
                        Intg bDen = powBase->value.den;
                        Rational r = buf.constantAccum;
                        bool absorbed = false;
                        Rational newExp;

                        // Check: r = 1/b?  -> b^(e-1)
                        if (bDen == Intg(1) && r == Rational(Intg(1), bNum)) {
                            newExp = powExp->value - Rational(Intg(1));
                            absorbed = true;
                        }
                        // Check: r = b?  -> b^(e+1)
                        else if (bDen == Intg(1) && r == Rational(bNum, Intg(1))) {
                            newExp = powExp->value + Rational(Intg(1));
                            absorbed = true;
                        }
                        // Check: r = b^k for integer k?
                        else if (bDen == Intg(1) && r.den == Intg(1)) {
                            // r = n, b = m. Check if n = m^k
                            Intg n = r.numerator();
                            Intg m = bNum;
                            if (n > Intg(1) && m > Intg(1)) {
                                Intg k(0);
                                Intg tmp = n;
                                while (tmp % m == Intg(0)) {
                                    tmp = tmp / m;
                                    k = k + Intg(1);
                                }
                                if (tmp == Intg(1) && k > Intg(0)) {
                                    newExp = powExp->value + Rational(k, Intg(1));
                                    absorbed = true;
                                }
                            }
                        }
                        // Check: r = b^(-k) -> 1/b^k?
                        else if (bDen == Intg(1) && r.numerator() == Intg(1)) {
                            Intg m = bNum;
                            Intg denom = r.den;
                            Intg k(0);
                            Intg tmp = denom;
                            while (tmp % m == Intg(0)) {
                                tmp = tmp / m;
                                k = k + Intg(1);
                            }
                            if (tmp == Intg(1) && k > Intg(0)) {
                                newExp = powExp->value - Rational(k, Intg(1));
                                absorbed = true;
                            }
                        }

                        if (absorbed) {
                            buf.constantAccum = Rational(Intg(1));
                            Exptree* newPow = SimpUtil::makeFunction("^");
                            newPow->child.push_back(SimpUtil::deepCopy(powBase));
                            newPow->child.push_back(SimpUtil::makeRational(newExp));
                            newPow = simplifyPow(newPow);
                            SimpUtil::freeTree(buf.items[i]);
                            buf.items[i] = newPow;
                            break;
                        }
                    }
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
                buf.count = 0;
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
            Exptree* result = buf.items[0];
            buf.items[0] = nullptr;
            buf.count = 0;
            return result;
        }

        sortItems(buf.items, buf.count);

        Exptree* result = SimpUtil::makeFunction("*");
        for (size_t i = 0; i < buf.count; ++i) {
            result->child.push_back(buf.items[i]);
        }
        buf.count = 0;

        return result;
    }

    Exptree* TreeSimplifier::simplifyMul(Exptree* node) {
        // Check for vector/matrix multiplication
        // 2 : matrix*matrix, vector*vector
        if (node->child.size() == 2) {
            Exptree* a = node->child[0];
            Exptree* b = node->child[1];
            if ((SimpUtil::isVectorNode(a) || SimpUtil::isMatrixNode(a)) &&
                (SimpUtil::isVectorNode(b) || SimpUtil::isMatrixNode(b))) {
                return simplifyMulVM(node);
            }
        }
        // scalar * vector/matrix
        bool hasVM = false;
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (SimpUtil::isVectorNode(node->child[i]) || SimpUtil::isMatrixNode(node->child[i])) {
                hasVM = true;
                break;
            }
        }
        if (hasVM) {
            return simplifyMulVM(node);
        }

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

            // 10^n: fast path using string construction
            if (base->value == Rational(Intg(10)) || base->value == Rational(Intg(-10))) {
                bool neg = (base->value == Rational(Intg(-10)));
                if (expNum < Intg(0)) {
                    // 10^(-n) = 1/10^n
                    Intg posExp = Intg(0) - expNum;
                    std::string s(posExp.toInt(), '0');
                    s = "1" + s;
                    Intg den(s);
                    if (neg && (posExp[0] & 1)) den = Intg(0) - den;
                    return SimpUtil::makeRational(Rational(Intg(1), den));
                }
                // 10^n
                int64_t n = expNum.toInt();
                std::string s(n, '0');
                s = "1" + s;
                Intg result(s);
                if (neg && (n & 1)) result = Intg(0) - result;
                return SimpUtil::makeRational(Rational(result));
            }

            // General case
            if (expNum < Intg(0)) {
                Intg posExp = Intg(0) - expNum;
                Intg baseNum = base->value.numerator();
                Intg baseDen = base->value.den;
                Intg resultNum = baseDen.pow(posExp);
                Intg resultDen = baseNum.pow(posExp);
                return SimpUtil::makeRational(Rational(resultNum, resultDen));
            }
            Intg baseNum = base->value.numerator();
            Intg baseDen = base->value.den;
            Intg resultNum = baseNum.pow(expNum);
            Intg resultDen = baseDen.pow(expNum);
            return SimpUtil::makeRational(Rational(resultNum, resultDen));
        }

        Intg expNum = exp->value.numerator();
        Intg expDen = exp->value.den;
        if (expNum == Intg(1) && expDen == Intg(2)) {
            Rational root;
            if (isPerfectSquare(base->value, root)) {
                return SimpUtil::makeRational(root);
            }
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
                    // Multiple non-i factors: multiply them together
                    Exptree* prod = SimpUtil::makeFunction("*");
                    prod->child.push_back(thetaPart);
                    prod->child.push_back(expArg->child[i]);
                    thetaPart = prod;
                }
            }
        }

        if (hasI) {
            if (!thetaPart) {
                thetaPart = SimpUtil::makeRational(Rational(Intg(1)));
            }

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

        // sqrt(-a) for negative rational a
        if (SimpUtil::isNegative(base)) {
            Rational posVal = Rational(Intg(0)) - base->value;
            Exptree* sqrtPos = SimpUtil::makeFunction("^");
            sqrtPos->child.push_back(SimpUtil::makeRational(posVal));
            sqrtPos->child.push_back(SimpUtil::makeRational(Rational(Intg(1), Intg(2))));
            sqrtPos = simplifyPow(sqrtPos);

            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(SimpUtil::makeVariable(ConstName::i));
            result->child.push_back(sqrtPos);

            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // sqrt(a*i) = (1+i)*sqrt(a/2) for a > 0
        if (SimpUtil::isFunction(base, "*")) {
            Rational coeff(Intg(1));
            bool hasI = false;
            bool hasOther = false;
            for (size_t i = 0; i < base->child.size(); ++i) {
                if (SimpUtil::isConstantI(base->child[i])) {
                    hasI = true;
                } else if (SimpUtil::isRational(base->child[i])) {
                    coeff = coeff * base->child[i]->value;
                } else {
                    hasOther = true;
                    break;
                }
            }
            if (hasI && !hasOther && coeff > Rational(Intg(0))) {
                // sqrt(a*i) = (1+i)*sqrt(a/2)
                Rational halfA = coeff * Rational(Intg(1), Intg(2));

                Exptree* sqrtHalfA = SimpUtil::makeFunction(FuncName::sqrt);
                sqrtHalfA->child.push_back(SimpUtil::makeRational(halfA));
                sqrtHalfA = simplifySqrt(sqrtHalfA);

                Exptree* onePlusI = SimpUtil::makeFunction("+");
                onePlusI->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                onePlusI->child.push_back(SimpUtil::makeVariable(ConstName::i));

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(onePlusI);
                result->child.push_back(sqrtHalfA);

                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        // sqrt(-a*i) = (1-i)*sqrt(a/2) for a > 0
        if (SimpUtil::isFunction(base, "*")) {
            bool hasMinusOne = false;
            bool hasI = false;
            Rational coeff(Intg(1));
            bool hasOther = false;
            for (size_t i = 0; i < base->child.size(); ++i) {
                if (SimpUtil::isMinusOne(base->child[i])) {
                    hasMinusOne = true;
                } else if (SimpUtil::isConstantI(base->child[i])) {
                    hasI = true;
                } else if (SimpUtil::isRational(base->child[i])) {
                    coeff = coeff * base->child[i]->value;
                } else {
                    hasOther = true;
                    break;
                }
            }
            if (hasMinusOne && hasI && !hasOther && coeff > Rational(Intg(0))) {
                // sqrt(-a*i) = (1-i)*sqrt(a/2)
                Rational halfA = coeff * Rational(Intg(1), Intg(2));

                Exptree* sqrtHalfA = SimpUtil::makeFunction(FuncName::sqrt);
                sqrtHalfA->child.push_back(SimpUtil::makeRational(halfA));
                sqrtHalfA = simplifySqrt(sqrtHalfA);

                Exptree* oneMinusI = SimpUtil::makeFunction("+");
                oneMinusI->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                Exptree* negI = SimpUtil::makeFunction("*");
                negI->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                negI->child.push_back(SimpUtil::makeVariable(ConstName::i));
                oneMinusI->child.push_back(negI);

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(oneMinusI);
                result->child.push_back(sqrtHalfA);

                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        return nullptr;
    }

    Exptree* TreeSimplifier::simplifyPow(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* base = node->child[0];
        Exptree* exp = node->child[1];

        // 0^0 -> NaN
        if (SimpUtil::isZero(base) && SimpUtil::isZero(exp)) {
            node->child.clear();
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg("NaN")));
        }

        // 0^x = 0 (x != 0)
        if (SimpUtil::isZero(base)) {
            node->child.clear();
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // x^0 = 1
        if (SimpUtil::isZero(exp)) {
            // Check for matrix^0 = I
            if (SimpUtil::isMatrixNode(base)) {
                size_t dim = 0, dummy = 0;
                SimpUtil::matrixDims(base, dim, dummy);
                node->child.clear();
                SimpUtil::freeTree(node);
                Exptree* I = SimpUtil::makeFunction(FuncName::matrix);
                I->child.push_back(SimpUtil::makeRational(Rational(Intg((int)dim))));
                I->child.push_back(SimpUtil::makeRational(Rational(Intg((int)dim))));
                for (size_t i = 0; i < dim; ++i)
                    for (size_t j = 0; j < dim; ++j)
                        I->child.push_back(SimpUtil::makeRational(i == j ? Rational(Intg(1)) : Rational(Intg(0))));
                return I;
            }
            node->child.clear();
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // x^1 = x
        if (SimpUtil::isOne(exp)) {
            node->child.clear();
            SimpUtil::freeTree(node);
            return base;
        }

        // 1^x = 1
        if (SimpUtil::isOne(base)) {
            node->child.clear();
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // Matrix power: A^n or A^(-1)
        if (SimpUtil::isMatrixNode(base) && SimpUtil::isInteger(exp)) {
            Intg expVal = exp->value.numerator();

            if (expVal == Intg(0)) {
                size_t dim = 0, dummy = 0;
                SimpUtil::matrixDims(base, dim, dummy);
                node->child.clear();
                SimpUtil::freeTree(node);
                Exptree* I = SimpUtil::makeFunction(FuncName::matrix);
                I->child.push_back(SimpUtil::makeRational(Rational(Intg((int)dim))));
                I->child.push_back(SimpUtil::makeRational(Rational(Intg((int)dim))));
                for (size_t i = 0; i < dim; ++i)
                    for (size_t j = 0; j < dim; ++j)
                        I->child.push_back(SimpUtil::makeRational(i == j ? Rational(Intg(1)) : Rational(Intg(0))));
                return I;
            }

            if (expVal == Intg(1)) {
                node->child.clear();
                SimpUtil::freeTree(node);
                return base;
            }

            if (expVal == Intg(-1)) {
                // A^(-1) = adj(A) / det(A)
                Exptree* adj = SimpUtil::makeFunction(FuncName::adjoint);
                adj->child.push_back(SimpUtil::deepCopy(base));
                adj = simplifyAdjoint(adj);

                Exptree* detNode = SimpUtil::makeFunction(FuncName::det);
                detNode->child.push_back(SimpUtil::deepCopy(base));
                detNode = simplifyDet(detNode);

                if (SimpUtil::isMatrixNode(adj) && SimpUtil::isRational(detNode) && !detNode->value.isZero()) {
                    Rational invDet = Rational(Intg(1)) / detNode->value;
                    Exptree* result = SimpUtil::makeFunction("*");
                    result->child.push_back(SimpUtil::makeRational(invDet));
                    result->child.push_back(adj);
                    SimpUtil::freeTree(detNode);
                    node->child.clear();
                    SimpUtil::freeTree(node);
                    return simplifyMul(result);
                }
                SimpUtil::freeTree(adj);
                SimpUtil::freeTree(detNode);
                return node;
            }

            if (expVal < Intg(-1)) {
                // A^(-n) = (A^(-1))^n
                Exptree* inv = SimpUtil::makeFunction("^");
                inv->child.push_back(SimpUtil::deepCopy(base));
                inv->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                inv = simplifyPow(inv);

                if (SimpUtil::isMatrixNode(inv)) {
                    Intg posExp = Intg(0) - expVal;
                    Exptree* result = SimpUtil::deepCopy(inv);
                    for (Intg i(1); i < posExp; i = i + Intg(1)) {
                        Exptree* prod = SimpUtil::makeFunction("*");
                        prod->child.push_back(result);
                        prod->child.push_back(SimpUtil::deepCopy(inv));
                        result = simplifyMul(prod);
                    }
                    SimpUtil::freeTree(inv);
                    node->child.clear();
                    SimpUtil::freeTree(node);
                    return result;
                }
                SimpUtil::freeTree(inv);
                return node;
            }

            // Positive power > 1
            Exptree* result = SimpUtil::deepCopy(base);
            for (Intg i(1); i < expVal; i = i + Intg(1)) {
                Exptree* prod = SimpUtil::makeFunction("*");
                prod->child.push_back(result);
                prod->child.push_back(SimpUtil::deepCopy(base));
                result = simplifyMul(prod);
            }
            node->child.clear();
            SimpUtil::freeTree(node);
            return result;
        }

        // Vector power: not defined, leave as-is
        if (SimpUtil::isVectorNode(base)) {
            return node;
        }

        // Constant folding for rational base and exponent
        Exptree* folded = foldRationalPower(base, exp);
        if (folded) {
            node->child.clear();
            SimpUtil::freeTree(node);
            return folded;
        }

        // Handle x^(1/2): extract perfect square factors
        if (SimpUtil::isRational(exp) && exp->value == Rational(Intg(1), Intg(2))) {
            if (SimpUtil::isRational(base) && SimpUtil::isPositive(base)) {
                Intg num = base->value.numerator();
                Intg den = base->value.den;
                Rational outside(Intg(1));

                for (Intg i(2); i * i <= num; i = i + Intg(1)) {
                    Intg i2 = i * i;
                    while (num % i2 == Intg(0)) {
                        outside = outside * Rational(i, Intg(1));
                        num = num / i2;
                    }
                }

                for (Intg i(2); i * i <= den; i = i + Intg(1)) {
                    Intg i2 = i * i;
                    while (den % i2 == Intg(0)) {
                        Rational denFactor(Intg(1), i);
                        outside = outside * denFactor;
                        den = den / i2;
                    }
                }

                if (outside != Rational(Intg(1))) {
                    Rational remaining(num, den);
                    if (remaining == Rational(Intg(1))) {
                        node->child.clear();
                        SimpUtil::freeTree(node);
                        return SimpUtil::makeRational(outside);
                    }

                    Exptree* result = SimpUtil::makeFunction("*");
                    result->child.push_back(SimpUtil::makeRational(outside));

                    Exptree* innerPow = SimpUtil::makeFunction("^");
                    innerPow->child.push_back(SimpUtil::makeRational(remaining));
                    innerPow->child.push_back(SimpUtil::makeRational(Rational(Intg(1), Intg(2))));

                    result->child.push_back(innerPow);
                    node->child.clear();
                    SimpUtil::freeTree(node);
                    return simplifyMul(result);
                }
            }
        }

        // (-1)^integer cycle
        if (SimpUtil::isMinusOne(base) && SimpUtil::isInteger(exp)) {
            Intg n = exp->value.numerator();
            Intg modRes = n % Intg(2);
            node->child.clear();
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(modRes == Intg(0) ? Rational(Intg(1)) : Rational(Intg(-1)));
        }

        // i^n cycle
        if (SimpUtil::isConstantI(base) && SimpUtil::isInteger(exp)) {
            Intg n = exp->value.numerator();
            Intg mod4 = n % Intg(4);
            if (mod4 < Intg(0)) mod4 = mod4 + Intg(4);

            node->child.clear();
            SimpUtil::freeTree(node);

            if (mod4 == Intg(0)) return SimpUtil::makeRational(Rational(Intg(1)));
            if (mod4 == Intg(1)) return SimpUtil::makeVariable(ConstName::i);
            if (mod4 == Intg(2)) return SimpUtil::makeRational(Rational(Intg(-1)));

            Exptree* negI = SimpUtil::makeFunction("*");
            negI->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negI->child.push_back(SimpUtil::makeVariable(ConstName::i));
            return negI;
        }

        // Complex power: (a*i)^integer -> a^n * i^n
        if (SimpUtil::isFunction(base, "*") && SimpUtil::isInteger(exp)) {
            bool hasI = false;
            for (size_t i = 0; i < base->child.size(); ++i) {
                if (SimpUtil::isConstantI(base->child[i])) { hasI = true; break; }
            }
            if (hasI) {
                Exptree* realPart = SimpUtil::makeFunction("*");
                Exptree* iPart = nullptr;
                for (size_t i = 0; i < base->child.size(); ++i) {
                    if (SimpUtil::isConstantI(base->child[i])) {
                        iPart = SimpUtil::makeFunction("^");
                        iPart->child.push_back(SimpUtil::deepCopy(base->child[i]));
                        iPart->child.push_back(SimpUtil::deepCopy(exp));
                    } else {
                        Exptree* powChild = SimpUtil::makeFunction("^");
                        powChild->child.push_back(SimpUtil::deepCopy(base->child[i]));
                        powChild->child.push_back(SimpUtil::deepCopy(exp));
                        realPart->child.push_back(powChild);
                    }
                }
                realPart = simplifyMul(realPart);
                if (iPart) iPart = simplifyPow(iPart);

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(realPart);
                if (iPart) result->child.push_back(iPart);

                node->child.clear();
                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        // i^i and (a*i)^(b*i) — convert to exponential form
        bool exp_has_i = SimpUtil::isConstantI(exp);
        if (!exp_has_i && SimpUtil::isFunction(exp, "*")) {
            for (size_t k = 0; k < exp->child.size(); ++k) {
                if (SimpUtil::isConstantI(exp->child[k])) { exp_has_i = true; break; }
            }
        }

        if (exp_has_i) {
            if (SimpUtil::isConstantI(base)) {
                node->child.clear();
                SimpUtil::freeTree(node);
                Exptree* negHalf = SimpUtil::makeRational(Rational(Intg(-1), Intg(2)));
                Exptree* negPiHalf = SimpUtil::makeFunction("*");
                negPiHalf->child.push_back(negHalf);
                negPiHalf->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                Exptree* result = SimpUtil::makeFunction("^");
                result->child.push_back(SimpUtil::makeVariable(ConstName::e));
                result->child.push_back(negPiHalf);
                return result;
            }

            bool base_has_i = false;
            if (SimpUtil::isFunction(base, "*")) {
                for (size_t k = 0; k < base->child.size(); ++k) {
                    if (SimpUtil::isConstantI(base->child[k])) { base_has_i = true; break; }
                }
            }

            if (base_has_i) {
                Exptree* a = SimpUtil::makeFunction("*");
                for (size_t k = 0; k < base->child.size(); ++k)
                    if (!SimpUtil::isConstantI(base->child[k]))
                        a->child.push_back(SimpUtil::deepCopy(base->child[k]));
                if (a->child.size() == 0) { SimpUtil::freeTree(a); a = SimpUtil::makeRational(Rational(Intg(1))); }
                else if (a->child.size() == 1) { Exptree* s = a->child[0]; a->child.clear(); SimpUtil::freeTree(a); a = s; }
                else a = simplifyMul(a);

                Exptree* b;
                if (SimpUtil::isConstantI(exp)) b = SimpUtil::makeRational(Rational(Intg(1)));
                else {
                    Exptree* bNode = SimpUtil::makeFunction("*");
                    for (size_t k = 0; k < exp->child.size(); ++k)
                        if (!SimpUtil::isConstantI(exp->child[k]))
                            bNode->child.push_back(SimpUtil::deepCopy(exp->child[k]));
                    if (bNode->child.size() == 0) { SimpUtil::freeTree(bNode); b = SimpUtil::makeRational(Rational(Intg(1))); }
                    else if (bNode->child.size() == 1) { b = bNode->child[0]; bNode->child.clear(); SimpUtil::freeTree(bNode); }
                    else b = simplifyMul(bNode);
                }

                Exptree* negHalf = SimpUtil::makeRational(Rational(Intg(-1), Intg(2)));
                Exptree* bPi = SimpUtil::makeFunction("*");
                bPi->child.push_back(SimpUtil::deepCopy(b));
                bPi->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                Exptree* expTerm = SimpUtil::makeFunction("*");
                expTerm->child.push_back(negHalf);
                expTerm->child.push_back(bPi);
                expTerm = simplifyMul(expTerm);

                Exptree* ePart = SimpUtil::makeFunction("^");
                ePart->child.push_back(SimpUtil::makeVariable(ConstName::e));
                ePart->child.push_back(expTerm);

                Exptree* aExpBI = SimpUtil::makeFunction("^");
                aExpBI->child.push_back(SimpUtil::deepCopy(a));
                aExpBI->child.push_back(SimpUtil::deepCopy(exp));

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(ePart);
                result->child.push_back(aExpBI);

                node->child.clear();
                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        // sqrt(-1) = i, sqrt(-a) = i*sqrt(a)
        Exptree* complexSqrt = handleComplexSqrt(node);
        if (complexSqrt) return complexSqrt;

        // (x^a)^b -> x^(a*b) when safe
        if (SimpUtil::isFunction(base, "^") && base->child.size() == 2) {
            bool canCombine = SimpUtil::isPositive(base->child[0]);
            if (!canCombine && SimpUtil::isRational(base->child[1]) && SimpUtil::isRational(exp))
                canCombine = base->child[1]->value.isInteger() && exp->value.isInteger();
            if (!canCombine && SimpUtil::isRational(base->child[1]) && SimpUtil::isRational(exp)) {
                Rational newExpVal = base->child[1]->value * exp->value;
                if (newExpVal.isInteger()) canCombine = true;
            }
            if (canCombine) {
                Exptree* newExp = SimpUtil::makeFunction("*");
                newExp->child.push_back(SimpUtil::deepCopy(base->child[1]));
                newExp->child.push_back(SimpUtil::deepCopy(exp));
                newExp = simplifyMul(newExp);
                Exptree* result = SimpUtil::makeFunction("^");
                result->child.push_back(SimpUtil::deepCopy(base->child[0]));
                result->child.push_back(newExp);
                node->child.clear();
                SimpUtil::freeTree(node);
                return result;
            }
        }

        // Euler's formula: e^(i*theta) -> cos(theta) + i*sin(theta)
        if (SimpUtil::isConstantE(base)) {
            Exptree* eulerResult = simplifyEulerForm(exp);
            if (eulerResult) {
                node->child.clear();
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
            node->child.clear();
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