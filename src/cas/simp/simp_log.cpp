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

    // ========== Helper ==========

    /**
     *  @brief Check if an Intg is a power of 10 (1 followed by zeros)
     *  @param n      The integer to check
     *  @param power  Output: the exponent (e.g. 1000 → power=3)
     *  @return true if n = 10^k for some k >= 0
     */
    static bool isPowerOf10(Intg n, Intg &power) {
        if (n <= Intg(0)) return false;
        std::string s = (std::string)n;
        if (s[0] != '1') return false;
        for (size_t i = 1; i < s.size(); i++) {
            if (s[i] != '0') return false;
        }
        power = Intg((int64_t)(s.size() - 1));
        return true;
    }

    /**
     *  @brief Extract factor base^k from a rational number
     *  @param num    Numerator
     *  @param den    Denominator
     *  @param base   The base to factor out
     *  @param power  Output: the exponent k extracted
     *  @param remNum Output: remaining numerator after factoring
     *  @param remDen Output: remaining denominator after factoring
     *  @return true if any factors were extracted (power > 0)
     */
    static bool extractBasePower(Intg num, Intg den,
                                  Intg base, Intg &power,
                                  Intg &remNum, Intg &remDen) {
        power = Intg(0);
        remNum = num;
        remDen = den;
        if (base <= Intg(1)) return false;

        Intg maxIter(100000);

        // Factor from numerator
        while (remNum > Intg(1) && power < maxIter) {
            auto dr = remNum.divmod(base);
            if (dr.second != Intg(0)) break;
            remNum = dr.first;
            power = power + Intg(1);
        }

        // Factor from denominator (if numerator exhausted)
        if (remNum == Intg(1) && remDen > Intg(1)) {
            Intg denPower(0);
            while (remDen > Intg(1) && (power + denPower) < maxIter) {
                auto dr = remDen.divmod(base);
                if (dr.second != Intg(0)) break;
                remDen = dr.first;
                denPower = denPower + Intg(1);
            }
            if (denPower > Intg(0)) {
                power = power - denPower;
                return true;
            }
        }

        return power > Intg(0);
    }

    // ========== Ln ==========

    Exptree* TreeSimplifier::simplifyLn(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // ln(0) = -inf
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg("-inf")));
        }

        // ln(1) = 0
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // ln(e) = 1
        if (SimpUtil::isConstantE(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // ln(-1) = i*pi
        if (SimpUtil::isMinusOne(arg)) {
            SimpUtil::freeTree(node);
            Exptree* iPi = SimpUtil::makeFunction("*");
            iPi->child.push_back(SimpUtil::makeVariable(ConstName::i));
            iPi->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return iPi;
        }

        // ln(i) = i*pi/2
        if (SimpUtil::isConstantI(arg)) {
            SimpUtil::freeTree(node);
            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            Exptree* iPi = SimpUtil::makeFunction("*");
            iPi->child.push_back(SimpUtil::makeVariable(ConstName::i));
            iPi->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(half);
            result->child.push_back(iPi);
            return result;
        }

        // ln(-i) = -i*pi/2
        if (SimpUtil::isFunction(arg, "*") && arg->child.size() == 2) {
            if (SimpUtil::isMinusOne(arg->child[0]) && SimpUtil::isConstantI(arg->child[1])) {
                SimpUtil::freeTree(node);
                Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
                Exptree* negI = SimpUtil::makeFunction("*");
                negI->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                negI->child.push_back(SimpUtil::makeVariable(ConstName::i));
                Exptree* negIPi = SimpUtil::makeFunction("*");
                negIPi->child.push_back(negI);
                negIPi->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(half);
                result->child.push_back(negIPi);
                return result;
            }
        }

        // ln(-a) = ln(a) + i*pi (for a > 0, handled by preTransform)
        if (SimpUtil::isFunction(arg, "*")) {
            bool hasMinusOne = false;
            Exptree* posPart = nullptr;

            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isMinusOne(arg->child[i])) {
                    hasMinusOne = true;
                }
            }

            if (hasMinusOne) {
                Exptree* newPos = SimpUtil::makeFunction("*");
                for (size_t i = 0; i < arg->child.size(); ++i) {
                    if (!SimpUtil::isMinusOne(arg->child[i])) {
                        newPos->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }
                if (newPos->child.size() == 0) {
                    SimpUtil::freeTree(newPos);
                    return node;
                }
                if (newPos->child.size() == 1) {
                    posPart = newPos->child[0];
                    newPos->child.clear();
                    SimpUtil::freeTree(newPos);
                } else {
                    posPart = simplifyMul(newPos);
                }

                Exptree* lnPos = SimpUtil::makeFunction(FuncName::ln);
                lnPos->child.push_back(posPart);

                Exptree* iPi = SimpUtil::makeFunction("*");
                iPi->child.push_back(SimpUtil::makeVariable(ConstName::i));
                iPi->child.push_back(SimpUtil::makeVariable(ConstName::pi));

                Exptree* result = SimpUtil::makeFunction("+");
                result->child.push_back(lnPos);
                result->child.push_back(iPi);

                SimpUtil::freeTree(node);
                return simplifyAdd(result);
            }
        }

        // ln(e^x) = x
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isConstantE(arg->child[0])) {
                Exptree* inner = SimpUtil::deepCopy(arg->child[1]);
                SimpUtil::freeTree(node);
                return inner;
            }
        }

        // ln(rational): try to express as ln(a^b) = b*ln(a)
        if (SimpUtil::isRational(arg) && SimpUtil::isPositive(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;

            // Factorize numerator
            if (den == Intg(1) && num > Intg(1)) {
                // Try powers of small primes
                Intg smallPrimes[] = {Intg(2), Intg(3), Intg(5), Intg(7)};
                for (int p = 0; p < 4; ++p) {
                    Intg n = num;
                    Intg expCount(0);
                    while (n % smallPrimes[p] == Intg(0)) {
                        n = n / smallPrimes[p];
                        expCount = expCount + Intg(1);
                    }
                    if (expCount > Intg(0) && n == Intg(1)) {
                        // num = prime^expCount
                        SimpUtil::freeTree(node);
                        Exptree* result = SimpUtil::makeFunction("*");
                        result->child.push_back(SimpUtil::makeRational(Rational(expCount)));
                        Exptree* lnPrime = SimpUtil::makeFunction(FuncName::ln);
                        lnPrime->child.push_back(SimpUtil::makeRational(Rational(smallPrimes[p])));
                        result->child.push_back(lnPrime);
                        return simplifyMul(result);
                    }
                }
            }

            // Factorize denominator similarly if needed (for fractions)
            if (den > Intg(1)) {
                // ln(num/den) = ln(num) - ln(den)
                Exptree* lnNum = SimpUtil::makeFunction(FuncName::ln);
                lnNum->child.push_back(SimpUtil::makeRational(Rational(num)));
                Exptree* lnDen = SimpUtil::makeFunction(FuncName::ln);
                lnDen->child.push_back(SimpUtil::makeRational(Rational(den)));
                Exptree* negLnDen = SimpUtil::makeFunction("*");
                negLnDen->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                negLnDen->child.push_back(lnDen);
                Exptree* result = SimpUtil::makeFunction("+");
                result->child.push_back(lnNum);
                result->child.push_back(negLnDen);
                SimpUtil::freeTree(node);
                return simplifyAdd(result);
            }
        }

        // ln(x^a) = a*ln(x) — generalized: remove isPositive restriction
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            Exptree* a = SimpUtil::deepCopy(arg->child[1]);
            Exptree* lnBase = SimpUtil::makeFunction(FuncName::ln);
            lnBase->child.push_back(SimpUtil::deepCopy(arg->child[0]));

            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(a);
            result->child.push_back(lnBase);

            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // ln(a*b) = ln(a) + ln(b)
        if (SimpUtil::isFunction(arg, "*")) {
            Exptree* sum = SimpUtil::makeFunction("+");
            for (size_t i = 0; i < arg->child.size(); ++i) {
                Exptree* lnTerm = SimpUtil::makeFunction(FuncName::ln);
                lnTerm->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                sum->child.push_back(lnTerm);
            }
            SimpUtil::freeTree(node);
            return simplifyAdd(sum);
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

        // log(rational, integer_base): try to factor out base^k
        if (base && SimpUtil::isRational(arg) && SimpUtil::isInteger(base)) {
            Intg b = base->value.numerator();
            if (b > Intg(1)) {
                Intg num = arg->value.numerator();
                Intg den = arg->value.den;
                Intg power, remNum, remDen;
                if (extractBasePower(num, den, b, power, remNum, remDen)) {
                    // arg = base^power * (remNum/remDen)
                    Exptree* logRem = SimpUtil::makeFunction(FuncName::log);
                    logRem->child.push_back(SimpUtil::makeRational(Rational(remNum, remDen)));
                    logRem->child.push_back(SimpUtil::deepCopy(base));
                    Exptree* result;
                    if (power == Intg(1)) {
                        result = SimpUtil::makeFunction("+");
                        result->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                        result->child.push_back(logRem);
                    } else {
                        result = SimpUtil::makeFunction("+");
                        result->child.push_back(SimpUtil::makeRational(Rational(power)));
                        result->child.push_back(logRem);
                    }
                    SimpUtil::freeTree(node);
                    return simplifyAdd(result);
                }
            }
        }

        // log(a*b, base) = log(a, base) + log(b, base)
        if (SimpUtil::isFunction(arg, "*")) {
            Exptree* sum = SimpUtil::makeFunction("+");
            for (size_t i = 0; i < arg->child.size(); ++i) {
                Exptree* logTerm = SimpUtil::makeFunction(FuncName::log);
                logTerm->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                if (base) logTerm->child.push_back(SimpUtil::deepCopy(base));
                sum->child.push_back(logTerm);
            }
            SimpUtil::freeTree(node);
            return simplifyAdd(sum);
        }

        // log(a^b, base) = b * log(a, base)
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            Exptree* b = SimpUtil::deepCopy(arg->child[1]);
            Exptree* logA = SimpUtil::makeFunction(FuncName::log);
            logA->child.push_back(SimpUtil::deepCopy(arg->child[0]));
            if (base) logA->child.push_back(SimpUtil::deepCopy(base));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(b);
            result->child.push_back(logA);
            SimpUtil::freeTree(node);
            return simplifyMul(result);
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

        // log10(rational): factor out 10^k
        if (SimpUtil::isRational(arg)) {
            Intg num = arg->value.numerator();
            Intg den = arg->value.den;
            Intg power, remNum, remDen;

            if (extractBasePower(num, den, Intg(10), power, remNum, remDen)) {
                if (remNum == Intg(1) && remDen == Intg(1)) {
                    // Exact power of 10: log10(10^k) = k
                    SimpUtil::freeTree(node);
                    return SimpUtil::makeRational(Rational(power));
                }
                // log10(10^k * m) = k + log10(m)
                Exptree* logRem = SimpUtil::makeFunction(FuncName::log10);
                logRem->child.push_back(SimpUtil::makeRational(Rational(remNum, remDen)));
                Exptree* result = SimpUtil::makeFunction("+");
                result->child.push_back(SimpUtil::makeRational(Rational(power)));
                result->child.push_back(logRem);
                SimpUtil::freeTree(node);
                return simplifyAdd(result);
            }
        }

        // log10(k * x) = log10(k) + log10(x)  if k is a power of 10
        // also handles log10(10 * x) = 1 + log10(x)
        if (SimpUtil::isFunction(arg, "*") && arg->child.size() == 2) {
            Exptree *c1 = arg->child[0], *c2 = arg->child[1];
            for (int pass = 0; pass < 2; pass++) {
                Exptree* kNode  = (pass == 0) ? c1 : c2;
                Exptree* other  = (pass == 0) ? c2 : c1;
                if (SimpUtil::isRational(kNode)) {
                    Intg num = kNode->value.numerator();
                    Intg den = kNode->value.den;
                    Intg power;
                    if (den == Intg(1) && isPowerOf10(num, power)) {
                        Exptree* logOther = SimpUtil::makeFunction(FuncName::log10);
                        logOther->child.push_back(SimpUtil::deepCopy(other));
                        Exptree* result = SimpUtil::makeFunction("+");
                        result->child.push_back(SimpUtil::makeRational(Rational(power)));
                        result->child.push_back(logOther);
                        SimpUtil::freeTree(node);
                        return simplifyAdd(result);
                    }
                }
            }
        }

        // log10(x / 10) = log10(x) - 1
        if (SimpUtil::isFunction(arg, "/") && arg->child.size() == 2 &&
            SimpUtil::isRational(arg->child[1]) &&
            arg->child[1]->value == Rational(Intg(10))) {
            Exptree* logNum = SimpUtil::makeFunction(FuncName::log10);
            logNum->child.push_back(SimpUtil::deepCopy(arg->child[0]));
            Exptree* result = SimpUtil::makeFunction("+");
            result->child.push_back(logNum);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            SimpUtil::freeTree(node);
            return simplifyAdd(result);
        }

        // log10(x^n) = n * log10(x)  (includes sqrt via x^(1/2))
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            Exptree* logBase = SimpUtil::makeFunction(FuncName::log10);
            logBase->child.push_back(SimpUtil::deepCopy(arg->child[0]));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(SimpUtil::deepCopy(arg->child[1]));
            result->child.push_back(logBase);
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // Fallback: log10(x) = ln(x) * ln(10)^(-1)
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