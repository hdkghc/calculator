/** @file /inc/cas/simp/simputil.hpp
 *  @brief Utility functions and predicates for expression tree simplification
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

#ifndef _CAS_SIMP_SIMPUTIL_HPP_
#define _CAS_SIMP_SIMPUTIL_HPP_

#include "cas/exptree.hpp"
#include "cas/expdef.hpp"
#include "cas/simp/exptreepool.hpp"
#include <cstring>

namespace CAS {

    /**
     *  @namespace SimpUtil
     *  @brief Lightweight utility functions for tree traversal and node queries
     *  @details Provides type/value predicates, factory functions, deep copy,
     *           tree deallocation, and canonical node comparison.
     */
    namespace SimpUtil {

        // ========== Forward declarations ==========

        /** @brief Deep copy an entire expression subtree */
        Exptree* deepCopy(Exptree* src);
        /** @brief Recursively deallocate a tree via pool */
        void freeTree(Exptree* root);
        /**
         *  @brief Compare two nodes for canonical ordering
         *  @return Negative if a<b, 0 if equal, positive if a>b
         *  @details Order: Rational < Variable < Function.
         *           Rationals by value, Variables by name,
         *           Functions by name then lexicographic children.
         */
        int8_t compareNodes(Exptree* a, Exptree* b);
        /** @brief Check if tree contains only rational numbers (no variables) */
        bool isNumeric(Exptree* n);

        // ========== Node type predicates ==========

        /** @brief Check if node holds a rational value */
        inline bool isRational(Exptree* n) { return n && n->valtp == Exptree::val_t::valRational; }
        /** @brief Check if node holds a variable name */
        inline bool isVariable(Exptree* n) { return n && n->valtp == Exptree::val_t::valVariable; }
        /** @brief Check if node is a function/operator */
        inline bool isFunction(Exptree* n) { return n && n->valtp == Exptree::val_t::valFunction; }
        /** @brief Check if node is a specific named function */
        inline bool isFunction(Exptree* n, const char* name) { return isFunction(n) && n->var == name; }

        // ========== Value predicates ==========

        /** @brief Node represents rational 0 */
        inline bool isZero(Exptree* n)      { return isRational(n) && n->value.isZero(); }
        /** @brief Node represents rational 1 */
        inline bool isOne(Exptree* n)       { return isRational(n) && n->value == Rational(Intg(1)); }
        /** @brief Node represents rational -1 */
        inline bool isMinusOne(Exptree* n)  { return isRational(n) && n->value == Rational(Intg(-1)); }
        /** @brief Node is strictly positive rational */
        inline bool isPositive(Exptree* n)  { return isRational(n) && n->value > Rational(Intg(0)); }
        /** @brief Node is strictly negative rational */
        inline bool isNegative(Exptree* n)  { return isRational(n) && n->value < Rational(Intg(0)); }
        /** @brief Node is rational with denominator 1 */
        inline bool isInteger(Exptree* n)   { return isRational(n) && n->value.isInteger(); }
        /** @brief Node is an even integer rational */
        inline bool isEvenInteger(Exptree* n) {
            if (!isInteger(n)) return false;
            return (n->value.numerator() % Intg(2)) == Intg(0);
        }

        // ========== Constant predicates ==========

        /** @brief Node is the constant e */
        inline bool isConstantE(Exptree* n)   { return isVariable(n) && n->var == ConstName::e; }
        /** @brief Node is the constant pi */
        inline bool isConstantPi(Exptree* n)  { return isVariable(n) && n->var == ConstName::pi; }
        /** @brief Node is the golden ratio phi */
        inline bool isConstantPhi(Exptree* n) { return isVariable(n) && n->var == ConstName::phi; }
        /** @brief Node is the imaginary unit i */
        inline bool isConstantI(Exptree* n)   { return isVariable(n) && n->var == ConstName::i; }

        // ========== Factory functions ==========

        /** @brief Allocate and initialize a rational node */
        inline Exptree* makeRational(const Rational& v) {
            Exptree* n = g_pool.allocate();
            n->valtp = Exptree::val_t::valRational;
            n->value = v;
            return n;
        }
        /** @brief Allocate and initialize a variable node */
        inline Exptree* makeVariable(const char* name) {
            Exptree* n = g_pool.allocate();
            n->valtp = Exptree::val_t::valVariable;
            n->var = name;
            return n;
        }
        /** @brief Allocate and initialize a function/operator node */
        inline Exptree* makeFunction(const char* name) {
            Exptree* n = g_pool.allocate();
            n->valtp = Exptree::val_t::valFunction;
            n->var = name;
            return n;
        }

        // ========== Core function implementations ==========

        inline Exptree* deepCopy(Exptree* src) {
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

        inline void freeTree(Exptree* root) {
            if (root) g_pool.deallocate(root);
        }

        inline int8_t compareNodes(Exptree* a, Exptree* b) {
            if (!a && !b) return 0;
            if (!a) return -1;
            if (!b) return 1;
            if (a->valtp != b->valtp)
                return static_cast<int8_t>(a->valtp) - static_cast<int8_t>(b->valtp);
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

        inline bool isNumeric(Exptree* n) {
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

} // namespace CAS

#endif // _CAS_SIMP_SIMPUTIL_HPP_