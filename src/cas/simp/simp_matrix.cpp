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

        // Scalar * vector
        if (!vectors.empty() && matrices.empty()) {
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

        // Scalar * matrix
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
                        prod->child.push_back(
                            SimpUtil::deepCopy(SimpUtil::matrixElem(mat, r, c, n)));
                        prod = simplifyMul(prod);
                        result->child.push_back(prod);
                    } else {
                        result->child.push_back(
                            SimpUtil::deepCopy(SimpUtil::matrixElem(mat, r, c, n)));
                    }
                }
            }

            if (scalarProd) SimpUtil::freeTree(scalarProd);
            SimpUtil::freeTree(node);
            return result;
        }

        // Matrix * Matrix
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
                        prod->child.push_back(
                            SimpUtil::deepCopy(SimpUtil::matrixElem(A, r, k, nA)));
                        prod->child.push_back(
                            SimpUtil::deepCopy(SimpUtil::matrixElem(B, k, c, nB)));
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

        // Vector * Vector: 2D/3D cross product, otherwise dot product
        if (vectors.size() == 2 && matrices.empty()) {
            Exptree* a = vectors[0];
            Exptree* b = vectors[1];
            size_t szA = SimpUtil::vectorElemCount(a);
            size_t szB = SimpUtil::vectorElemCount(b);

            if (szA == szB) {
                if (szA == 2) {
                    // 2D cross product = a1*b2 - a2*b1
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

                    if (scalarProd) {
                        Exptree* scaled = SimpUtil::makeFunction("*");
                        scaled->child.push_back(SimpUtil::deepCopy(scalarProd));
                        scaled->child.push_back(diff);
                        diff = simplifyMul(scaled);
                    }

                    if (scalarProd) SimpUtil::freeTree(scalarProd);
                    SimpUtil::freeTree(node);
                    return diff;
                }

                if (szA == 3) {
                    // 3D cross product
                    Exptree* result = SimpUtil::makeFunction(FuncName::vector);
                    result->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));

                    // x = a2*b3 - a3*b2
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

                    // y = a3*b1 - a1*b3
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

                    // z = a1*b2 - a2*b1
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

                // Default: dot product for other dimensions
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

        if (scalarProd) SimpUtil::freeTree(scalarProd);
        return node;
    }

} // namespace CAS