/** @file /inc/cas/expand.hpp
 *  @brief Polynomial expansion for the computer algebra system module
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

#ifndef _CAS_SIMP_EXPAND_HPP_
#define _CAS_SIMP_EXPAND_HPP_

#include "cas/treesimp.hpp"

namespace CAS {

    class TreeExpander {
        public:
            /**
             *  @name expand
             *  @brief Expand polynomial expressions using distributive law
             *  @param root Root of expression tree
             *  @return Expanded expression
             */
            static Exptree* expand(Exptree*& root);

        private:
            TreeExpander() = default;

            static TreeExpander& instance();

            Exptree* expandNode(Exptree* node);
            Exptree* expandMul(Exptree* node);
            Exptree* expandPow(Exptree* node);
    };

} // namespace CAS

#endif