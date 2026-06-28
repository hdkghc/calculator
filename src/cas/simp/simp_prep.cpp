/** @file /src/cas/simp/simp_prep.cpp
 *  @brief Preprocessing transformations for expression tree simplification
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

    void TreeSimplifier::preTransform(Exptree*& node) {
        if (!node) return;

        for (size_t i = 0; i < node->child.size(); ++i) {
            preTransform(node->child[i]);
        }

        if (!SimpUtil::isFunction(node)) return;

        // ---- Subtraction a - b -> a + (-1)*b ----
        if (node->var == "-" && node->child.size() == 2) {
            Exptree* a = node->child[0];
            Exptree* b = node->child[1];
            Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
            Exptree* mul = SimpUtil::makeFunction("*");
            mul->child.push_back(negOne);
            mul->child.push_back(b);
            node->var = "+";
            node->child.clear();
            node->child.push_back(a);
            node->child.push_back(mul);
            return;
        }

        // ---- Unary minus -x -> (-1)*x ----
        if (node->var == "-" && node->child.size() == 1) {
            Exptree* arg = node->child[0];
            node->var = "*";
            node->child.clear();
            node->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            node->child.push_back(arg);
            return;
        }

        // ---- Division a/b -> a * b^(-1) ----
        if (node->var == "/" && node->child.size() == 2) {
            Exptree* a = node->child[0];
            Exptree* b = node->child[1];
            Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
            Exptree* powNode = SimpUtil::makeFunction("^");
            powNode->child.push_back(b);
            powNode->child.push_back(negOne);
            node->var = "*";
            node->child.clear();
            node->child.push_back(a);
            node->child.push_back(powNode);
            return;
        }

        // ---- sqrt(x) -> x^(1/2) ----
        if (node->var == FuncName::sqrt && node->child.size() == 1) {
            Exptree* arg = node->child[0];
            node->var = "^";
            node->child.clear();
            node->child.push_back(arg);
            node->child.push_back(SimpUtil::makeRational(Rational(Intg(1), Intg(2))));
            return;
        }

        // ---- root(a, b) = b^(1/a) ----
        if (node->var == FuncName::root && node->child.size() == 2) {
            Exptree* degree = node->child[0];
            Exptree* radicand = node->child[1];
            Exptree* negOne = SimpUtil::makeRational(Rational(Intg(-1)));
            Exptree* invDeg = SimpUtil::makeFunction("^");
            invDeg->child.push_back(degree);
            invDeg->child.push_back(negOne);
            node->var = "^";
            node->child.clear();
            node->child.push_back(radicand);
            node->child.push_back(invDeg);
            return;
        }

        // ---- exp(x) -> e^x ----
        if (node->var == FuncName::exp && node->child.size() == 1) {
            Exptree* arg = node->child[0];
            node->var = "^";
            node->child.clear();
            node->child.push_back(SimpUtil::makeVariable(ConstName::e));
            node->child.push_back(arg);
            return;
        }

        // ---- Golden ratio phi -> (sqrt(5) + 1) / 2 ----
        if (SimpUtil::isConstantPhi(node)) {
            Exptree* sqrt5 = SimpUtil::makeFunction(FuncName::sqrt);
            sqrt5->child.push_back(SimpUtil::makeRational(Rational(Intg(5))));
            Exptree* one = SimpUtil::makeRational(Rational(Intg(1)));
            Exptree* sum = SimpUtil::makeFunction("+");
            sum->child.push_back(sqrt5);
            sum->child.push_back(one);
            Exptree* two = SimpUtil::makeRational(Rational(Intg(2)));
            node->var = "/";
            node->child.clear();
            node->child.push_back(sum);
            node->child.push_back(two);
            return;
        }

        // ---- deg(x) -> (pi/180) * x ----
        if (node->var == FuncName::deg && node->child.size() == 1) {
            Exptree* arg = node->child[0];
            Exptree* piDiv180 = SimpUtil::makeRational(Rational(Intg(1), Intg(180)));
            Exptree* piMul = SimpUtil::makeFunction("*");
            piMul->child.push_back(piDiv180);
            piMul->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            node->var = "*";
            node->child.clear();
            node->child.push_back(piMul);
            node->child.push_back(arg);
            return;
        }

        // ---- rad(x) -> (180/pi) * x ----
        if (node->var == FuncName::rad && node->child.size() == 1) {
            Exptree* arg = node->child[0];
            Exptree* piInv = SimpUtil::makeFunction("^");
            piInv->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            piInv->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            Exptree* coeff = SimpUtil::makeRational(Rational(Intg(180)));
            Exptree* fullCoeff = SimpUtil::makeFunction("*");
            fullCoeff->child.push_back(coeff);
            fullCoeff->child.push_back(piInv);
            node->var = "*";
            node->child.clear();
            node->child.push_back(fullCoeff);
            node->child.push_back(arg);
            return;
        }

        // ---- sinh(x) -> (e^x - e^(-x)) / 2 ----
        if (node->var == FuncName::sinh && node->child.size() == 1) {
            Exptree* x = node->child[0];
            Exptree* e = SimpUtil::makeVariable(ConstName::e);

            Exptree* e_x = SimpUtil::makeFunction("^");
            e_x->child.push_back(SimpUtil::deepCopy(e));
            e_x->child.push_back(SimpUtil::deepCopy(x));

            Exptree* negX = SimpUtil::makeFunction("*");
            negX->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negX->child.push_back(SimpUtil::deepCopy(x));

            Exptree* e_negX = SimpUtil::makeFunction("^");
            e_negX->child.push_back(SimpUtil::deepCopy(e));
            e_negX->child.push_back(negX);

            Exptree* negE = SimpUtil::makeFunction("*");
            negE->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negE->child.push_back(e_negX);

            Exptree* diff = SimpUtil::makeFunction("+");
            diff->child.push_back(e_x);
            diff->child.push_back(negE);

            node->var = "/";
            node->child.clear();
            node->child.push_back(diff);
            node->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
            return;
        }

        // ---- cosh(x) -> (e^x + e^(-x)) / 2 ----
        if (node->var == FuncName::cosh && node->child.size() == 1) {
            Exptree* x = node->child[0];
            Exptree* e = SimpUtil::makeVariable(ConstName::e);

            Exptree* e_x = SimpUtil::makeFunction("^");
            e_x->child.push_back(SimpUtil::deepCopy(e));
            e_x->child.push_back(SimpUtil::deepCopy(x));

            Exptree* negX = SimpUtil::makeFunction("*");
            negX->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negX->child.push_back(SimpUtil::deepCopy(x));

            Exptree* e_negX = SimpUtil::makeFunction("^");
            e_negX->child.push_back(SimpUtil::deepCopy(e));
            e_negX->child.push_back(negX);

            Exptree* sum = SimpUtil::makeFunction("+");
            sum->child.push_back(e_x);
            sum->child.push_back(e_negX);

            node->var = "/";
            node->child.clear();
            node->child.push_back(sum);
            node->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
            return;
        }

        // ---- tanh(x) -> (e^x - e^(-x)) / (e^x + e^(-x)) ----
        if (node->var == FuncName::tanh && node->child.size() == 1) {
            Exptree* x = node->child[0];
            Exptree* e = SimpUtil::makeVariable(ConstName::e);

            Exptree* e_x = SimpUtil::makeFunction("^");
            e_x->child.push_back(SimpUtil::deepCopy(e));
            e_x->child.push_back(SimpUtil::deepCopy(x));

            Exptree* negX = SimpUtil::makeFunction("*");
            negX->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negX->child.push_back(SimpUtil::deepCopy(x));

            Exptree* e_negX = SimpUtil::makeFunction("^");
            e_negX->child.push_back(SimpUtil::deepCopy(e));
            e_negX->child.push_back(negX);

            Exptree* num = SimpUtil::makeFunction("+");
            num->child.push_back(SimpUtil::deepCopy(e_x));
            Exptree* negE_negX = SimpUtil::makeFunction("*");
            negE_negX->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negE_negX->child.push_back(e_negX);
            num->child.push_back(negE_negX);

            Exptree* den = SimpUtil::makeFunction("+");
            den->child.push_back(e_x);
            den->child.push_back(SimpUtil::deepCopy(e_negX));
            SimpUtil::freeTree(e_negX);

            node->var = "/";
            node->child.clear();
            node->child.push_back(num);
            node->child.push_back(den);
            return;
        }

        // ---- asinh(x) -> ln(x + sqrt(x^2 + 1)) ----
        if (node->var == FuncName::asinh && node->child.size() == 1) {
            Exptree* x = node->child[0];

            Exptree* xSq = SimpUtil::makeFunction("^");
            xSq->child.push_back(SimpUtil::deepCopy(x));
            xSq->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

            Exptree* one = SimpUtil::makeRational(Rational(Intg(1)));
            Exptree* sum = SimpUtil::makeFunction("+");
            sum->child.push_back(xSq);
            sum->child.push_back(one);

            Exptree* sqrtNode = SimpUtil::makeFunction(FuncName::sqrt);
            sqrtNode->child.push_back(sum);

            Exptree* sum2 = SimpUtil::makeFunction("+");
            sum2->child.push_back(SimpUtil::deepCopy(x));
            sum2->child.push_back(sqrtNode);

            node->var = FuncName::ln;
            node->child.clear();
            node->child.push_back(sum2);
            return;
        }

        // ---- acosh(x) -> ln(x + sqrt(x^2 - 1)) ----
        if (node->var == FuncName::acosh && node->child.size() == 1) {
            Exptree* x = node->child[0];

            Exptree* xSq = SimpUtil::makeFunction("^");
            xSq->child.push_back(SimpUtil::deepCopy(x));
            xSq->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

            Exptree* one = SimpUtil::makeRational(Rational(Intg(1)));

            Exptree* diff = SimpUtil::makeFunction("+");
            diff->child.push_back(xSq);
            Exptree* negOne = SimpUtil::makeFunction("*");
            negOne->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negOne->child.push_back(SimpUtil::deepCopy(one));
            diff->child.push_back(negOne);

            Exptree* sqrtNode = SimpUtil::makeFunction(FuncName::sqrt);
            sqrtNode->child.push_back(diff);

            Exptree* sum2 = SimpUtil::makeFunction("+");
            sum2->child.push_back(SimpUtil::deepCopy(x));
            sum2->child.push_back(sqrtNode);

            node->var = FuncName::ln;
            node->child.clear();
            node->child.push_back(sum2);
            return;
        }

        // ---- atanh(x) -> (1/2) * ln((1+x)/(1-x)) ----
        if (node->var == FuncName::atanh && node->child.size() == 1) {
            Exptree* x = node->child[0];
            Exptree* one = SimpUtil::makeRational(Rational(Intg(1)));

            Exptree* num = SimpUtil::makeFunction("+");
            num->child.push_back(SimpUtil::deepCopy(one));
            num->child.push_back(SimpUtil::deepCopy(x));

            Exptree* negX = SimpUtil::makeFunction("*");
            negX->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negX->child.push_back(SimpUtil::deepCopy(x));

            Exptree* den = SimpUtil::makeFunction("+");
            den->child.push_back(SimpUtil::deepCopy(one));
            den->child.push_back(negX);

            Exptree* div = SimpUtil::makeFunction("/");
            div->child.push_back(num);
            div->child.push_back(den);

            Exptree* lnNode = SimpUtil::makeFunction(FuncName::ln);
            lnNode->child.push_back(div);

            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            node->var = "*";
            node->child.clear();
            node->child.push_back(half);
            node->child.push_back(lnNode);
            return;
        }

        // ---- Flatten nested same-operator expressions ----
        if (node->var == "+" || node->var == "*") {
            std::vector<Exptree*> newChildren;
            for (size_t i = 0; i < node->child.size(); ++i) {
                Exptree* child = node->child[i];
                if (SimpUtil::isFunction(child) && child->var == node->var) {
                    for (size_t j = 0; j < child->child.size(); ++j) {
                        newChildren.push_back(child->child[j]);
                    }
                    child->child.clear();
                    SimpUtil::freeTree(child);
                } else {
                    newChildren.push_back(child);
                }
            }
            node->child = std::move(newChildren);
        }
    }

} // namespace CAS