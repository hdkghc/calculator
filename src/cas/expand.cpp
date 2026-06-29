/** @file /src/cas/expand.cpp
 *  @brief Polynomial expansion for the computer algebra system module
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

#include "cas/expand.hpp"

namespace CAS {

    TreeExpander& TreeExpander::instance() {
        static TreeExpander inst;
        return inst;
    }

    Exptree* TreeExpander::expand(Exptree*& root) {
        if (!root) return nullptr;
        TreeExpander& exp = instance();
        root = exp.expandNode(root);
        return root;
    }

    Exptree* TreeExpander::expandNode(Exptree* node) {
        if (!node) return nullptr;
        if (node->valtp == Exptree::val_t::valRational ||
            node->valtp == Exptree::val_t::valVariable) {
            return node;
        }
        if (node->valtp != Exptree::val_t::valFunction) return node;

        for (size_t i = 0; i < node->child.size(); ++i) {
            node->child[i] = expandNode(node->child[i]);
        }

        const std::string& func = node->var;

        if (func == "*") return expandMul(node);
        if (func == "^") return expandPow(node);

        return node;
    }

    Exptree* TreeExpander::expandMul(Exptree* node) {
        // Distribute multiplication over addition: *(a+b, c+d, ...) -> a*c + a*d + b*c + b*d
        // Find first sum child
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (SimpUtil::isFunction(node->child[i], "+")) {
                Exptree* sum = node->child[i];
                Exptree* result = SimpUtil::makeFunction("+");

                for (size_t j = 0; j < sum->child.size(); ++j) {
                    Exptree* term = SimpUtil::makeFunction("*");
                    for (size_t k = 0; k < node->child.size(); ++k) {
                        if (k == i) {
                            term->child.push_back(SimpUtil::deepCopy(sum->child[j]));
                        } else {
                            term->child.push_back(SimpUtil::deepCopy(node->child[k]));
                        }
                    }
                    // If term has only one factor, unwrap
                    if (term->child.size() == 1) {
                        Exptree* single = term->child[0];
                        term->child.clear();
                        SimpUtil::freeTree(term);
                        result->child.push_back(single);
                    } else {
                        result->child.push_back(term);
                    }
                }

                SimpUtil::freeTree(node);
                return expandNode(result);
            }
        }
        return node;
    }

    Exptree* TreeExpander::expandPow(Exptree* node) {
        // (a+b)^n using binomial theorem
        if (node->child.size() != 2) return node;

        Exptree* base = node->child[0];
        Exptree* exp = node->child[1];

        if (!SimpUtil::isFunction(base, "+")) return node;
        if (!SimpUtil::isInteger(exp) || !SimpUtil::isPositive(exp)) return node;

        Intg n = exp->value.numerator();
        if (n <= Intg(0)) return node;

        // (a + b)^n = sum(k=0..n) C(n,k) * a^(n-k) * b^k
        // For now, handle binomial (2-term sum) specifically
        if (base->child.size() != 2) {
            // More than 2 terms: use multinomial? Fall back to repeated multiplication
            Exptree* result = SimpUtil::deepCopy(base);
            for (Intg i(1); i < n; i = i + Intg(1)) {
                Exptree* prod = SimpUtil::makeFunction("*");
                prod->child.push_back(result);
                prod->child.push_back(SimpUtil::deepCopy(base));
                result = prod;
            }
            SimpUtil::freeTree(node);
            return expandNode(result);
        }

        Exptree* a = base->child[0];
        Exptree* b = base->child[1];

        // Compute binomial coefficients: C(n,k) = n!/(k!*(n-k)!)
        Intg nFac(1);
        for (Intg i(2); i <= n; i = i + Intg(1)) nFac = nFac * i;

        Exptree* result = SimpUtil::makeFunction("+");

        for (Intg k(0); k <= n; k = k + Intg(1)) {
            Intg nk = n - k;

            // C(n,k)
            Intg kFac(1);
            for (Intg i(2); i <= k; i = i + Intg(1)) kFac = kFac * i;
            Intg nkFac(1);
            for (Intg i(2); i <= nk; i = i + Intg(1)) nkFac = nkFac * i;
            Intg binom = nFac / (kFac * nkFac);

            // Build term: C(n,k) * a^(n-k) * b^k
            Exptree* term = SimpUtil::makeFunction("*");

            // Coefficient
            if (binom != Intg(1)) {
                term->child.push_back(SimpUtil::makeRational(Rational(binom)));
            }

            // a^(n-k)
            if (nk == Intg(0)) {
                // skip
            } else if (nk == Intg(1)) {
                term->child.push_back(SimpUtil::deepCopy(a));
            } else {
                Exptree* aPow = SimpUtil::makeFunction("^");
                aPow->child.push_back(SimpUtil::deepCopy(a));
                aPow->child.push_back(SimpUtil::makeRational(Rational(nk)));
                term->child.push_back(aPow);
            }

            // b^k
            if (k == Intg(0)) {
                // skip
            } else if (k == Intg(1)) {
                term->child.push_back(SimpUtil::deepCopy(b));
            } else {
                Exptree* bPow = SimpUtil::makeFunction("^");
                bPow->child.push_back(SimpUtil::deepCopy(b));
                bPow->child.push_back(SimpUtil::makeRational(Rational(k)));
                term->child.push_back(bPow);
            }

            // If term is empty (n=0,k=0), it's 1
            if (term->child.size() == 0) {
                term->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
            }
            // If term has only one factor, unwrap
            if (term->child.size() == 1) {
                Exptree* single = term->child[0];
                term->child.clear();
                SimpUtil::freeTree(term);
                result->child.push_back(single);
            } else {
                result->child.push_back(term);
            }
        }

        SimpUtil::freeTree(node);
        return expandNode(result);
    }

} // namespace CAS