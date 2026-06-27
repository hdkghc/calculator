/** @file /inc/cas/exptree.hpp
 *  @brief Expression tree implementation for the computer algebra system module of the calculator project
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
#ifndef _CAS_EXPTREE_HPP_
#define _CAS_EXPTREE_HPP_

#include "cas/rational.hpp"
#include "cas/intg.hpp"

namespace CAS {
    class Exptree {
        public:
            /** @name value
             *  @brief The value of the expression tree node
             */
            Rational value;
            /** @name var
             *  @brief The value of the expression tree node / function name
             */
            std::string var;
            /** @name valtp
             *  @brief The type of the value of the expression tree node
             */
            enum class val_t : uint8_t {
                valNull,        // Empty node
                valRational,    // Rational number
                valVariable,    // Variable
                valFunction     // Function (include operators)
            } valtp;
            /** @name child
             *  @brief The child of the expression tree node
             */
            std::vector<Exptree*> child;
            Exptree() : value(0), var(""), valtp(val_t::valNull) {}
            Exptree operator+(Exptree *rhs) {
                Exptree ret;
                ret.valtp = val_t::valFunction;
                ret.var = "+";
                ret.child.push_back(this);
                ret.child.push_back(rhs);
                return ret;
            }
            Exptree operator-(Exptree *rhs) {
                Exptree ret;
                ret.valtp = val_t::valFunction;
                ret.var = "-";
                ret.child.push_back(this);
                ret.child.push_back(rhs);
                return ret;
            }
            Exptree operator*(Exptree *rhs) {
                Exptree ret;
                ret.valtp = val_t::valFunction;
                ret.var = "*";
                ret.child.push_back(this);
                ret.child.push_back(rhs);
                return ret;
            }
            Exptree operator/(Exptree *rhs) {
                Exptree ret;
                ret.valtp = val_t::valFunction;
                ret.var = "/";
                ret.child.push_back(this);
                ret.child.push_back(rhs);
                return ret;
            }
            Exptree operator^(Exptree *rhs) {
                Exptree ret;
                ret.valtp = val_t::valFunction;
                ret.var = "^";
                ret.child.push_back(this);
                ret.child.push_back(rhs);
                return ret;
            }
            Exptree operator-() {
                Exptree ret;
                ret.valtp = val_t::valFunction;
                ret.var = "-";
                ret.child.push_back(this);
                return ret;
            }
    };
} // namespace CAS

#endif // _CAS_EXPTREE_HPP_