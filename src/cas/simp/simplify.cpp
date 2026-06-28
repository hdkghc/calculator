/** @file /src/cas/simp/simplify.cpp
 *  @brief Main entry point and singleton for TreeSimplifier
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

    ExptreePool g_pool;

    Exptree* TreeSimplifier::simplify(Exptree*& root) {
        if (!root) return nullptr;
        TreeSimplifier& simp = instance();
        root = simp.simplifyNode(root);
        
        return root;
    }

    TreeSimplifier& TreeSimplifier::instance() {
        static TreeSimplifier inst;
        return inst;
    }

} // namespace CAS