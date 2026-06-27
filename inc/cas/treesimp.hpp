/** @file /inc/cas/treesimp.hpp
 *  @brief Expression tree simplification for the computer algebra system module of the calculator project
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

#include "cas/exptree.hpp"
#include "cas/expdef.hpp"
#include <algorithm>
#include <cstring>


namespace CAS {

    /**
     *  @name SimpUtil
     *  @brief Utility functions and types for the expression tree simplifier
     */
    namespace SimpUtil {

        // Forward declarations
        Exptree* deepCopy(Exptree* src);
        void freeTree(Exptree* root);
        int8_t compareNodes(Exptree* a, Exptree* b);
        bool isNumeric(Exptree* n);

        /** @name isRational
         *  @brief Check if a node holds a rational number value
         */
        inline bool isRational(Exptree* n) {
            return n && n->valtp == Exptree::val_t::valRational;
        }

        /** @name isVariable
         *  @brief Check if a node holds a variable name
         */
        inline bool isVariable(Exptree* n) {
            return n && n->valtp == Exptree::val_t::valVariable;
        }

        /** @name isFunction
         *  @brief Check if a node is a function or operator node
         */
        inline bool isFunction(Exptree* n) {
            return n && n->valtp == Exptree::val_t::valFunction;
        }

        /** @name isFunction
         *  @brief Check if a node is a specific named function
         */
        inline bool isFunction(Exptree* n, const char* name) {
            return isFunction(n) && n->var == name;
        }

        /** @name isZero
         *  @brief Check if a node represents the rational number zero
         */
        inline bool isZero(Exptree* n) {
            return isRational(n) && n->value.isZero();
        }

        /** @name isOne
         *  @brief Check if a node represents the rational number one
         */
        inline bool isOne(Exptree* n) {
            return isRational(n) && n->value == Rational(Intg(1));
        }

        /** @name isMinusOne
         *  @brief Check if a node represents the rational number negative one
         */
        inline bool isMinusOne(Exptree* n) {
            return isRational(n) && n->value == Rational(Intg(-1));
        }

        /** @name isPositive
         *  @brief Check if a node is a strictly positive rational number
         */
        inline bool isPositive(Exptree* n) {
            return isRational(n) && n->value > Rational(Intg(0));
        }

        /** @name isNegative
         *  @brief Check if a node is a strictly negative rational number
         */
        inline bool isNegative(Exptree* n) {
            return isRational(n) && n->value < Rational(Intg(0));
        }

        /** @name isInteger
         *  @brief Check if a node is a rational with denominator 1 (integer value)
         */
        inline bool isInteger(Exptree* n) {
            return isRational(n) && n->value.isInteger();
        }

        /** @name isEvenInteger
         *  @brief Check if a node is an even integer rational number
         */
        inline bool isEvenInteger(Exptree* n) {
            if (!isInteger(n)) return false;
            Intg num = n->value.numerator();
            return (num % Intg(2)) == Intg(0);
        }

        /** @name isConstantE
         *  @brief Check if a node represents the mathematical constant e
         */
        inline bool isConstantE(Exptree* n) {
            return isVariable(n) && n->var == ConstName::e;
        }

        /** @name isConstantPi
         *  @brief Check if a node represents the mathematical constant pi
         */
        inline bool isConstantPi(Exptree* n) {
            return isVariable(n) && n->var == ConstName::pi;
        }

        /** @name isConstantPhi
         *  @brief Check if a node represents the golden ratio phi
         */
        inline bool isConstantPhi(Exptree* n) {
            return isVariable(n) && n->var == ConstName::phi;
        }

        /** @name isConstantI
         *  @brief Check if a node represents the imaginary unit i
         */
        inline bool isConstantI(Exptree* n) {
            return isVariable(n) && n->var == ConstName::i;
        }

        /** @name makeRational
         *  @brief Create a new rational number node from the object pool
         */
        inline Exptree* makeRational(const Rational& v) {
            Exptree* n = g_pool.allocate();
            n->valtp = Exptree::val_t::valRational;
            n->value = v;
            return n;
        }

        /** @name makeVariable
         *  @brief Create a new variable node from the object pool
         */
        inline Exptree* makeVariable(const char* name) {
            Exptree* n = g_pool.allocate();
            n->valtp = Exptree::val_t::valVariable;
            n->var = name;
            return n;
        }

        /** @name makeFunction
         *  @brief Create a new function node from the object pool
         */
        inline Exptree* makeFunction(const char* name) {
            Exptree* n = g_pool.allocate();
            n->valtp = Exptree::val_t::valFunction;
            n->var = name;
            return n;
        }

    } // namespace SimpUtil

    // ============================================================================
    // Global object pool 
    // ============================================================================

    /**
     *  @class ExptreePool
     *  @brief Fixed-size object pool for Exptree nodes
     */
    class ExptreePool {
        /** @name POOL_SIZE
         *  @brief Number of pre-allocated nodes in the pool
         *  @details 48 nodes at approximately 200 bytes each uses about 10KB of SRAM,
         *           leaving ample memory for other operations on the RP2040.
         */
        static constexpr size_t POOL_SIZE = 48;

        /** @name MAX_CHILDREN
         *  @brief Maximum number of child pointers pre-reserved per node
         */
        static constexpr size_t MAX_CHILDREN = 6;

        /** @name PoolNode
         *  @brief Internal structure representing a single pool entry
         */
        struct PoolNode {
            Exptree node;       ///< The actual expression tree node
            bool used;          ///< Allocation flag

            /** @name PoolNode
             *  @brief Constructor initializes pool entry as free
             */
            PoolNode() : used(false) {
                node.child.reserve(MAX_CHILDREN);
            }
        };

        PoolNode pool_[POOL_SIZE];

    public:
        /** @name allocate
         *  @brief Allocate an Exptree node from the pool
         *  @return Pointer to a freshly initialized Exptree node
         *  @details Searches the pool for a free entry. If found, marks it as used,
         *           resets the node to default state, and returns its address.
         *           If the pool is exhausted, falls back to heap allocation via new.
         */
        Exptree* allocate() {
            for (size_t i = 0; i < POOL_SIZE; ++i) {
                if (!pool_[i].used) {
                    pool_[i].used = true;
                    pool_[i].node.valtp = Exptree::val_t::valNull;
                    pool_[i].node.value = Rational(Intg(0));
                    pool_[i].node.var.clear();
                    pool_[i].node.child.clear();
                    return &pool_[i].node;
                }
            }
            // Pool exhausted, fallback to heap allocation
            Exptree* fallback = new Exptree();
            fallback->child.reserve(MAX_CHILDREN);
            return fallback;
        }

        /** @name deallocate
         *  @brief Return a node and all its descendants to the pool
         *  @param node Pointer to the root of the subtree to free
         *  @details Recursively frees all child nodes first, then returns the root node
         *           to the pool if it originated from there. Nodes allocated as heap
         *           fallback are freed using delete.
         */
        void deallocate(Exptree* node) {
            if (!node) return;

            // Recursively free all children first
            for (size_t i = 0; i < node->child.size(); ++i) {
                deallocate(node->child[i]);
            }
            node->child.clear();

            // Check if this node belongs to the pool
            for (size_t i = 0; i < POOL_SIZE; ++i) {
                if (&pool_[i].node == node) {
                    pool_[i].used = false;
                    return;
                }
            }
            // Not from pool, use standard delete
            delete node;
        }

        /** @name available
         *  @brief Query the number of free nodes remaining in the pool
         *  @return Count of unused pool entries
         */
        size_t available() const {
            size_t count = 0;
            for (size_t i = 0; i < POOL_SIZE; ++i) {
                if (!pool_[i].used) count++;
            }
            return count;
        }
    };

    extern ExptreePool g_pool;

    /**
     *  @class TreeSimplifier
     *  @brief Expression tree simplifier implementing symbolic algebra rules
     */
    class TreeSimplifier {
    public:
        /** @name simplify
         *  @brief Main entry point for expression tree simplification
         *  @param root Reference to pointer to the root node. May be replaced during simplification.
         *  @return Pointer to the simplified root node
         *  @details Applies preprocessing normalization followed by recursive bottom-up
         *           simplification. The original tree may be modified or replaced entirely.
         *  @example
         *  @code
         *  Exptree* expr = buildExpression("x + 2*x - 3 + 1");
         *  TreeSimplifier::simplify(expr);
         *  // expr now represents 3*x - 2 in normalized form:
         *  // ((+) ((*) 3 x) -2)
         *  @endcode
         */
        static Exptree* simplify(Exptree*& root);

    private:
        TreeSimplifier() = default;

        // Non-copyable
        TreeSimplifier(const TreeSimplifier&) = delete;
        TreeSimplifier& operator=(const TreeSimplifier&) = delete;

        /** @name instance
         *  @brief Get the singleton instance of the simplifier
         *  @return Reference to the static TreeSimplifier instance
         *  @details Uses Meyer's singleton pattern for thread-safe initialization
         */
        static TreeSimplifier& instance() {
            static TreeSimplifier inst;
            return inst;
        }

        /**
         *  @name WorkBuffer
         *  @brief Reusable buffer for collecting and merging terms during simplification
         *  @details Avoids dynamic allocation during recursive simplification by providing
         *           fixed-size arrays. The same buffer instances are reused across multiple
         *           simplification calls in the singleton TreeSimplifier.
         */
        struct WorkBuffer {
            /** @name MAX_ITEMS
             *  @brief Maximum number of terms/factors the buffer can hold
             */
            static constexpr size_t MAX_ITEMS = 24;

            Exptree* items[MAX_ITEMS];  ///< Array of pointers to collected terms/factors
            size_t count;               ///< Current number of items in the buffer
            Rational constantAccum;     ///< Accumulated constant value during collection

            /** @name resetAdd
             *  @brief Reset buffer state for addition term collection (constant = 0)
             */
            void resetAdd() {
                count = 0;
                constantAccum = Rational(Intg(0));
            }

            /** @name resetMul
             *  @brief Reset buffer state for multiplication factor collection (constant = 1)
             */
            void resetMul() {
                count = 0;
                constantAccum = Rational(Intg(1));
            }

            /** @name addItem
             *  @brief Add an item pointer to the buffer
             *  @param item Pointer to the expression node to add
             *  @return true if the item was added, false if the buffer is full
             */
            bool addItem(Exptree* item) {
                if (count >= MAX_ITEMS) return false;
                items[count++] = item;
                return true;
            }
        };

        WorkBuffer addBuf_;  ///< Work buffer for addition operations
        WorkBuffer mulBuf_;  ///< Work buffer for multiplication operations

        // ========== Preprocessing phase ==========
                
        /** @name preTransform
         *  @brief Normalize expression forms before simplification
         *  @param node Reference to node pointer (may be restructured in-place)
         *  @details Performs the following normalizations:
         *           - Subtraction a-b -> a + (-1)*b
         *           - Unary minus -x -> (-1)*x
         *           - Division a/b -> a * b^(-1)
         *           - sqrt(x) -> x^(1/2)
         *           - root(a,b) -> b^(1/a)
         *           - exp(x) -> e^x
         *           - Flatten nested same-operator trees (a+(b+c) -> a+b+c)
         */
        void preTransform(Exptree*& node);

        // ========== Core recursive simplification ==========

        /** @name simplifyNode
         *  @brief Recursively simplify an expression node and all its children
         *  @param node Pointer to the node to simplify
         *  @return Pointer to the simplified node (may be different from input)
         *  @details Performs bottom-up simplification: children are simplified first,
         *           then the node itself is simplified based on its function/operator type.
         */
        Exptree* simplifyNode(Exptree* node);

        // ========== Arithmetic operation simplifiers ==========
        Exptree* simplifyAdd(Exptree* node);
        Exptree* simplifyMul(Exptree* node);
        Exptree* simplifyPow(Exptree* node);
        Exptree* simplifyNeg(Exptree* node);

        // ========== Function simplifiers ==========
        Exptree* simplifySqrt(Exptree* node);
        Exptree* simplifyAbs(Exptree* node);
        Exptree* simplifyLn(Exptree* node);
        Exptree* simplifySin(Exptree* node);
        Exptree* simplifyCos(Exptree* node);
        Exptree* simplifyTan(Exptree* node);
        Exptree* simplifyAsin(Exptree* node);
        Exptree* simplifyAcos(Exptree* node);
        Exptree* simplifyAtan(Exptree* node);
        
        /** @name simplifySinh
         *  @brief Convert hyperbolic sine to exponential form: sinh(x) = (e^x - e^(-x))/2
         */
        Exptree* simplifySinh(Exptree* node);
        
        /** @name simplifyCosh
         *  @brief Convert hyperbolic cosine to exponential form: cosh(x) = (e^x + e^(-x))/2
         */
        Exptree* simplifyCosh(Exptree* node);
        
        /** @name simplifyTanh
         *  @brief Convert hyperbolic tangent to exponential form or sinh/cosh ratio
         */
        Exptree* simplifyTanh(Exptree* node);
        
        /** @name simplifyAsinh
         *  @brief Simplify inverse hyperbolic sine: asinh(x) = ln(x + sqrt(x^2 + 1))
         */
        Exptree* simplifyAsinh(Exptree* node);
        
        /** @name simplifyAcosh
         *  @brief Simplify inverse hyperbolic cosine: acosh(x) = ln(x + sqrt(x^2 - 1)) for x >= 1
         */
        Exptree* simplifyAcosh(Exptree* node);
        
        /** @name simplifyAtanh
         *  @brief Simplify inverse hyperbolic tangent: atanh(x) = (1/2)*ln((1+x)/(1-x)) for |x| < 1
         */
        Exptree* simplifyAtanh(Exptree* node);

        /** @name simplifyLog
         *  @brief Simplify a general logarithm expression
         */
        Exptree* simplifyLog(Exptree* node);
        
        /** @name simplifyLog10
         *  @brief Simplify base-10 logarithm with special value recognition
         */
        Exptree* simplifyLog10(Exptree* node);
        
        /** @name simplifySignum
         *  @brief Simplify a signum expression
         */
        Exptree* simplifySignum(Exptree* node);
        
        /** @name simplifyFact
         *  @brief Simplify a factorial expression
         */
        Exptree* simplifyFact(Exptree* node);
        
        /** @name simplifyDeg
         *  @brief Convert degrees to radians: deg(x) = (pi/180)*x
         */
        Exptree* simplifyDeg(Exptree* node);
        
        /** @name simplifyRad
         *  @brief Convert radians to degrees: rad(x) = (180/pi)*x
         */
        Exptree* simplifyRad(Exptree* node);

        // ========== Addition internal methods ==========
        void collectAddTerms(Exptree* node, WorkBuffer& buf);
        void mergeAddTerms(WorkBuffer& buf);
        Exptree* rebuildAdd(WorkBuffer& buf);

        // ========== Multiplication internal methods ==========
        void collectMulFactors(Exptree* node, WorkBuffer& buf);
        void mergeMulFactors(WorkBuffer& buf);
        Exptree* rebuildMul(WorkBuffer& buf);

        // ========== Power internal methods ==========
        Exptree* foldRationalPower(Exptree* base, Exptree* exp);
        bool isPerfectSquare(const Rational& r, Rational& root);
        Exptree* simplifyEulerForm(Exptree* expArg);

        // ========== Trigonometric helper ==========
        /** @name simplifyTrigSpecialAngles
         *  @brief Recognize and simplify trigonometric functions for special angles
         *  @param node The trig function node
         *  @param funcName The trig function name ("sin", "cos", "tan")
         *  @return Simplified expression or nullptr if not a special angle
         *  @details Handles angles: 0, pi, pi/2, pi/3, pi/4, pi/6, pi/5 (36°), pi/10 (18°)
         */
        Exptree* simplifyTrigSpecialAngles(Exptree* node, const char* funcName);

        // ========== Complex number support ==========
        Exptree* handleComplexSqrt(Exptree* node);

        // ========== Sorting ==========
        void sortItems(Exptree** items, size_t count);
    };

    // Global pool instance
    ExptreePool g_pool;

    // ============================================================================
    // SimpUtil implementations
    // ============================================================================
    namespace SimpUtil {

        Exptree* deepCopy(Exptree* src) {
            if (!src) return nullptr;
            Exptree* copy = g_pool.allocate();
            copy->valtp = src->valtp;
            if (src->valtp == Exptree::val_t::valRational) {
                copy->value = src->value;
            } else if (src->valtp == Exptree::val_t::valVariable) {
                copy->var = src->var;
            } else if (src->valtp == Exptree::val_t::valFunction) {
                copy->var = src->var;
                for (size_t i = 0; i < src->child.size(); ++i) {
                    copy->child.push_back(deepCopy(src->child[i]));
                }
            }
            return copy;
        }

        void freeTree(Exptree* root) {
            if (root) g_pool.deallocate(root);
        }

        int8_t compareNodes(Exptree* a, Exptree* b) {
            if (!a && !b) return 0;
            if (!a) return -1;
            if (!b) return 1;
            if (a->valtp != b->valtp) {
                return static_cast<int8_t>(a->valtp) - static_cast<int8_t>(b->valtp);
            }
            if (a->valtp == Exptree::val_t::valRational) {
                if (a->value < b->value) return -1;
                if (a->value > b->value) return 1;
                return 0;
            }
            if (a->valtp == Exptree::val_t::valVariable) {
                if (a->var < b->var) return -1;
                if (a->var > b->var) return 1;
                return 0;
            }
            if (a->valtp == Exptree::val_t::valFunction) {
                if (a->var < b->var) return -1;
                if (a->var > b->var) return 1;
                size_t minSz = (a->child.size() < b->child.size()) ? a->child.size() : b->child.size();
                for (size_t i = 0; i < minSz; ++i) {
                    int8_t cmp = compareNodes(a->child[i], b->child[i]);
                    if (cmp != 0) return cmp;
                }
                if (a->child.size() < b->child.size()) return -1;
                if (a->child.size() > b->child.size()) return 1;
                return 0;
            }
            return 0;
        }

        bool isNumeric(Exptree* n) {
            if (!n) return false;
            if (n->valtp == Exptree::val_t::valRational) return true;
            if (n->valtp == Exptree::val_t::valVariable) return false;
            if (n->valtp == Exptree::val_t::valFunction) {
                for (size_t i = 0; i < n->child.size(); ++i) {
                    if (!isNumeric(n->child[i])) return false;
                }
                return true;
            }
            return false;
        }

    } // namespace SimpUtil

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

    Exptree* TreeSimplifier::simplifyNode(Exptree* node) {
        if (!node) return nullptr;
        if (node->valtp == Exptree::val_t::valRational ||
            node->valtp == Exptree::val_t::valVariable) {
            return node;
        }
        if (node->valtp != Exptree::val_t::valFunction) return node;

        for (size_t i = 0; i < node->child.size(); ++i) {
            node->child[i] = simplifyNode(node->child[i]);
        }

        const std::string& func = node->var;

        // Arithmetic
        if (func == "+") return simplifyAdd(node);
        if (func == "*") return simplifyMul(node);
        if (func == "^") return simplifyPow(node);

        // Trigonometric
        if (func == FuncName::sin)   return simplifySin(node);
        if (func == FuncName::cos)   return simplifyCos(node);
        if (func == FuncName::tan)   return simplifyTan(node);
        if (func == FuncName::asin)  return simplifyAsin(node);
        if (func == FuncName::acos)  return simplifyAcos(node);
        if (func == FuncName::atan)  return simplifyAtan(node);

        // Hyperbolic (now converted to exponential form in preTransform, 
        // but we keep the handlers for cases where preTransform wasn't applied)
        if (func == FuncName::sinh)  return simplifySinh(node);
        if (func == FuncName::cosh)  return simplifyCosh(node);
        if (func == FuncName::tanh)  return simplifyTanh(node);
        if (func == FuncName::asinh) return simplifyAsinh(node);
        if (func == FuncName::acosh) return simplifyAcosh(node);
        if (func == FuncName::atanh) return simplifyAtanh(node);

        // Logarithmic
        if (func == FuncName::ln)    return simplifyLn(node);
        if (func == FuncName::log)   return simplifyLog(node);
        if (func == FuncName::log10) return simplifyLog10(node);

        // Other elementary functions
        if (func == FuncName::abs)   return simplifyAbs(node);
        if (func == FuncName::sign)  return simplifySignum(node);
        if (func == FuncName::fact)  return simplifyFact(node);

        // Angle conversion functions (if not pre-transformed)
        if (func == FuncName::deg)   return simplifyDeg(node);
        if (func == FuncName::rad)   return simplifyRad(node);

        return node;
    }

    Exptree* TreeSimplifier::simplifyTrigSpecialAngles(Exptree* node, const char* funcName) {
        if (node->child.size() != 1) return nullptr;
        Exptree* arg = node->child[0];
        
        // Handle zero
        if (SimpUtil::isZero(arg)) {
            if (std::strcmp(funcName, "sin") == 0 || std::strcmp(funcName, "tan") == 0)
                return SimpUtil::makeRational(Rational(Intg(0)));
            if (std::strcmp(funcName, "cos") == 0)
                return SimpUtil::makeRational(Rational(Intg(1)));
            return nullptr;
        }
        
        // Handle pi
        if (SimpUtil::isConstantPi(arg)) {
            if (std::strcmp(funcName, "sin") == 0 || std::strcmp(funcName, "tan") == 0)
                return SimpUtil::makeRational(Rational(Intg(0)));
            if (std::strcmp(funcName, "cos") == 0)
                return SimpUtil::makeRational(Rational(Intg(-1)));
            return nullptr;
        }
        
        // Handle k*pi patterns
        if (SimpUtil::isFunction(arg, "*")) {
            bool hasPi = false;
            Rational coeff(Intg(0));
            
            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isConstantPi(arg->child[i])) {
                    hasPi = true;
                } else if (SimpUtil::isRational(arg->child[i])) {
                    coeff = coeff + arg->child[i]->value; // Simplified: multiply fractions
                }
            }
            
            if (hasPi && coeff > Rational(Intg(0))) {
                // Normalize coefficient to [0, 2]
                // coeff represents k in k*pi
                bool isSin = (std::strcmp(funcName, "sin") == 0);
                bool isCos = (std::strcmp(funcName, "cos") == 0);
                bool isTan = (std::strcmp(funcName, "tan") == 0);
                
                // sin(pi/2) = 1, cos(pi/2) = 0
                if (coeff == Rational(Intg(1), Intg(2))) {
                    if (isSin) return SimpUtil::makeRational(Rational(Intg(1)));
                    if (isCos || isTan) return SimpUtil::makeRational(Rational(Intg(0)));
                }
                // sin(pi/3) = sqrt(3)/2, cos(pi/3) = 1/2, tan(pi/3) = sqrt(3)
                if (coeff == Rational(Intg(1), Intg(3))) {
                    if (isSin) {
                        Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                        sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                        Exptree* result = SimpUtil::makeFunction("/");
                        result->child.push_back(sqrt3);
                        result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                        return result;
                    }
                    if (isCos) return SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
                    if (isTan) {
                        Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                        sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                        return sqrt3;
                    }
                }
                // sin(pi/4) = cos(pi/4) = sqrt(2)/2, tan(pi/4) = 1
                if (coeff == Rational(Intg(1), Intg(4))) {
                    if (isSin || isCos) {
                        Exptree* sqrt2 = SimpUtil::makeFunction(FuncName::sqrt);
                        sqrt2->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                        Exptree* result = SimpUtil::makeFunction("/");
                        result->child.push_back(sqrt2);
                        result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                        return result;
                    }
                    if (isTan) return SimpUtil::makeRational(Rational(Intg(1)));
                }
                // sin(pi/6) = 1/2, cos(pi/6) = sqrt(3)/2, tan(pi/6) = 1/sqrt(3)
                if (coeff == Rational(Intg(1), Intg(6))) {
                    if (isSin) return SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
                    if (isCos) {
                        Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                        sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                        Exptree* result = SimpUtil::makeFunction("/");
                        result->child.push_back(sqrt3);
                        result->child.push_back(SimpUtil::makeRational(Rational(Intg(2))));
                        return result;
                    }
                    if (isTan) {
                        Exptree* sqrt3 = SimpUtil::makeFunction(FuncName::sqrt);
                        sqrt3->child.push_back(SimpUtil::makeRational(Rational(Intg(3))));
                        Exptree* result = SimpUtil::makeFunction("/");
                        result->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                        result->child.push_back(sqrt3);
                        return result;
                    }
                }
                // sin(pi/5) = sqrt(10-2*sqrt(5))/4  (sin 36°)
                // cos(pi/5) = (sqrt(5)+1)/4 = phi/2  (cos 36°)
                if (coeff == Rational(Intg(1), Intg(5))) {
                    if (isCos) {
                        Exptree* sqrt5 = SimpUtil::makeFunction(FuncName::sqrt);
                        sqrt5->child.push_back(SimpUtil::makeRational(Rational(Intg(5))));
                        Exptree* one = SimpUtil::makeRational(Rational(Intg(1)));
                        Exptree* sum = SimpUtil::makeFunction("+");
                        sum->child.push_back(sqrt5);
                        sum->child.push_back(one);
                        Exptree* result = SimpUtil::makeFunction("/");
                        result->child.push_back(sum);
                        result->child.push_back(SimpUtil::makeRational(Rational(Intg(4))));
                        return result;
                    }
                }
                // sin(pi/10) = (sqrt(5)-1)/4  (sin 18°)
                // cos(pi/10) = sqrt(10+2*sqrt(5))/4  (cos 18°)
                if (coeff == Rational(Intg(1), Intg(10))) {
                    if (isSin) {
                        Exptree* sqrt5 = SimpUtil::makeFunction(FuncName::sqrt);
                        sqrt5->child.push_back(SimpUtil::makeRational(Rational(Intg(5))));
                        Exptree* negOne = SimpUtil::makeFunction("*");
                        negOne->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                        negOne->child.push_back(SimpUtil::makeRational(Rational(Intg(1))));
                        Exptree* sum = SimpUtil::makeFunction("+");
                        sum->child.push_back(sqrt5);
                        sum->child.push_back(negOne);
                        Exptree* result = SimpUtil::makeFunction("/");
                        result->child.push_back(sum);
                        result->child.push_back(SimpUtil::makeRational(Rational(Intg(4))));
                        return result;
                    }
                }
            }
        }
        
        return nullptr;
    }

    Exptree* TreeSimplifier::simplifySin(Exptree* node) {
        // Try special angles first
        Exptree* special = simplifyTrigSpecialAngles(node, "sin");
        if (special) {
            SimpUtil::freeTree(node);
            return special;
        }
        
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        // sin(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // sin(-x) = -sin(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner = nullptr;
                if (arg->child.size() == 2) {
                    inner = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }
                Exptree* innerSin = SimpUtil::makeFunction(FuncName::sin);
                innerSin->child.push_back(inner);
                innerSin = simplifySin(innerSin);
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                result->child.push_back(innerSin);
                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        return node;
    }

    Exptree* TreeSimplifier::simplifyCos(Exptree* node) {
        Exptree* special = simplifyTrigSpecialAngles(node, "cos");
        if (special) {
            SimpUtil::freeTree(node);
            return special;
        }
        
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // cos(-x) = cos(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner = nullptr;
                if (arg->child.size() == 2) {
                    inner = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }
                Exptree* newCos = SimpUtil::makeFunction(FuncName::cos);
                newCos->child.push_back(inner);
                SimpUtil::freeTree(node);
                return simplifyCos(newCos);
            }
        }

        return node;
    }

    Exptree* TreeSimplifier::simplifyTan(Exptree* node) {
        Exptree* special = simplifyTrigSpecialAngles(node, "tan");
        if (special) {
            SimpUtil::freeTree(node);
            return special;
        }
        
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];

        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // tan(-x) = -tan(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner = nullptr;
                if (arg->child.size() == 2) {
                    inner = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }
                Exptree* innerTan = SimpUtil::makeFunction(FuncName::tan);
                innerTan->child.push_back(inner);
                innerTan = simplifyTan(innerTan);
                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                result->child.push_back(innerTan);
                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        return node;
    }

    Exptree* TreeSimplifier::simplifyAsin(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(half);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1), Intg(2))) {
            SimpUtil::freeTree(node);
            Exptree* sixth = SimpUtil::makeRational(Rational(Intg(1), Intg(6)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(sixth);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }
        return node;
    }

    Exptree* TreeSimplifier::simplifyAcos(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(half);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1), Intg(2))) {
            SimpUtil::freeTree(node);
            Exptree* third = SimpUtil::makeRational(Rational(Intg(1), Intg(3)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(third);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }
        return node;
    }

    Exptree* TreeSimplifier::simplifyAtan(Exptree* node) {
        if (node->child.size() != 1) return node;
        Exptree* arg = node->child[0];
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            Exptree* quarter = SimpUtil::makeRational(Rational(Intg(1), Intg(4)));
            Exptree* result = SimpUtil::makeFunction("*");
            result->child.push_back(quarter);
            result->child.push_back(SimpUtil::makeVariable(ConstName::pi));
            return result;
        }
        return node;
    }

    Exptree* TreeSimplifier::simplifySinh(Exptree* node) {
        // Already converted to exponential form in preTransform
        // Apply preTransform again if needed
        preTransform(node);
        return simplifyNode(node);
    }

    Exptree* TreeSimplifier::simplifyCosh(Exptree* node) {
        preTransform(node);
        return simplifyNode(node);
    }

    Exptree* TreeSimplifier::simplifyTanh(Exptree* node) {
        preTransform(node);
        return simplifyNode(node);
    }

    Exptree* TreeSimplifier::simplifyAsinh(Exptree* node) {
        preTransform(node);
        return simplifyNode(node);
    }

    Exptree* TreeSimplifier::simplifyAcosh(Exptree* node) {
        preTransform(node);
        return simplifyNode(node);
    }

    Exptree* TreeSimplifier::simplifyAtanh(Exptree* node) {
        preTransform(node);
        return simplifyNode(node);
    }

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
        
        // log10(100) = 2
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(100))) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(2)));
        }
        
        // log10(1000) = 3
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1000))) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(3)));
        }
        
        // log10(1/10) = -1
        if (SimpUtil::isRational(arg) && arg->value == Rational(Intg(1), Intg(10))) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(-1)));
        }
        
        // Convert to natural log: log10(x) = ln(x)/ln(10)
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

    Exptree* TreeSimplifier::simplifyDeg(Exptree* node) {
        // Already transformed in preTransform
        preTransform(node);
        return simplifyNode(node);
    }

    Exptree* TreeSimplifier::simplifyRad(Exptree* node) {
        // Already transformed in preTransform
        preTransform(node);
        return simplifyNode(node);
    }

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
                    if (base_j != term_j->child[1]) {
                        SimpUtil::freeTree(base_j);
                    }
                } else {
                    if (base_j != term_j->child[1]) {
                        SimpUtil::freeTree(base_j);
                    }
                }
            }

            // Rebuild term with merged coefficient
            if (coeff_i.isZero()) {
                SimpUtil::freeTree(buf.items[i]);
                buf.items[i] = nullptr;
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

    void TreeSimplifier::sortItems(Exptree** items, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            for (size_t j = i + 1; j < count; ++j) {
                if (SimpUtil::compareNodes(items[i], items[j]) > 0) {
                    Exptree* tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }
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

    bool TreeSimplifier::isPerfectSquare(const Rational& r, Rational& root) {
        if (const_cast<Rational&>(r) < Rational(Intg(0))) return false;

        Intg num = const_cast<Rational&>(r).numerator();
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

    Exptree* TreeSimplifier::simplifySqrt(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // sqrt(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // sqrt(1) = 1
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // sqrt of perfect square rational
        if (SimpUtil::isRational(arg)) {
            Rational root;
            if (isPerfectSquare(arg->value, root)) {
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(root);
            }

            // Extract perfect square factors from numerator and denominator
            if (SimpUtil::isPositive(arg)) {
                Intg num = arg->value.numerator();
                Intg den = arg->value.den;
                Rational outside(Intg(1));
                Rational inside(Intg(1));

                // Try to extract squares from numerator
                Intg sqrtNum = num.sqrt(num);
                if (sqrtNum * sqrtNum == num) {
                    // num is perfect square
                    outside = Rational(sqrtNum, Intg(1));
                    num = Intg(1);
                }

                // Try to extract squares from denominator
                Intg sqrtDen = den.sqrt(den);
                if (sqrtDen * sqrtDen == den) {
                    // den is perfect square
                    Rational denFactor(Intg(1), sqrtDen);
                    outside = outside * denFactor;
                    den = Intg(1);
                }

                if (outside != Rational(Intg(1))) {
                    Rational remaining(num, den);
                    if (remaining == Rational(Intg(1))) {
                        SimpUtil::freeTree(node);
                        return SimpUtil::makeRational(outside);
                    }

                    Exptree* result = SimpUtil::makeFunction("*");
                    result->child.push_back(SimpUtil::makeRational(outside));

                    Exptree* innerSqrt = SimpUtil::makeFunction(FuncName::sqrt);
                    innerSqrt->child.push_back(SimpUtil::makeRational(remaining));
                    result->child.push_back(innerSqrt);

                    SimpUtil::freeTree(node);
                    return simplifyMul(result);
                }
            }
        }

        // sqrt(x^2) = abs(x)
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isRational(arg->child[1]) && arg->child[1]->value == Rational(Intg(2))) {
                Exptree* absNode = SimpUtil::makeFunction(FuncName::abs);
                absNode->child.push_back(SimpUtil::deepCopy(arg->child[0]));
                SimpUtil::freeTree(node);
                return simplifyAbs(absNode);
            }
        }

        // sqrt(-1) = i, sqrt(-a) = i*sqrt(a)
        // Build as power and let simplifyPow handle it
        Exptree* half = SimpUtil::makeRational(Rational(Intg(1), Intg(2)));
        node->var = "^";
        node->child.clear();
        node->child.push_back(SimpUtil::deepCopy(arg));
        node->child.push_back(half);
        return simplifyPow(node);
    }

    Exptree* TreeSimplifier::simplifyAbs(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // abs(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // abs(abs(x)) = abs(x)
        if (SimpUtil::isFunction(arg, FuncName::abs)) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // abs(n) = n for n > 0
        if (SimpUtil::isPositive(arg)) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // abs(-n) = n for n < 0
        if (SimpUtil::isNegative(arg)) {
            Rational posVal = Rational(Intg(0)) - arg->value;
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(posVal);
        }

        // abs(-x) = abs(x) where -x is represented as (-1)*x
        if (SimpUtil::isFunction(arg, "*")) {
            if (!arg->child.empty() && SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* remaining = SimpUtil::makeFunction("*");
                for (size_t i = 1; i < arg->child.size(); ++i) {
                    remaining->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                }
                if (arg->child.size() == 2) {
                    SimpUtil::freeTree(remaining);
                    remaining = SimpUtil::deepCopy(arg->child[1]);
                }

                Exptree* newAbs = SimpUtil::makeFunction(FuncName::abs);
                newAbs->child.push_back(remaining);
                SimpUtil::freeTree(node);
                return simplifyAbs(newAbs);
            }
        }

        // abs(x*y) = abs(x)*abs(y)
        if (SimpUtil::isFunction(arg, "*")) {
            Exptree* result = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < arg->child.size(); ++i) {
                if (SimpUtil::isRational(arg->child[i])) {
                    Rational val = arg->child[i]->value;
                    if (val < Rational(Intg(0))) {
                        val = Rational(Intg(0)) - val;
                    }
                    result->child.push_back(SimpUtil::makeRational(val));
                } else {
                    Exptree* absChild = SimpUtil::makeFunction(FuncName::abs);
                    absChild->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    result->child.push_back(absChild);
                }
            }
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // abs(x^n) = abs(x)^n for even integer n
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isEvenInteger(arg->child[1])) {
                Exptree* absBase = SimpUtil::makeFunction(FuncName::abs);
                absBase->child.push_back(SimpUtil::deepCopy(arg->child[0]));

                Exptree* result = SimpUtil::makeFunction("^");
                result->child.push_back(absBase);
                result->child.push_back(SimpUtil::deepCopy(arg->child[1]));

                SimpUtil::freeTree(node);
                return result;
            }
        }

        return node;
    }

    Exptree* TreeSimplifier::simplifyLn(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // ln(0) -> undefined, leave as-is
        if (SimpUtil::isZero(arg)) {
            return node;
        }

        // ln(1) = 0 (exact)
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // ln(e) = 1 (exact, uses ConstName::e)
        if (SimpUtil::isConstantE(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // ln(e^x) = x
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isConstantE(arg->child[0])) {
                Exptree* inner = SimpUtil::deepCopy(arg->child[1]);
                SimpUtil::freeTree(node);
                return inner;
            }
        }

        // ln(x^a) = a*ln(x) for x > 0
        if (SimpUtil::isFunction(arg, "^") && arg->child.size() == 2) {
            if (SimpUtil::isPositive(arg->child[0])) {
                Exptree* a = SimpUtil::deepCopy(arg->child[1]);
                Exptree* lnBase = SimpUtil::makeFunction(FuncName::ln);
                lnBase->child.push_back(SimpUtil::deepCopy(arg->child[0]));

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(a);
                result->child.push_back(lnBase);

                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        return node;
    }

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

    Exptree* TreeSimplifier::simplifySignum(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // signum(0) = 0
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(0)));
        }

        // signum(n) = 1 for n > 0
        if (SimpUtil::isPositive(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // signum(-n) = -1 for n < 0
        if (SimpUtil::isNegative(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(-1)));
        }

        // signum(signum(x)) = signum(x)
        if (SimpUtil::isFunction(arg, FuncName::sign)) {
            SimpUtil::freeTree(node);
            return arg;
        }

        // signum(-x) = -signum(x)
        if (SimpUtil::isFunction(arg, "*") && !arg->child.empty()) {
            if (SimpUtil::isMinusOne(arg->child[0])) {
                Exptree* inner = nullptr;
                if (arg->child.size() == 2) {
                    inner = SimpUtil::deepCopy(arg->child[1]);
                } else {
                    inner = SimpUtil::makeFunction("*");
                    for (size_t i = 1; i < arg->child.size(); ++i) {
                        inner->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                    }
                }

                Exptree* innerSign = SimpUtil::makeFunction(FuncName::sign);
                innerSign->child.push_back(inner);
                innerSign = simplifySignum(innerSign);

                Exptree* result = SimpUtil::makeFunction("*");
                result->child.push_back(SimpUtil::makeRational(Rational(Intg(-1))));
                result->child.push_back(innerSign);

                SimpUtil::freeTree(node);
                return simplifyMul(result);
            }
        }

        // signum(x*y) = signum(x)*signum(y)
        if (SimpUtil::isFunction(arg, "*")) {
            Exptree* result = SimpUtil::makeFunction("*");
            for (size_t i = 0; i < arg->child.size(); ++i) {
                Exptree* signChild = SimpUtil::makeFunction(FuncName::sign);
                signChild->child.push_back(SimpUtil::deepCopy(arg->child[i]));
                result->child.push_back(signChild);
            }
            SimpUtil::freeTree(node);
            return simplifyMul(result);
        }

        // signum(abs(x)) = 1 (for x != 0)
        if (SimpUtil::isFunction(arg, FuncName::abs)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        return node;
    }

    Exptree* TreeSimplifier::simplifyFact(Exptree* node) {
        if (node->child.size() != 1) return node;

        Exptree* arg = node->child[0];

        // 0! = 1
        if (SimpUtil::isZero(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // 1! = 1
        if (SimpUtil::isOne(arg)) {
            SimpUtil::freeTree(node);
            return SimpUtil::makeRational(Rational(Intg(1)));
        }

        // n! for small positive integers
        if (SimpUtil::isInteger(arg) && SimpUtil::isPositive(arg)) {
            Intg n = arg->value.numerator();
            if (n <= Intg(20)) {
                Intg result(1);
                Intg i(2);
                while (i <= n) {
                    result = result * i;
                    i = i + Intg(1);
                }
                SimpUtil::freeTree(node);
                return SimpUtil::makeRational(Rational(result));
            }
        }

        return node;
    }

} // namespace CAS

#endif // _CAS_TREESIMP_HPP_