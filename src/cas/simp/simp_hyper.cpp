/** @file /src/cas/simp/simp_hyper.cpp
 *  @brief Hyperbolic function simplifiers
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

    Exptree* TreeSimplifier::simplifySinh(Exptree* node) {
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

} // namespace CAS