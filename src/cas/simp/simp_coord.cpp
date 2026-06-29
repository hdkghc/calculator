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

        // polar(0, 0) -> vector(2, 0, 0)
        if (SimpUtil::isZero(x) && SimpUtil::isZero(y)) {
            SimpUtil::freeTree(node);
            Exptree* result = SimpUtil::makeFunction(FuncName::vector);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
            return result;
        }

        // polar(x, y) -> vector(2, r, theta)
        // r = sqrt(x^2 + y^2)
        Exptree* xSq = SimpUtil::makeFunction("^");
        xSq->child.push_back(SimpUtil::deepCopy(x));
        xSq->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

        Exptree* ySq = SimpUtil::makeFunction("^");
        ySq->child.push_back(SimpUtil::deepCopy(y));
        ySq->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

        Exptree* sum = SimpUtil::makeFunction("+");
        sum->child.push_back(xSq);
        sum->child.push_back(ySq);

        Exptree* rNode = SimpUtil::makeFunction(FuncName::sqrt);
        rNode->child.push_back(sum);
        rNode = simplifySqrt(rNode);

        // theta = atan(y/x), but handle x=0
        Exptree* thetaNode;
        if (SimpUtil::isZero(x)) {
            // x = 0: on y-axis
            if (SimpUtil::isZero(y)) {
                thetaNode = SimpUtil::makeRational(Rational(Intg(0)));
            } else if (SimpUtil::isPositive(y)) {
                thetaNode = SimpUtil::makeFunction("*");
                thetaNode->child.push_back(SimpUtil::makeRational(Rational(Intg(1), Intg(2))));
                thetaNode->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                thetaNode = simplifyMul(thetaNode);
            } else {
                thetaNode = SimpUtil::makeFunction("*");
                thetaNode->child.push_back(SimpUtil::makeRational(Rational(Intg(-1), Intg(2))));
                thetaNode->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                thetaNode = simplifyMul(thetaNode);
            }
        } else {
            // x != 0: theta = atan(y/x), then adjust for quadrants II and III
            Exptree* atanNode = SimpUtil::makeFunction(FuncName::atan);
            Exptree* div = SimpUtil::makeFunction("/");
            div->child.push_back(SimpUtil::deepCopy(y));
            div->child.push_back(SimpUtil::deepCopy(x));
            atanNode->child.push_back(div);
            atanNode = simplifyAtan(atanNode);

            if (SimpUtil::isNegative(x)) {
                // Quadrant II (y >= 0) or III (y < 0)
                if (SimpUtil::isPositive(y) || SimpUtil::isZero(y)) {
                    // Quadrant II: theta = atan(y/x) + pi
                    Exptree* sum = SimpUtil::makeFunction("+");
                    sum->child.push_back(atanNode);
                    sum->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                    thetaNode = simplifyAdd(sum);
                } else {
                    // Quadrant III: theta = atan(y/x) - pi
                    Exptree* diff = SimpUtil::makeFunction("+");
                    diff->child.push_back(atanNode);
                    Exptree* negPi = SimpUtil::makeFunction("*");
                    negPi->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                    negPi->child.push_back(SimpUtil::makeVariable(ConstName::pi));
                    diff->child.push_back(negPi);
                    thetaNode = simplifyAdd(diff);
                }
            } else {
                // Quadrant I (y >= 0) or IV (y < 0): atan(y/x) is correct
                thetaNode = atanNode;
            }
        }

        Exptree* result = SimpUtil::makeFunction(FuncName::vector);
        result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
        result->child.push_back(rNode);
        result->child.push_back(thetaNode);

        SimpUtil::freeTree(node);
        return result;
    }

    // ========== Rect ==========

    Exptree* TreeSimplifier::simplifyRect(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* r = node->child[0];
        Exptree* theta = node->child[1];

        // rect(r, theta) -> vector(2, x, y)
        // x = r*cos(theta), y = r*sin(theta)

        Exptree* cosTerm = SimpUtil::makeFunction(FuncName::cos);
        cosTerm->child.push_back(SimpUtil::deepCopy(theta));

        Exptree* sinTerm = SimpUtil::makeFunction(FuncName::sin);
        sinTerm->child.push_back(SimpUtil::deepCopy(theta));

        Exptree* xNode = SimpUtil::makeFunction("*");
        xNode->child.push_back(SimpUtil::deepCopy(r));
        xNode->child.push_back(cosTerm);
        xNode = simplifyMul(xNode);

        Exptree* yNode = SimpUtil::makeFunction("*");
        yNode->child.push_back(SimpUtil::deepCopy(r));
        yNode->child.push_back(sinTerm);
        yNode = simplifyMul(yNode);

        Exptree* result = SimpUtil::makeFunction(FuncName::vector);
        result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
        result->child.push_back(xNode);
        result->child.push_back(yNode);

        SimpUtil::freeTree(node);
        return result;
    }

} // namespace CAS