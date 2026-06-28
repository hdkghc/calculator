/** @file /inc/cas/simp/simp_internal.hpp
 *  @brief Internal types for the expression tree simplifier
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

#ifndef _CAS_SIMP_INTERNAL_HPP_
#define _CAS_SIMP_INTERNAL_HPP_

#include "cas/simp/simputil.hpp"
#include <cstddef>

namespace CAS {

    /**
     *  @struct WorkBuffer
     *  @brief Reusable fixed-size buffer for term/factor collection
     *  @details Avoids heap allocation during recursive simplification.
     *           Each TreeSimplifier instance owns two buffers (addBuf_, mulBuf_).
     *           Holds up to 24 items with an accumulated rational constant.
     */
    struct WorkBuffer {
        /** @brief Max items the buffer can hold */
        static constexpr size_t MAX_ITEMS = 24;

        /** @brief Array of owned expression pointers */
        Exptree* items[MAX_ITEMS];
        /** @brief Current number of valid items */
        size_t count;
        /** @brief Accumulated constant (sum for add, product for mul) */
        Rational constantAccum;

        /** @brief Reset for addition: count=0, constant=0 */
        void resetAdd() {
            count = 0;
            constantAccum = Rational(Intg(0));
        }

        /** @brief Reset for multiplication: count=0, constant=1 */
        void resetMul() {
            count = 0;
            constantAccum = Rational(Intg(1));
        }

        /**
         *  @brief Add item, transferring ownership
         *  @return false if buffer full (caller retains ownership)
         */
        bool addItem(Exptree* item) {
            if (count >= MAX_ITEMS) return false;
            items[count++] = item;
            return true;
        }

        /** @brief Free all items and reset count */
        void clearItems() {
            for (size_t i = 0; i < count; ++i) {
                if (items[i]) {
                    SimpUtil::freeTree(items[i]);
                    items[i] = nullptr;
                }
            }
            count = 0;
        }
    };

} // namespace CAS

#endif // _CAS_SIMP_INTERNAL_HPP_