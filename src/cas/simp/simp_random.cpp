/** @file /src/cas/simp/simp_random.cpp
 *  @brief Random number function simplifiers (pass-through)
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

    // ========== Randrat ==========

    Exptree* TreeSimplifier::simplifyRandrat(Exptree* node) {
        // Random functions have no algebraic simplification.
        // Simply return node as-is. Children have already been simplified
        // by simplifyNode before dispatching here.
        return node;
    }

    // ========== Randint ==========

    Exptree* TreeSimplifier::simplifyRandint(Exptree* node) {
        // Random functions have no algebraic simplification.
        // Simply return node as-is.
        return node;
    }

} // namespace CAS