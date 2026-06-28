/** @file /src/cas/simp/simp_coord.cpp
 *  @brief Coordinate transformation function simplifiers
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

    // ========== Polar ==========

    Exptree* TreeSimplifier::simplifyPolar(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* x = node->child[0];
        Exptree* y = node->child[1];

        // polar(0, 0) = 0 (magnitude 0)
        if (SimpUtil::isZero(x) && SimpUtil::isZero(y)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // polar(x, 0) = |x| (on real axis)
        if (SimpUtil::isZero(y)) {
            SimpUtil::freeTree(y);
            SimpUtil::freeTree(node);
            Exptree* absNode = SimpUtil::makeFunction(FuncName::abs);
            absNode->child.push_back(x);
            return simplifyAbs(absNode);
        }

        // polar(0, y) = |y| (on imaginary axis)
        if (SimpUtil::isZero(x)) {
            SimpUtil::freeTree(x);
            SimpUtil::freeTree(node);
            Exptree* absNode = SimpUtil::makeFunction(FuncName::abs);
            absNode->child.push_back(y);
            return simplifyAbs(absNode);
        }

        // polar(x, y) = sqrt(x^2 + y^2) for magnitude
        Exptree* xSq = SimpUtil::makeFunction("^");
        xSq->child.push_back(SimpUtil::deepCopy(x));
        xSq->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

        Exptree* ySq = SimpUtil::makeFunction("^");
        ySq->child.push_back(SimpUtil::deepCopy(y));
        ySq->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

        Exptree* sum = SimpUtil::makeFunction("+");
        sum->child.push_back(xSq);
        sum->child.push_back(ySq);

        Exptree* result = SimpUtil::makeFunction(FuncName::sqrt);
        result->child.push_back(sum);

        SimpUtil::freeTree(node);
        return simplifySqrt(result);
    }

    // ========== Rect ==========

    Exptree* TreeSimplifier::simplifyRect(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* r = node->child[0];
        Exptree* theta = node->child[1];

        // rect(0, theta) = 0
        if (SimpUtil::isZero(r)) {
            SimpUtil::freeTree(theta);
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // rect(r, 0) = r (pure real)
        if (SimpUtil::isZero(theta)) {
            SimpUtil::freeTree(theta);
            SimpUtil::freeTree(node);
            return r;
        }

        // rect(r, pi/2) = r*i (pure imaginary)
        if (SimpUtil::isFunction(theta, "*") && theta->child.size() >= 2) {
            bool hasPi = false;
            Rational coeff(Intg(0));
            for (size_t i = 0; i < theta->child.size(); ++i) {
                if (SimpUtil::isConstantPi(theta->child[i])) hasPi = true;
                else if (SimpUtil::isRational(theta->child[i]))
                    coeff = theta->child[i]->value;
            }
            if (hasPi && coeff == Rational(Intg(1), Intg(2))) {
                SimpUtil::freeTree(theta);
                SimpUtil::freeTree(node);
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(r);
                result->child.push_back(SimpUtil::makeVariable(ConstName::i));
                return simplifyMul(result);
            }
        }

        // rect(r, theta) -> (r*cos(theta), r*sin(theta))
        // Return as complex number: r*cos(theta) + i*r*sin(theta)
        Exptree* cosTerm = SimpUtil::makeFunction(FuncName::cos);
        cosTerm->child.push_back(SimpUtil::deepCopy(theta));

        Exptree* sinTerm = SimpUtil::makeFunction(FuncName::sin);
        sinTerm->child.push_back(SimpUtil::deepCopy(theta));

        Exptree* realPart = SimpUtil::makeFunction("*");
        realPart->child.push_back(SimpUtil::deepCopy(r));
        realPart->child.push_back(cosTerm);

        Exptree* imagPart = SimpUtil::makeFunction("*");
        imagPart->child.push_back(SimpUtil::deepCopy(r));
        imagPart->child.push_back(sinTerm);

        Exptree* imagWithI = SimpUtil::makeFunction("*");
        imagWithI->child.push_back(imagPart);
        imagWithI->child.push_back(SimpUtil::makeVariable(ConstName::i));

        Exptree* result = SimpUtil::makeFunction("+");
        result->child.push_back(realPart);
        result->child.push_back(imagWithI);

        SimpUtil::freeTree(node);
        return simplifyAdd(result);
    }

} // namespace CAS