/** @file /inc/cas/treesimp.hpp
 *  @brief Expression tree simplification for the computer algebra system module
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

#ifndef _CAS_TREESIMP_HPP_
#define _CAS_TREESIMP_HPP_

#include "cas/expdef.hpp"
#include "cas/simp/simputil.hpp"
#include "cas/simp/exptreepool.hpp"
#include "cas/simp/simp_internal.hpp"

namespace CAS {

    /**
     *  @var g_pool
     *  @brief Global instance of the expression tree node pool
     *  @details Provides memory allocation services for all simplification
     *           operations. Defined in simplify.cpp.
     */
    extern ExptreePool g_pool;

    /**
     *  @class TreeSimplifier
     *  @brief Expression tree simplifier implementing symbolic algebra rules
     *  @details Provides a static entry point for simplifying expression trees.
     *           Internally uses a singleton pattern to maintain reusable work
     *           buffers. The simplification process consists of two phases:
     *           1. Preprocessing normalization (e.g., a-b -> a+(-1)*b)
     *           2. Bottom-up recursive simplification with constant folding,
     *              like-term collection, and special value recognition.
     *
     *           Supports arithmetic, trigonometric, hyperbolic, logarithmic,
     *           and other elementary functions with complex number support
     *           including Euler's formula and powers of i.
     */
    class TreeSimplifier {
    public:
        /**
         *  @name simplify
         *  @brief Main entry point for expression tree simplification
         *  @param root Reference to pointer to the root node. May be replaced
         *              during simplification.
         *  @return Pointer to the simplified root node
         *  @details Applies preprocessing normalization followed by recursive
         *           bottom-up simplification. The original tree may be modified
         *           or replaced entirely.
         *  @example
         *  @code
         *  Exptree* expr = buildExpression("x + 2*x - 3 + 1");
         *  TreeSimplifier::simplify(expr);
         *  // expr now represents 3*x - 2
         *  @endcode
         */
        static Exptree* simplify(Exptree*& root);

    private:
        /** @brief Private default constructor for singleton pattern */
        TreeSimplifier() = default;
        TreeSimplifier(const TreeSimplifier&) = delete;
        TreeSimplifier& operator=(const TreeSimplifier&) = delete;

        /**
         *  @name instance
         *  @brief Get the singleton instance
         *  @return Reference to the static TreeSimplifier instance
         *  @details Meyer's singleton, thread-safe in C++11+.
         */
        static TreeSimplifier& instance();

        /** @brief Reusable work buffer for addition term collection */
        WorkBuffer addBuf_;
        /** @brief Reusable work buffer for multiplication factor collection */
        WorkBuffer mulBuf_;

        // ========== Preprocessing phase ==========

        /**
         *  @name preTransform
         *  @brief Normalize expression forms before simplification
         *  @param node Reference to node pointer, may be restructured in-place
         *  @details Applies canonicalizing transformations:
         *           - a-b -> a+(-1)*b, -x -> (-1)*x
         *           - a/b -> a*b^(-1), sqrt(x) -> x^(1/2)
         *           - exp(x) -> e^x, root(a,b) -> b^(1/a)
         *           - phi -> (sqrt(5)+1)/2
         *           - deg(x) -> (pi/180)*x, rad(x) -> (180/pi)*x
         *           - Hyperbolic functions to exponential/logarithmic form
         *           - Flatten nested same-operator trees
         */
        void preTransform(Exptree*& node);

        // ========== Core recursive simplification ==========

        /**
         *  @name simplifyNode
         *  @brief Recursively simplify an expression node bottom-up
         *  @param node Pointer to the node to simplify
         *  @return Pointer to the simplified node (may differ from input)
         *  @details Children simplified first, then dispatches to specialized
         *           simplifiers based on function name.
         */
        Exptree* simplifyNode(Exptree* node);

        // ========== Arithmetic operation simplifiers ==========

        /**
         *  @name simplifyAdd
         *  @brief Simplify an addition expression
         *  @details Collects terms, merges like terms, constant folding,
         *           eliminates zero terms, returns canonical sum.
         */
        Exptree* simplifyAdd(Exptree* node);

        /**
         *  @name simplifyMul
         *  @brief Simplify a multiplication expression
         *  @details Collects factors, merges like bases by adding exponents,
         *           constant folding, eliminates 1 and 0 factors.
         */
        Exptree* simplifyMul(Exptree* node);

        /**
         *  @name simplifyPow
         *  @brief Simplify a power expression
         *  @details Handles 0^x=0, x^0=1, x^1=x, 1^x=1, constant folding,
         *           (-1)^n cycle, i^n cycle, Euler's formula, complex sqrt,
         *           power-of-power combination, distribution over products.
         */
        Exptree* simplifyPow(Exptree* node);

        /**
         *  @name simplifyNeg
         *  @brief Simplify negation: -0=0, -(-x)=x, -n=-n, else (-1)*x
         */
        Exptree* simplifyNeg(Exptree* node);

        // ========== Trigonometric simplifiers ==========

        /**
         *  @name simplifySin
         *  @brief Simplify sine: special angles, sin(0)=0, sin(-x)=-sin(x)
         */
        Exptree* simplifySin(Exptree* node);
        /**
         *  @name simplifyCos
         *  @brief Simplify cosine: special angles, cos(0)=1, cos(-x)=cos(x)
         */
        Exptree* simplifyCos(Exptree* node);
        /**
         *  @name simplifyTan
         *  @brief Simplify tangent: special angles, tan(0)=0, tan(-x)=-tan(x)
         */
        Exptree* simplifyTan(Exptree* node);
        /**
         *  @name simplifyAsin
         *  @brief Simplify inverse sine: asin(0)=0, asin(1)=pi/2, asin(1/2)=pi/6
         */
        Exptree* simplifyAsin(Exptree* node);
        /**
         *  @name simplifyAcos
         *  @brief Simplify inverse cosine: acos(0)=pi/2, acos(1)=0, acos(1/2)=pi/3
         */
        Exptree* simplifyAcos(Exptree* node);
        /**
         *  @name simplifyAtan
         *  @brief Simplify inverse tangent: atan(0)=0, atan(1)=pi/4
         */
        Exptree* simplifyAtan(Exptree* node);

        // ========== Hyperbolic simplifiers ==========

        /**
         *  @name simplifySinh
         *  @brief Simplify hyperbolic sine via conversion to exponential form
         */
        Exptree* simplifySinh(Exptree* node);
        /**
         *  @name simplifyCosh
         *  @brief Simplify hyperbolic cosine via conversion to exponential form
         */
        Exptree* simplifyCosh(Exptree* node);
        /**
         *  @name simplifyTanh
         *  @brief Simplify hyperbolic tangent via conversion to exponential form
         */
        Exptree* simplifyTanh(Exptree* node);
        /**
         *  @name simplifyAsinh
         *  @brief Simplify inverse hyperbolic sine via logarithmic form
         */
        Exptree* simplifyAsinh(Exptree* node);
        /**
         *  @name simplifyAcosh
         *  @brief Simplify inverse hyperbolic cosine via logarithmic form
         */
        Exptree* simplifyAcosh(Exptree* node);
        /**
         *  @name simplifyAtanh
         *  @brief Simplify inverse hyperbolic tangent via logarithmic form
         */
        Exptree* simplifyAtanh(Exptree* node);

        // ========== Logarithmic simplifiers ==========

        /**
         *  @name simplifyLn
         *  @brief Simplify natural logarithm: ln(1)=0, ln(e)=1, ln(e^x)=x,
         *         ln(x^a)=a*ln(x) for x>0
         */
        Exptree* simplifyLn(Exptree* node);
        /**
         *  @name simplifyLog
         *  @brief Simplify general logarithm: log(1)=0, log(x,x)=1,
         *         log(x,e)=ln(x), log(x,10)=log10(x), else ln(a)/ln(b)
         */
        Exptree* simplifyLog(Exptree* node);
        /**
         *  @name simplifyLog10
         *  @brief Simplify base-10 logarithm: special values for 1,10,100,1000,
         *         else ln(x)/ln(10)
         */
        Exptree* simplifyLog10(Exptree* node);

        // ========== Other function simplifiers ==========

        /**
         *  @name simplifySqrt
         *  @brief Simplify square root: sqrt(0)=0, sqrt(1)=1, perfect squares,
         *         sqrt(x^2)=abs(x), sqrt(-a)=i*sqrt(a)
         */
        Exptree* simplifySqrt(Exptree* node);
        /**
         *  @name simplifyAbs
         *  @brief Simplify absolute value: abs(0)=0, abs(abs(x))=abs(x),
         *         abs(n)=|n|, abs(-x)=abs(x), abs(x*y)=abs(x)*abs(y)
         */
        Exptree* simplifyAbs(Exptree* node);
        /**
         *  @name simplifySignum
         *  @brief Simplify signum: sign(0)=0, sign(n>0)=1, sign(n<0)=-1,
         *         sign(-x)=-sign(x), sign(x*y)=sign(x)*sign(y), sign(abs(x))=1
         */
        Exptree* simplifySignum(Exptree* node);
        /**
         *  @name simplifyFact
         *  @brief Simplify factorial: 0!=1, 1!=1, n! for n<=20
         */
        Exptree* simplifyFact(Exptree* node);
        /**
         *  @name simplifyDeg
         *  @brief Convert degrees to radians via preTransform
         */
        Exptree* simplifyDeg(Exptree* node);
        /**
         *  @name simplifyRad
         *  @brief Convert radians to degrees via preTransform
         */
        Exptree* simplifyRad(Exptree* node);

        // ========== Internal helper methods ==========

        /**
         *  @name collectAddTerms
         *  @brief Recursively flatten addition into buffer, accumulate constants
         */
        void collectAddTerms(Exptree* node, WorkBuffer& buf);
        /**
         *  @name mergeAddTerms
         *  @brief Merge like terms in buffer by adding coefficients
         */
        void mergeAddTerms(WorkBuffer& buf);
        /**
         *  @name rebuildAdd
         *  @brief Rebuild canonical addition from buffer, sort, add constant
         */
        Exptree* rebuildAdd(WorkBuffer& buf);

        /**
         *  @name collectMulFactors
         *  @brief Recursively flatten multiplication into buffer
         */
        void collectMulFactors(Exptree* node, WorkBuffer& buf);
        /**
         *  @name mergeMulFactors
         *  @brief Merge like factors by adding exponents
         */
        void mergeMulFactors(WorkBuffer& buf);
        /**
         *  @name rebuildMul
         *  @brief Rebuild canonical multiplication from buffer
         */
        Exptree* rebuildMul(WorkBuffer& buf);

        /**
         *  @name foldRationalPower
         *  @brief Compute exact rational power for non-negative integer exponents
         *  @return Result node or nullptr if not computable
         */
        Exptree* foldRationalPower(Exptree* base, Exptree* exp);
        /**
         *  @name isPerfectSquare
         *  @brief Check if rational is perfect square, output root if true
         */
        bool isPerfectSquare(Rational r, Rational& root);
        /**
         *  @name simplifyEulerForm
         *  @brief Apply Euler's formula: e^(i*theta) -> cos(theta)+i*sin(theta)
         */
        Exptree* simplifyEulerForm(Exptree* expArg);
        /**
         *  @name simplifyTrigSpecialAngles
         *  @brief Recognize trig values for 0, pi/6, pi/4, pi/3, pi/2, pi, etc.
         */
        Exptree* simplifyTrigSpecialAngles(Exptree* node, const char* funcName);
        /**
         *  @name handleComplexSqrt
         *  @brief Convert sqrt(-a) to i*sqrt(a), sqrt(-1) to i
         */
        Exptree* handleComplexSqrt(Exptree* node);
        /**
         *  @name sortItems
         *  @brief Bubble sort expression pointers by canonical order
         */
        void sortItems(Exptree** items, size_t count);
    };

} // namespace CAS

#endif // _CAS_TREESIMP_HPP_