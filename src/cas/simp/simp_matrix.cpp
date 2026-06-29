/** @file /src/cas/simp/simp_matrix.cpp
 *  @brief Vector and matrix function simplifiers
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
#include "cas/simp/simputil.hpp"

namespace CAS {

    // ========== Helper for Determinant ==========

    /**
     *  @brief Recursively compute determinant via Laplace expansion
     *  @param mat Matrix node (must be square, all rational)
     *  @param n Dimension
     *  @param nCols Number of columns in mat (for indexing)
     *  @return Determinant as Rational
     */
    static Rational detRecursive(Exptree* mat, size_t n, size_t nCols) {
        if (n == 1) {
            return SimpUtil::matrixElem(mat, 0, 0, nCols)->value;
        }
        if (n == 2) {
            Rational a = SimpUtil::matrixElem(mat, 0, 0, nCols)->value;
            Rational b = SimpUtil::matrixElem(mat, 0, 1, nCols)->value;
            Rational c = SimpUtil::matrixElem(mat, 1, 0, nCols)->value;
            Rational d = SimpUtil::matrixElem(mat, 1, 1, nCols)->value;
            return a * d - b * c;
        }

        // Laplace expansion along first row
        Rational det(Intg(0));
        int sign = 1;  // +1 for j=0, then alternates

        for (size_t j = 0; j < n; ++j) {
            Rational a0j = SimpUtil::matrixElem(mat, 0, j, nCols)->value;

            if (!a0j.isZero()) {
                // Build submatrix of size (n-1) x (n-1)
                // submatrix elements: rows 1..n-1, cols 0..n except j
                Exptree* sub = SimpUtil::makeFunction(FuncName::matrix);
                sub->child.push_back(SimpUtil::makeRational(Rational(Intg((int)(n-1)))));
                sub->child.push_back(SimpUtil::makeRational(Rational(Intg((int)(n-1)))));

                for (size_t i = 1; i < n; ++i) {
                    for (size_t k = 0; k < n; ++k) {
                        if (k == j) continue;
                        Exptree* elem = SimpUtil::matrixElem(mat, i, k, nCols);
                        sub->child.push_back(SimpUtil::deepCopy(elem));
                    }
                }

                Rational subDet = detRecursive(sub, n - 1, n - 1);
                Rational term = a0j * subDet;

                if (sign == 1) {
                    det = det + term;
                } else {
                    det = det - term;
                }

                SimpUtil::freeTree(sub);
            }

            sign = -sign;
        }

        return det;
    }

    // ========== Vector ==========

    Exptree* TreeSimplifier::simplifyVector(Exptree* node) {
        if (node->child.size() < 1) return node;

        // Check if first child is an integer (normal form)
        if (SimpUtil::isInteger(node->child[0])) {
            size_t expectedN = (size_t)node->child[0]->value.numerator().toInt();
            if (node->child.size() == expectedN + 1) {
                return node;
            }
        }

        // Conversion cases: vector(single_arg)
        if (node->child.size() == 1) {
            Exptree* arg = node->child[0];

            // vector(matrix) -> flatten
            if (SimpUtil::isMatrixNode(arg)) {
                size_t m = 0, n = 0;
                if (SimpUtil::matrixDims(arg, m, n)) {
                    Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                    size_t total = m * n;
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)total))));
                    for (size_t i = 0; i < m; ++i) {
                        for (size_t j = 0; j < n; ++j) {
                            result->child.push_back(
                                SimpUtil::deepCopy(SimpUtil::matrixElem(arg, i, j, n)));
                        }
                    }
                    SimpUtil::freeTree(node);
                    return result;
                }
            }

            // vector(a + b*i) -> [real, imag]
            Exptree* realPart = nullptr;
            Exptree* imagPart = nullptr;
            if (splitComplexSum(*this, arg, realPart, imagPart)) {
                Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                result->child.push_back(realPart);
                result->child.push_back(imagPart);
                SimpUtil::freeTree(node);
                return result;
            }

            // vector(i) -> [0, 1]
            if (SimpUtil::isConstantI(arg)) {
                Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                SimpUtil::freeTree(node);
                SimpUtil::freeTree(arg);
                return result;
            }

            // vector(-i) -> [0, -1]
            if (SimpUtil::isFunction(arg, "*") && arg->child.size() == 2) {
                if (SimpUtil::isMinusOne(arg->child[0]) && SimpUtil::isConstantI(arg->child[1])) {
                    Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                    SimpUtil::freeTree(node);
                    return result;
                }
            }

            // vector(c * i) -> [0, c]
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
                    Exptree* c;
                    if (coeff->child.size() == 1) {
                        c = coeff->child[0];
                        coeff->child.clear();
                        SimpUtil::freeTree(coeff);
                    } else if (coeff->child.size() == 0) {
                        SimpUtil::freeTree(coeff);
                        c = SimpUtil::makeRational(Rational(Intg(1)));
                    } else {
                        c = simplifyMul(coeff);
                    }

                    Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
                    result->child.push_back(c);
                    SimpUtil::freeTree(node);
                    return result;
                }
                SimpUtil::freeTree(coeff);
            }
        }

        return node;
    }

    // ========== Dot ==========

    Exptree* TreeSimplifier::simplifyDot(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* a = node->child[0];
        Exptree* b = node->child[1];

        if (!SimpUtil::isVectorNode(a) || !SimpUtil::isVectorNode(b)) return node;

        size_t sizeA = SimpUtil::vectorElemCount(a);
        size_t sizeB = SimpUtil::vectorElemCount(b);

        if (sizeA != sizeB || sizeA == 0) return node;

        bool allRational = true;
        for (size_t i = 0; i < sizeA && allRational; ++i) {
            Exptree* ea = SimpUtil::vectorElem(a, i);
            Exptree* eb = SimpUtil::vectorElem(b, i);
            if (!SimpUtil::isRational(ea) || !SimpUtil::isRational(eb))
                allRational = false;
        }

        if (allRational) {
            Rational sum(Intg(0));
            for (size_t i = 0; i < sizeA; ++i) {
                sum = sum + SimpUtil::vectorElem(a, i)->value * SimpUtil::vectorElem(b, i)->value;
            }
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(sum);
        }

        Exptree* result = SimpUtil::makeFunction("+");
        for (size_t i = 0; i < sizeA; ++i) {
            Exptree* prod = SimpUtil::makeFunction("*");
            prod->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, i)));
            prod->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, i)));
            result->child.push_back(prod);
        }
        SimpUtil::freeTree(node);
        return simplifyAdd(result);
    }

    // ========== Angle ==========

    Exptree* TreeSimplifier::simplifyAngle(Exptree* node) {
        if (node->child.size() != 2) return node;

        Exptree* a = node->child[0];
        Exptree* b = node->child[1];

        if (!SimpUtil::isVectorNode(a) || !SimpUtil::isVectorNode(b)) return node;

        size_t sizeA = SimpUtil::vectorElemCount(a);
        size_t sizeB = SimpUtil::vectorElemCount(b);

        if (sizeA != sizeB || sizeA == 0) return node;

        Exptree* dotNode = SimpUtil::makeFunction(FuncName::dot);
        dotNode->child.push_back(SimpUtil::deepCopy(a));
        dotNode->child.push_back(SimpUtil::deepCopy(b));
        dotNode = simplifyDot(dotNode);

        Exptree* magA2 = SimpUtil::makeFunction(FuncName::dot);
        magA2->child.push_back(SimpUtil::deepCopy(a));
        magA2->child.push_back(SimpUtil::deepCopy(a));
        magA2 = simplifyDot(magA2);

        Exptree* magA = SimpUtil::makeFunction(FuncName::sqrt);
        magA->child.push_back(magA2);
        magA = simplifySqrt(magA);

        Exptree* magB2 = SimpUtil::makeFunction(FuncName::dot);
        magB2->child.push_back(SimpUtil::deepCopy(b));
        magB2->child.push_back(SimpUtil::deepCopy(b));
        magB2 = simplifyDot(magB2);

        Exptree* magB = SimpUtil::makeFunction(FuncName::sqrt);
        magB->child.push_back(magB2);
        magB = simplifySqrt(magB);

        Exptree* magProd = SimpUtil::makeFunction("*");
        magProd->child.push_back(magA);
        magProd->child.push_back(magB);
        magProd = simplifyMul(magProd);

        Exptree* div = SimpUtil::makeFunction("/");
        div->child.push_back(dotNode);
        div->child.push_back(magProd);

        Exptree* acosNode = SimpUtil::makeFunction(FuncName::acos);
        acosNode->child.push_back(div);

        SimpUtil::freeTree(node);
        return simplifyAcos(acosNode);
    }

    // ========== Matrix ==========

    Exptree* TreeSimplifier::simplifyMatrix(Exptree* node) {
        if (node->child.size() < 1) return node;

        if (node->child.size() >= 2 &&
            SimpUtil::isInteger(node->child[0]) && SimpUtil::isInteger(node->child[1])) {
            size_t m = (size_t)node->child[0]->value.numerator().toInt();
            size_t n = (size_t)node->child[1]->value.numerator().toInt();
            if (node->child.size() == 2 + m * n) {
                return node;
            }
        }

        if (node->child.size() == 1 && SimpUtil::isVectorNode(node->child[0])) {
            Exptree* vec = node->child[0];
            size_t sz = SimpUtil::vectorElemCount(vec);

            Exptree* result = SimpUtil::makeFunction(FuncName::matrix);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)sz))));
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));

            for (size_t i = 0; i < sz; ++i) {
                result->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(vec, i)));
            }

            SimpUtil::freeTree(node);
            return result;
        }

        return node;
    }

    // ========== Det (recursive Laplace expansion) ==========

    Exptree* TreeSimplifier::simplifyDet(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* mat = node->child[0];

        size_t m = 0, n = 0;
        if (!SimpUtil::matrixDims(mat, m, n)) return node;
        if (m != n || m == 0) return node;

        // Check if all elements are rational
        bool allRational = true;
        for (size_t i = 0; i < m && allRational; ++i) {
            for (size_t j = 0; j < n && allRational; ++j) {
                if (!SimpUtil::isRational(SimpUtil::matrixElem(mat, i, j, n)))
                    allRational = false;
            }
        }

        if (!allRational) return node;

        // Compute determinant recursively
        Rational det = detRecursive(mat, n, n);

        SimpUtil::freeTree(node);
        return SimpUtil::makeRational(det);
    }

    // ========== Transpose ==========

    Exptree* TreeSimplifier::simplifyTranspose(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* mat = node->child[0];

        size_t m = 0, n = 0;
        if (!SimpUtil::matrixDims(mat, m, n)) return node;

        Exptree* result = SimpUtil::makeFunction(FuncName::matrix);
        result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)n))));
        result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)m))));

        for (size_t j = 0; j < n; ++j) {
            for (size_t i = 0; i < m; ++i) {
                result->child.push_back(
                    SimpUtil::deepCopy(SimpUtil::matrixElem(mat, i, j, n)));
            }
        }

        SimpUtil::freeTree(node);
        return result;
    }

    // ========== Vector/Matrix Addition ==========

    Exptree* TreeSimplifier::simplifyAddVM(Exptree* node) {
        if (node->child.size() == 0) return node;

        if (SimpUtil::isVectorNode(node->child[0])) {
            size_t sz = SimpUtil::vectorElemCount(node->child[0]);
            for (size_t i = 1; i < node->child.size(); ++i) {
                if (!SimpUtil::isVectorNode(node->child[i]) ||
                    SimpUtil::vectorElemCount(node->child[i]) != sz)
                    return node;
            }

            Exptree* result = SimpUtil::makeFunction(FuncName::vector);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)sz))));

            for (size_t j = 0; j < sz; ++j) {
                Exptree* sum = SimpUtil::makeFunction("+");
                for (size_t i = 0; i < node->child.size(); ++i) {
                    sum->child.push_back(
                        SimpUtil::deepCopy(SimpUtil::vectorElem(node->child[i], j)));
                }
                sum = simplifyAdd(sum);
                result->child.push_back(sum);
            }

            SimpUtil::freeTree(node);
            return result;
        }

        if (SimpUtil::isMatrixNode(node->child[0])) {
            size_t m = 0, n = 0;
            if (!SimpUtil::matrixDims(node->child[0], m, n)) return node;
            for (size_t i = 1; i < node->child.size(); ++i) {
                size_t mi = 0, ni = 0;
                if (!SimpUtil::matrixDims(node->child[i], mi, ni)) return node;
                if (mi != m || ni != n) return node;
            }

            Exptree* result = SimpUtil::makeFunction(FuncName::matrix);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)m))));
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)n))));

            for (size_t r = 0; r < m; ++r) {
                for (size_t c = 0; c < n; ++c) {
                    Exptree* sum = SimpUtil::makeFunction("+");
                    for (size_t i = 0; i < node->child.size(); ++i) {
                        sum->child.push_back(
                            SimpUtil::deepCopy(SimpUtil::matrixElem(node->child[i], r, c, n)));
                    }
                    sum = simplifyAdd(sum);
                    result->child.push_back(sum);
                }
            }

            SimpUtil::freeTree(node);
            return result;
        }

        return node;
    }

    // ========== Vector/Matrix Multiplication ==========

    Exptree* TreeSimplifier::simplifyMulVM(Exptree* node) {
        if (node->child.size() == 0) return node;

        std::vector<Exptree*> scalars;
        std::vector<Exptree*> vectors;
        std::vector<Exptree*> matrices;

        for (size_t i = 0; i < node->child.size(); ++i) {
            Exptree* c = node->child[i];
            if (SimpUtil::isMatrixNode(c)) matrices.push_back(c);
            else if (SimpUtil::isVectorNode(c)) vectors.push_back(c);
            else scalars.push_back(c);
        }

        Exptree* scalarProd = nullptr;
        if (scalars.size() > 0) {
            scalarProd = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < scalars.size(); ++i) {
                scalarProd->child.push_back(SimpUtil::deepCopy(scalars[i]));
            }
            scalarProd = simplifyMul(scalarProd);
        }

        if (vectors.empty() && matrices.empty()) {
            if (scalarProd) SimpUtil::freeTree(scalarProd);
            return node;
        }

        // Case 1: Vector * Vector (2D cross=scalar, 3D cross=vector, other=dot)
        if (vectors.size() == 2 && matrices.empty()) {
            Exptree* a = vectors[0];
            Exptree* b = vectors[1];
            size_t szA = SimpUtil::vectorElemCount(a);
            size_t szB = SimpUtil::vectorElemCount(b);

            if (szA == szB) {
                if (szA == 2) {
                    Exptree* prod1 = SimpUtil::makeFunction("*");
                    prod1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 0)));
                    prod1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 1)));

                    Exptree* prod2 = SimpUtil::makeFunction("*");
                    prod2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 1)));
                    prod2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 0)));

                    Exptree* diff = SimpUtil::makeFunction("+");
                    diff->child.push_back(prod1);
                    Exptree* negProd2 = SimpUtil::makeFunction("*");
                    negProd2->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                    negProd2->child.push_back(prod2);
                    diff->child.push_back(negProd2);
                    diff = simplifyAdd(diff);

                    // Return 3D vector (0, 0, diff)
                    Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(0))));
                    result->child.push_back(diff);

                    if (scalarProd) {
                        for (size_t i = 1; i < result->child.size(); ++i) {
                            Exptree* scaled = SimpUtil::makeFunction("*");
                            scaled->child.push_back(SimpUtil::deepCopy(scalarProd));
                            scaled->child.push_back(result->child[i]);
                            result->child[i] = simplifyMul(scaled);
                        }
                    }

                    if (scalarProd) SimpUtil::freeTree(scalarProd);
                    SimpUtil::freeTree(node);
                    return result;
                }

                if (szA == 3) {
                    Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));

                    Exptree* x1 = SimpUtil::makeFunction("*");
                    x1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 1)));
                    x1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 2)));
                    Exptree* x2 = SimpUtil::makeFunction("*");
                    x2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 2)));
                    x2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 1)));
                    Exptree* x = SimpUtil::makeFunction("+");
                    x->child.push_back(x1);
                    Exptree* negX2 = SimpUtil::makeFunction("*");
                    negX2->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                    negX2->child.push_back(x2);
                    x->child.push_back(negX2);
                    result->child.push_back(simplifyAdd(x));

                    Exptree* y1 = SimpUtil::makeFunction("*");
                    y1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 2)));
                    y1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 0)));
                    Exptree* y2 = SimpUtil::makeFunction("*");
                    y2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 0)));
                    y2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 2)));
                    Exptree* y = SimpUtil::makeFunction("+");
                    y->child.push_back(y1);
                    Exptree* negY2 = SimpUtil::makeFunction("*");
                    negY2->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                    negY2->child.push_back(y2);
                    y->child.push_back(negY2);
                    result->child.push_back(simplifyAdd(y));

                    Exptree* z1 = SimpUtil::makeFunction("*");
                    z1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 0)));
                    z1->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 1)));
                    Exptree* z2 = SimpUtil::makeFunction("*");
                    z2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(a, 1)));
                    z2->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(b, 0)));
                    Exptree* z = SimpUtil::makeFunction("+");
                    z->child.push_back(z1);
                    Exptree* negZ2 = SimpUtil::makeFunction("*");
                    negZ2->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                    negZ2->child.push_back(z2);
                    z->child.push_back(negZ2);
                    result->child.push_back(simplifyAdd(z));

                    if (scalarProd) {
                        for (size_t i = 1; i < result->child.size(); ++i) {
                            Exptree* scaled = SimpUtil::makeFunction("*");
                            scaled->child.push_back(SimpUtil::deepCopy(scalarProd));
                            scaled->child.push_back(result->child[i]);
                            result->child[i] = simplifyMul(scaled);
                        }
                    }

                    if (scalarProd) SimpUtil::freeTree(scalarProd);
                    SimpUtil::freeTree(node);
                    return result;
                }

                Exptree* dotResult = SimpUtil::makeFunction(FuncName::dot);
                dotResult->child.push_back(SimpUtil::deepCopy(a));
                dotResult->child.push_back(SimpUtil::deepCopy(b));
                dotResult = simplifyDot(dotResult);

                if (scalarProd) {
                    Exptree* scaled = SimpUtil::makeFunction("*");
                    scaled->child.push_back(SimpUtil::deepCopy(scalarProd));
                    scaled->child.push_back(dotResult);
                    dotResult = simplifyMul(scaled);
                }

                if (scalarProd) SimpUtil::freeTree(scalarProd);
                SimpUtil::freeTree(node);
                return dotResult;
            }
        }

        // Case 2: Scalar * vector
        if (vectors.size() == 1 && matrices.empty()) {
            Exptree* vec = vectors[0];
            size_t sz = SimpUtil::vectorElemCount(vec);

            Exptree* result = SimpUtil::makeFunction(FuncName::vector);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)sz))));

            for (size_t j = 0; j < sz; ++j) {
                if (scalarProd) {
                    Exptree* prod = SimpUtil::makeFunction("*");
                    prod->child.push_back(SimpUtil::deepCopy(scalarProd));
                    prod->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(vec, j)));
                    prod = simplifyMul(prod);
                    result->child.push_back(prod);
                } else {
                    result->child.push_back(SimpUtil::deepCopy(SimpUtil::vectorElem(vec, j)));
                }
            }

            if (scalarProd) SimpUtil::freeTree(scalarProd);
            SimpUtil::freeTree(node);
            return result;
        }

        // Case 3: Scalar * matrix
        if (vectors.empty() && matrices.size() == 1) {
            Exptree* mat = matrices[0];
            size_t m = 0, n = 0;
            SimpUtil::matrixDims(mat, m, n);

            Exptree* result = SimpUtil::makeFunction(FuncName::matrix);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)m))));
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)n))));

            for (size_t r = 0; r < m; ++r) {
                for (size_t c = 0; c < n; ++c) {
                    if (scalarProd) {
                        Exptree* prod = SimpUtil::makeFunction("*");
                        prod->child.push_back(SimpUtil::deepCopy(scalarProd));
                        prod->child.push_back(SimpUtil::deepCopy(SimpUtil::matrixElem(mat, r, c, n)));
                        prod = simplifyMul(prod);
                        result->child.push_back(prod);
                    } else {
                        result->child.push_back(SimpUtil::deepCopy(SimpUtil::matrixElem(mat, r, c, n)));
                    }
                }
            }

            if (scalarProd) SimpUtil::freeTree(scalarProd);
            SimpUtil::freeTree(node);
            return result;
        }

        // Case 4: Matrix * Matrix
        if (vectors.empty() && matrices.size() == 2) {
            Exptree* A = matrices[0];
            Exptree* B = matrices[1];
            size_t mA = 0, nA = 0, mB = 0, nB = 0;
            if (!SimpUtil::matrixDims(A, mA, nA) || !SimpUtil::matrixDims(B, mB, nB)) {
                if (scalarProd) SimpUtil::freeTree(scalarProd);
                return node;
            }
            if (nA != mB) {
                if (scalarProd) SimpUtil::freeTree(scalarProd);
                return node;
            }

            Exptree* result = SimpUtil::makeFunction(FuncName::matrix);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)mA))));
            result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)nB))));

            for (size_t r = 0; r < mA; ++r) {
                for (size_t c = 0; c < nB; ++c) {
                    Exptree* sum = SimpUtil::makeFunction("+");
                    for (size_t k = 0; k < nA; ++k) {
                        Exptree* prod = SimpUtil::makeFunction("*");
                        prod->child.push_back(SimpUtil::deepCopy(SimpUtil::matrixElem(A, r, k, nA)));
                        prod->child.push_back(SimpUtil::deepCopy(SimpUtil::matrixElem(B, k, c, nB)));
                        prod = simplifyMul(prod);
                        sum->child.push_back(prod);
                    }
                    sum = simplifyAdd(sum);

                    if (scalarProd) {
                        Exptree* scaled = SimpUtil::makeFunction("*");
                        scaled->child.push_back(SimpUtil::deepCopy(scalarProd));
                        scaled->child.push_back(sum);
                        sum = simplifyMul(scaled);
                    }
                    result->child.push_back(sum);
                }
            }

            if (scalarProd) SimpUtil::freeTree(scalarProd);
            SimpUtil::freeTree(node);
            return result;
        }

        if (scalarProd) SimpUtil::freeTree(scalarProd);
        return node;
    }

    // ========== Normalize ==========

    Exptree* TreeSimplifier::simplifyNormalize(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        if (!SimpUtil::isVectorNode(arg)) return node;

        size_t sz = SimpUtil::vectorElemCount(arg);
        if (sz == 0) return node;

        // normalize(v) = v / abs(v) = v * abs(v)^(-1)
        Exptree* absV = SimpUtil::makeFunction(FuncName::abs);
        absV->child.push_back(SimpUtil::deepCopy(arg));
        absV = simplifyAbs(absV);

        Exptree* invAbs = SimpUtil::makeFunction("^");
        invAbs->child.push_back(absV);
        invAbs->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));

        Exptree* result = SimpUtil::makeFunction("*");
        result->child.push_back(SimpUtil::deepCopy(arg));
        result->child.push_back(invAbs);

        SimpUtil::freeTree(node);
        return simplifyMul(result);
    }

    Exptree* TreeSimplifier::simplifyRank(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* mat = node->child[0];

        size_t m = 0, n = 0;
        if (!SimpUtil::matrixDims(mat, m, n)) return node;

        // Check all rational
        bool allRational = true;
        for (size_t i = 0; i < m && allRational; ++i)
            for (size_t j = 0; j < n && allRational; ++j)
                if (!SimpUtil::isRational(SimpUtil::matrixElem(mat, i, j, n)))
                    allRational = false;
        if (!allRational) return node;

        // Copy matrix to local array
        std::vector<std::vector<Rational>> A(m, std::vector<Rational>(n));
        for (size_t i = 0; i < m; ++i)
            for (size_t j = 0; j < n; ++j)
                A[i][j] = SimpUtil::matrixElem(mat, i, j, n)->value;

        // Gaussian elimination
        size_t rank = 0;
        size_t col = 0;
        for (size_t row = 0; row < m && col < n; ++col) {
            // Find pivot
            size_t pivot = row;
            while (pivot < m && A[pivot][col].isZero()) pivot++;
            if (pivot == m) continue;

            std::swap(A[row], A[pivot]);
            rank++;

            // Eliminate below
            for (size_t i = row + 1; i < m; ++i) {
                if (!A[i][col].isZero()) {
                    Rational factor = A[i][col] / A[row][col];
                    for (size_t j = col; j < n; ++j)
                        A[i][j] = A[i][j] - factor * A[row][j];
                }
            }
            row++;
        }

        SimpUtil::freeTree(node);
        return SimpUtil::makeRational(Rational(Intg(rank)));
    }
    
    // Get characteristic polynomial coefficients via Faddeev-LeVerrier
    // coeffs[0] = 1, p(λ) = coeffs[0]·λ^n + coeffs[1]·λ^(n-1) + ... + coeffs[n]
    static std::vector<Rational> eigenvalCoeffs(Exptree* mat, size_t m, size_t n) {
        std::vector<Rational> coeffs(m + 1);
        coeffs[0] = Rational(Intg(1));
        
        std::vector<std::vector<Rational>> A(m, std::vector<Rational>(m));
        for (size_t i = 0; i < m; ++i)
            for (size_t j = 0; j < m; ++j)
                A[i][j] = SimpUtil::matrixElem(mat, i, j, n)->value;
        
        auto B = A;
        for (size_t k = 1; k <= m; ++k) {
            Rational trace(Intg(0));
            for (size_t i = 0; i < m; ++i) trace = trace + B[i][i];
            coeffs[k] = Rational(Intg(0)) - trace / Rational(Intg((int)k));
            
            if (k < m) {
                std::vector<std::vector<Rational>> C(m, std::vector<Rational>(m));
                for (size_t i = 0; i < m; ++i)
                    for (size_t j = 0; j < m; ++j) {
                        C[i][j] = B[i][j];
                        if (i == j) C[i][j] = C[i][j] + coeffs[k];
                    }
                std::vector<std::vector<Rational>> newB(m, std::vector<Rational>(m));
                for (size_t i = 0; i < m; ++i)
                    for (size_t j = 0; j < m; ++j)
                        for (size_t p = 0; p < m; ++p)
                            newB[i][j] = newB[i][j] + A[i][p] * C[p][j];
                B = newB;
            }
        }
        return coeffs;
    }

    // ========== Eigenval ==========

    Exptree* TreeSimplifier::simplifyEigenval(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* mat = node->child[0];
        size_t m = 0, n = 0;
        if (!SimpUtil::matrixDims(mat, m, n)) return node;
        if (m != n || m == 0) return node;

        bool allRational = true;
        for (size_t i = 0; i < m && allRational; ++i)
            for (size_t j = 0; j < n && allRational; ++j)
                if (!SimpUtil::isRational(SimpUtil::matrixElem(mat, i, j, n)))
                    allRational = false;
        if (!allRational) return node;

        auto coeffs = eigenvalCoeffs(mat, m, n);
        Exptree* result = SimpUtil::makeFunction(FuncName::vector);

        // n = 1
        if (m == 1) {
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
            result->child.push_back(SimpUtil::deepCopy(SimpUtil::matrixElem(mat, 0, 0, n)));
            SimpUtil::freeTree(node);
            return result;
        }

        // n = 2: quadratic formula
        if (m == 2) {
            Rational c1 = coeffs[1], c2 = coeffs[2];
            Rational disc = c1*c1 - Rational(Intg(4))*c2;
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
            Exptree* sqrtDisc = SimpUtil::makeFunction(FuncName::sqrt);
            sqrtDisc->child.push_back(SimpUtil::makeRational(disc));
            sqrtDisc = simplifySqrt(sqrtDisc);
            Exptree* negC1_2 = SimpUtil::makeRational(Rational(Intg(0)) - c1 * Rational(Intg(1), Intg(2)));
            Exptree* lambda1 = SimpUtil::makeFunction("+");
            lambda1->child.push_back(SimpUtil::deepCopy(negC1_2));
            Exptree* halfSqrt = SimpUtil::makeFunction("*");
            halfSqrt->child.push_back(SimpUtil::makeRational(Rational(Intg(1), Intg(2))));
            halfSqrt->child.push_back(SimpUtil::deepCopy(sqrtDisc));
            lambda1->child.push_back(halfSqrt);
            Exptree* lambda2 = SimpUtil::makeFunction("+");
            lambda2->child.push_back(SimpUtil::deepCopy(negC1_2));
            Exptree* negHalfSqrt = SimpUtil::makeFunction("*");
            negHalfSqrt->child.push_back(SimpUtil::makeRational(Rational(Intg(-1), Intg(2))));
            negHalfSqrt->child.push_back(sqrtDisc);
            lambda2->child.push_back(negHalfSqrt);
            result->child.push_back(lambda1);
            result->child.push_back(lambda2);
            SimpUtil::freeTree(node);
            return result;
        }

        // Fast path: check if matrix is triangular (all zeros below/above diagonal)
        // If so, eigenvalues are just the diagonal entries
        if (m == 3 || m == 4) {
            bool isUpperTriangular = true;
            bool isLowerTriangular = true;
            for (size_t i = 0; i < m && (isUpperTriangular || isLowerTriangular); ++i) {
                for (size_t j = 0; j < m; ++j) {
                    if (i > j && !SimpUtil::matrixElem(mat, i, j, n)->value.isZero())
                        isUpperTriangular = false;
                    if (i < j && !SimpUtil::matrixElem(mat, i, j, n)->value.isZero())
                        isLowerTriangular = false;
                }
            }
            if (isUpperTriangular || isLowerTriangular) {
                result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)m))));
                for (size_t i = 0; i < m; ++i)
                    result->child.push_back(SimpUtil::deepCopy(SimpUtil::matrixElem(mat, i, i, n)));
                SimpUtil::freeTree(node);
                return result;
            }
        }

        // n = 3: Cardano formula
        if (m == 3) {
            Rational c1 = coeffs[1], c2 = coeffs[2], c3 = coeffs[3];
            Rational oneThird(Intg(1), Intg(3));
            Rational c1_3 = c1 * oneThird;
            Rational p = c2 - c1 * c1_3;
            Rational q = Rational(Intg(2)) * c1_3 * c1_3 * c1_3 - c1_3 * c2 + c3;
            q = Rational(Intg(0)) - q;
            Rational disc = q*q / Rational(Intg(4)) + p*p*p / Rational(Intg(27));

            result->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));

            Exptree* oneThirdNode = SimpUtil::makeRational(oneThird);
            Exptree* negQHalf = SimpUtil::makeRational(Rational(Intg(0)) - q * Rational(Intg(1), Intg(2)));
            Exptree* sqrtDisc = SimpUtil::makeFunction(FuncName::sqrt);
            sqrtDisc->child.push_back(SimpUtil::makeRational(disc));

            Exptree* u_inner = SimpUtil::makeFunction("+");
            u_inner->child.push_back(SimpUtil::deepCopy(negQHalf));
            u_inner->child.push_back(SimpUtil::deepCopy(sqrtDisc));
            Exptree* u = SimpUtil::makeFunction("^");
            u->child.push_back(u_inner);
            u->child.push_back(SimpUtil::deepCopy(oneThirdNode));

            Exptree* negSqrtDisc = SimpUtil::makeFunction("*");
            negSqrtDisc->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negSqrtDisc->child.push_back(SimpUtil::deepCopy(sqrtDisc));
            Exptree* v_inner = SimpUtil::makeFunction("+");
            v_inner->child.push_back(SimpUtil::deepCopy(negQHalf));
            v_inner->child.push_back(negSqrtDisc);
            Exptree* v = SimpUtil::makeFunction("^");
            v->child.push_back(v_inner);
            v->child.push_back(SimpUtil::deepCopy(oneThirdNode));

            Exptree* lambda1 = SimpUtil::makeFunction("+");
            lambda1->child.push_back(SimpUtil::makeRational(c1_3));
            lambda1->child.push_back(SimpUtil::deepCopy(u));
            lambda1->child.push_back(SimpUtil::deepCopy(v));

            Exptree* omega = SimpUtil::makeFunction("/");
            Exptree* omegaNum = SimpUtil::makeFunction("+");
            omegaNum->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            Exptree* iSqrt3 = SimpUtil::makeFunction("*");
            iSqrt3->child.push_back(SimpUtil::makeVariable(ConstName::i));
            Exptree* sqrt3Node = SimpUtil::makeFunction(FuncName::sqrt);
            sqrt3Node->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
            iSqrt3->child.push_back(sqrt3Node);
            omegaNum->child.push_back(iSqrt3);
            omega->child.push_back(omegaNum);
            omega->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

            Exptree* omega2 = SimpUtil::makeFunction("*");
            omega2->child.push_back(SimpUtil::deepCopy(omega));
            omega2->child.push_back(SimpUtil::deepCopy(omega));

            Exptree* lambda2 = SimpUtil::makeFunction("+");
            lambda2->child.push_back(SimpUtil::makeRational(c1_3));
            Exptree* omega_u = SimpUtil::makeFunction("*");
            omega_u->child.push_back(SimpUtil::deepCopy(omega));
            omega_u->child.push_back(SimpUtil::deepCopy(u));
            lambda2->child.push_back(omega_u);
            Exptree* omega2_v = SimpUtil::makeFunction("*");
            omega2_v->child.push_back(omega2);
            omega2_v->child.push_back(SimpUtil::deepCopy(v));
            lambda2->child.push_back(omega2_v);

            Exptree* lambda3 = SimpUtil::makeFunction("+");
            lambda3->child.push_back(SimpUtil::makeRational(c1_3));
            Exptree* omega2_u = SimpUtil::makeFunction("*");
            omega2_u->child.push_back(SimpUtil::deepCopy(omega2));
            omega2_u->child.push_back(SimpUtil::deepCopy(u));
            lambda3->child.push_back(omega2_u);
            Exptree* omega_v = SimpUtil::makeFunction("*");
            omega_v->child.push_back(SimpUtil::deepCopy(omega));
            omega_v->child.push_back(SimpUtil::deepCopy(v));
            lambda3->child.push_back(omega_v);

            result->child.push_back(lambda1);
            result->child.push_back(lambda2);
            result->child.push_back(lambda3);
            SimpUtil::freeTree(node);
            return result;
        }

        // n = 4: Ferrari's method
        if (m == 4) {
            Rational c1 = coeffs[1], c2 = coeffs[2], c3 = coeffs[3], c4 = coeffs[4];
            Rational c1_4 = c1 / Rational(Intg(4));
            Rational p = c2 - Rational(Intg(6)) * c1_4 * c1_4;
            Rational q = c3 - Rational(Intg(2)) * c2 * c1_4 + Rational(Intg(8)) * c1_4 * c1_4 * c1_4;
            Rational r = c4 - c3 * c1_4 + c2 * c1_4 * c1_4 - Rational(Intg(3)) * c1_4 * c1_4 * c1_4 * c1_4;

            Rational rp = Rational(Intg(2)) * p;
            Rational rq = p * p - Rational(Intg(4)) * r;
            Rational rr = Rational(Intg(0)) - q * q;
            Rational rp_3 = rp / Rational(Intg(3));
            Rational P = rq - rp * rp_3;
            Rational Q = Rational(Intg(2)) * rp_3 * rp_3 * rp_3 - rp_3 * rq + rr;
            Q = Rational(Intg(0)) - Q;
            Rational disc3 = Q*Q / Rational(Intg(4)) + P*P*P / Rational(Intg(27));

            Exptree* sqrtDisc3 = SimpUtil::makeFunction(FuncName::sqrt);
            sqrtDisc3->child.push_back(SimpUtil::makeRational(disc3));

            Exptree* oneThirdNode = SimpUtil::makeRational(Rational(Intg(1), Intg(3)));
            Exptree* negQHalf3 = SimpUtil::makeRational(Rational(Intg(0)) - Q * Rational(Intg(1), Intg(2)));

            Exptree* u3_inner = SimpUtil::makeFunction("+");
            u3_inner->child.push_back(SimpUtil::deepCopy(negQHalf3));
            u3_inner->child.push_back(SimpUtil::deepCopy(sqrtDisc3));
            Exptree* u3 = SimpUtil::makeFunction("^");
            u3->child.push_back(u3_inner);
            u3->child.push_back(SimpUtil::deepCopy(oneThirdNode));

            Exptree* negSqrtDisc3 = SimpUtil::makeFunction("*");
            negSqrtDisc3->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negSqrtDisc3->child.push_back(SimpUtil::deepCopy(sqrtDisc3));
            Exptree* v3_inner = SimpUtil::makeFunction("+");
            v3_inner->child.push_back(SimpUtil::deepCopy(negQHalf3));
            v3_inner->child.push_back(negSqrtDisc3);
            Exptree* v3 = SimpUtil::makeFunction("^");
            v3->child.push_back(v3_inner);
            v3->child.push_back(SimpUtil::deepCopy(oneThirdNode));

            Exptree* m_node = SimpUtil::makeFunction("+");
            m_node->child.push_back(u3);
            m_node->child.push_back(v3);
            Exptree* negRP3 = SimpUtil::makeRational(Rational(Intg(0)) - rp_3);
            m_node->child.push_back(negRP3);

            Exptree* twoM = SimpUtil::makeFunction("*");
            twoM->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
            twoM->child.push_back(SimpUtil::deepCopy(m_node));
            Exptree* R = SimpUtil::makeFunction(FuncName::sqrt);
            R->child.push_back(twoM);

            Exptree* twoQ = SimpUtil::makeFunction("*");
            twoQ->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
            twoQ->child.push_back(SimpUtil::makeRational(q));
            Exptree* twoQoverR = SimpUtil::makeFunction("/");
            twoQoverR->child.push_back(twoQ);
            twoQoverR->child.push_back(SimpUtil::deepCopy(R));

            Exptree* negM = SimpUtil::makeFunction("*");
            negM->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negM->child.push_back(SimpUtil::deepCopy(m_node));
            Exptree* negP = SimpUtil::makeRational(Rational(Intg(0)) - p);

            Exptree* S_inner = SimpUtil::makeFunction("+");
            S_inner->child.push_back(SimpUtil::deepCopy(negM));
            S_inner->child.push_back(SimpUtil::deepCopy(negP));
            Exptree* neg2qR = SimpUtil::makeFunction("*");
            neg2qR->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            neg2qR->child.push_back(twoQoverR);
            S_inner->child.push_back(neg2qR);
            Exptree* S = SimpUtil::makeFunction(FuncName::sqrt);
            S->child.push_back(S_inner);

            Exptree* T_inner = SimpUtil::makeFunction("+");
            T_inner->child.push_back(SimpUtil::deepCopy(negM));
            T_inner->child.push_back(SimpUtil::deepCopy(negP));
            T_inner->child.push_back(SimpUtil::deepCopy(twoQoverR));
            Exptree* T = SimpUtil::makeFunction(FuncName::sqrt);
            T->child.push_back(T_inner);

            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            Exptree* negC1_4 = SimpUtil::makeRational(Rational(Intg(0)) - c1_4);

            result->child.push_back(SimpUtil::makeRational(Rational(Intg(4))));

            Exptree* l1 = SimpUtil::makeFunction("+");
            l1->child.push_back(SimpUtil::deepCopy(negC1_4));
            Exptree* RplusS = SimpUtil::makeFunction("+");
            RplusS->child.push_back(SimpUtil::deepCopy(R));
            RplusS->child.push_back(SimpUtil::deepCopy(S));
            Exptree* l1x = SimpUtil::makeFunction("*");
            l1x->child.push_back(half);
            l1x->child.push_back(RplusS);
            l1->child.push_back(l1x);
            result->child.push_back(l1);

            Exptree* l2 = SimpUtil::makeFunction("+");
            l2->child.push_back(SimpUtil::deepCopy(negC1_4));
            Exptree* RminusS = SimpUtil::makeFunction("+");
            RminusS->child.push_back(SimpUtil::deepCopy(R));
            Exptree* negS2 = SimpUtil::makeFunction("*");
            negS2->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negS2->child.push_back(SimpUtil::deepCopy(S));
            RminusS->child.push_back(negS2);
            Exptree* l2x = SimpUtil::makeFunction("*");
            l2x->child.push_back(SimpUtil::deepCopy(half));
            l2x->child.push_back(RminusS);
            l2->child.push_back(l2x);
            result->child.push_back(l2);

            Exptree* l3 = SimpUtil::makeFunction("+");
            l3->child.push_back(SimpUtil::deepCopy(negC1_4));
            Exptree* negRplusT = SimpUtil::makeFunction("+");
            Exptree* negR3 = SimpUtil::makeFunction("*");
            negR3->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negR3->child.push_back(SimpUtil::deepCopy(R));
            negRplusT->child.push_back(negR3);
            negRplusT->child.push_back(SimpUtil::deepCopy(T));
            Exptree* l3x = SimpUtil::makeFunction("*");
            l3x->child.push_back(SimpUtil::deepCopy(half));
            l3x->child.push_back(negRplusT);
            l3->child.push_back(l3x);
            result->child.push_back(l3);

            Exptree* l4 = SimpUtil::makeFunction("+");
            l4->child.push_back(SimpUtil::deepCopy(negC1_4));
            Exptree* negRminusT = SimpUtil::makeFunction("+");
            Exptree* negR4 = SimpUtil::makeFunction("*");
            negR4->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negR4->child.push_back(SimpUtil::deepCopy(R));
            negRminusT->child.push_back(negR4);
            Exptree* negT = SimpUtil::makeFunction("*");
            negT->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
            negT->child.push_back(SimpUtil::deepCopy(T));
            negRminusT->child.push_back(negT);
            Exptree* l4x = SimpUtil::makeFunction("*");
            l4x->child.push_back(SimpUtil::deepCopy(half));
            l4x->child.push_back(negRminusT);
            l4->child.push_back(l4x);
            result->child.push_back(l4);

            SimpUtil::freeTree(node);
            return result;
        }

        // n >= 5: characteristic polynomial coefficients
        result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)(m+1)))));
        for (size_t i = 0; i <= m; ++i)
            result->child.push_back(SimpUtil::makeRational(coeffs[i]));
        SimpUtil::freeTree(node);
        return result;
    }

    // ========== Eigenvec ==========

    Exptree* TreeSimplifier::simplifyEigenvec(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* mat = node->child[0];
        size_t m = 0, n = 0;
        if (!SimpUtil::matrixDims(mat, m, n)) return node;
        if (m != n || m == 0) return node;

        bool allRational = true;
        for (size_t i = 0; i < m && allRational; ++i)
            for (size_t j = 0; j < n && allRational; ++j)
                if (!SimpUtil::isRational(SimpUtil::matrixElem(mat, i, j, n)))
                    allRational = false;
        if (!allRational) return node;

        if (m == 1) {
            Exptree* result = SimpUtil::makeFunction(FuncName::vector);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
            Exptree* v = SimpUtil::makeFunction(FuncName::vector);
            v->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
            v->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
            result->child.push_back(v);
            SimpUtil::freeTree(node);
            return result;
        }

        if (m == 2) {
            Rational a = SimpUtil::matrixElem(mat, 0, 0, n)->value;
            Rational b = SimpUtil::matrixElem(mat, 0, 1, n)->value;
            Rational c = SimpUtil::matrixElem(mat, 1, 0, n)->value;
            Rational d = SimpUtil::matrixElem(mat, 1, 1, n)->value;
            Rational trace = a + d;
            Rational det = a*d - b*c;
            Rational disc = trace*trace - Rational(Intg(4))*det;

            Exptree* result = SimpUtil::makeFunction(FuncName::vector);
            result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));

            Exptree* sqrtDisc = SimpUtil::makeFunction(FuncName::sqrt);
            sqrtDisc->child.push_back(SimpUtil::makeRational(disc));
            sqrtDisc = simplifySqrt(sqrtDisc);

            auto buildVec = [&](int sign) -> Exptree* {
                Exptree* v = SimpUtil::makeFunction(FuncName::vector);
                v->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                Exptree* lambda = SimpUtil::makeFunction("/");
                Exptree* s = SimpUtil::makeFunction("+");
                s->child.push_back(SimpUtil::makeRational(Rational(Intg(0))-trace));
                if (sign == 1) {
                    s->child.push_back(SimpUtil::deepCopy(sqrtDisc));
                } else {
                    Exptree* ns = SimpUtil::makeFunction("*");
                    ns->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                    ns->child.push_back(SimpUtil::deepCopy(sqrtDisc));
                    s->child.push_back(ns);
                }
                lambda->child.push_back(s);
                lambda->child.push_back(SimpUtil::makeRational(Rational(Intg(-2))));

                if (!b.isZero()) {
                    v->child.push_back(SimpUtil::makeRational(b));
                    Exptree* vy = SimpUtil::makeFunction("+");
                    vy->child.push_back(lambda);
                    vy->child.push_back(SimpUtil::makeRational(Rational(Intg(0))-a));
                    v->child.push_back(vy);
                } else {
                    Exptree* vx = SimpUtil::makeFunction("+");
                    vx->child.push_back(lambda);
                    vx->child.push_back(SimpUtil::makeRational(Rational(Intg(0))-d));
                    v->child.push_back(vx);
                    v->child.push_back(SimpUtil::makeRational(c));
                }
                return v;
            };

            result->child.push_back(buildVec(1));
            result->child.push_back(buildVec(-1));
            SimpUtil::freeTree(node);
            return result;
        }

        return node;
    }

    Exptree* TreeSimplifier::simplifyAdjoint(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* mat = node->child[0];

        size_t m = 0, n = 0;
        if (!SimpUtil::matrixDims(mat, m, n)) return node;
        if (m != n || m == 0) return node;

        bool allRational = true;
        for (size_t i = 0; i < m && allRational; ++i)
            for (size_t j = 0; j < n && allRational; ++j)
                if (!SimpUtil::isRational(SimpUtil::matrixElem(mat, i, j, n)))
                    allRational = false;
        if (!allRational) return node;

        // Build cofactor matrix
        Exptree* cofactor = SimpUtil::makeFunction(FuncName::matrix);
        cofactor->child.push_back(SimpUtil::makeRational(Rational(Intg((int)m))));
        cofactor->child.push_back(SimpUtil::makeRational(Rational(Intg((int)n))));

        for (size_t i = 0; i < m; ++i) {
            for (size_t j = 0; j < n; ++j) {
                // Build submatrix excluding row i, col j
                Exptree* sub = SimpUtil::makeFunction(FuncName::matrix);
                sub->child.push_back(SimpUtil::makeRational(Rational(Intg((int)(m-1)))));
                sub->child.push_back(SimpUtil::makeRational(Rational(Intg((int)(n-1)))));
                for (size_t r = 0; r < m; ++r) {
                    if (r == i) continue;
                    for (size_t c = 0; c < n; ++c) {
                        if (c == j) continue;
                        sub->child.push_back(SimpUtil::deepCopy(SimpUtil::matrixElem(mat, r, c, n)));
                    }
                }

                // det of submatrix
                Exptree* detNode = SimpUtil::makeFunction(FuncName::det);
                detNode->child.push_back(sub);
                detNode = simplifyDet(detNode);

                Rational cofactorVal;
                if (SimpUtil::isRational(detNode)) {
                    cofactorVal = detNode->value;
                } else {
                    // Keep symbolic
                    cofactorVal = Rational(Intg(0)); // fallback, shouldn't happen for rational matrix
                }
                SimpUtil::freeTree(detNode);

                if ((i + j) % 2 == 1) {
                    cofactorVal = Rational(Intg(0)) - cofactorVal;
                }
                cofactor->child.push_back(SimpUtil::makeRational(cofactorVal));
            }
        }

        // adjoint = cofactor^T
        Exptree* result = SimpUtil::makeFunction(FuncName::matrix);
        result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)n))));
        result->child.push_back(SimpUtil::makeRational(Rational(Intg((int)m))));
        for (size_t j = 0; j < n; ++j) {
            for (size_t i = 0; i < m; ++i) {
                result->child.push_back(SimpUtil::deepCopy(cofactor->child[2 + i * n + j]));
            }
        }

        SimpUtil::freeTree(cofactor);
        SimpUtil::freeTree(node);
        return result;
    }

} // namespace CAS