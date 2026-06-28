/** @file /inc/cas/simp/exptreepool.hpp
 *  @brief Fixed-size object pool for Exptree nodes
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

#ifndef _CAS_SIMP_EXPTREEPOOL_HPP_
#define _CAS_SIMP_EXPTREEPOOL_HPP_

#include "cas/exptree.hpp"
#include <cstddef>

namespace CAS {

    /**
     *  @class ExptreePool
     *  @brief Fixed-size object pool for Exptree nodes
     *  @details Pre-allocates 48 Exptree nodes (~10KB SRAM) to avoid dynamic
     *           allocation during simplification. Falls back to heap allocation
     *           via operator new when exhausted. Deallocation automatically
     *           detects pool vs heap origin.
     *
     *           Designed for the RP2040 microcontroller's memory constraints.
     */
    class ExptreePool {
        /** @brief Number of pre-allocated nodes (48) */
        static constexpr size_t POOL_SIZE = 48;
        /** @brief Pre-reserved child pointer capacity per node (6) */
        static constexpr size_t MAX_CHILDREN = 6;

        /** @brief Internal pool entry: node + used flag */
        struct PoolNode {
            Exptree node;       ///< The expression tree node
            bool used;          ///< Allocation status
            /** @brief Construct as free, pre-reserve child capacity */
            PoolNode() : used(false) { node.child.reserve(MAX_CHILDREN); }
        };

        PoolNode pool_[POOL_SIZE];  ///< Fixed array of pool entries

    public:
        /**
         *  @name allocate
         *  @brief Allocate a node from pool or heap
         *  @return Pointer to initialized Exptree node (valNull, value 0)
         *  @details O(POOL_SIZE) linear search, effectively O(1).
         */
        Exptree* allocate();

        /**
         *  @name deallocate
         *  @brief Recursively free node and descendants
         *  @param node Root of subtree to free, nullptr safe
         *  @details Children freed depth-first. Pool nodes reset to free;
         *           heap nodes deleted.
         */
        void deallocate(Exptree* node);

        /**
         *  @name available
         *  @brief Query free pool entries remaining
         *  @return Count of unused pool slots (excludes heap fallback)
         */
        size_t available() const;
    };

    /** @brief Global pool instance, defined in simplify.cpp */
    extern ExptreePool g_pool;

    // ========== Inline implementations ==========

    inline Exptree* ExptreePool::allocate() {
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
        Exptree* fallback = new Exptree();
        fallback->child.reserve(MAX_CHILDREN);
        return fallback;
    }

    inline void ExptreePool::deallocate(Exptree* node) {
        if (!node) return;
        for (size_t i = 0; i < node->child.size(); ++i) {
            deallocate(node->child[i]);
        }
        node->child.clear();
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            if (&pool_[i].node == node) {
                pool_[i].used = false;
                return;
            }
        }
        delete node;
    }

    inline size_t ExptreePool::available() const {
        size_t count = 0;
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            if (!pool_[i].used) count++;
        }
        return count;
    }

} // namespace CAS

#endif // _CAS_SIMP_EXPTREEPOOL_HPP_