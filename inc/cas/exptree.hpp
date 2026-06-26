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
    /** @name SpecialVal
     *  @brief Enumeration for special values in the expression tree
     *  @details This enumeration is used to represent special mathematical constants and values in the expression
     */
    enum class SpecialVal : uint8_t {
        spNull, spPI, spE, spImag
    };

    /** @name operator_t
     *  @brief Enumeration for operators in the expression tree
     *  @details This enumeration is used to represent mathematical operators in the expression tree
     */
    enum class operator_t : uint8_t {
        opNull, opAdd, opSub, opMul, opDiv, opPow, opSqrt, opRoot, opLog, opLn, opSin, opCos, opTan, opCsc, opSec, opCot,
        opAsin, opAcos, opAtan, opAcsc, opAsec, opAcot, opSinh, opCosh, opTanh, opCsch, opSech, opCoth,
        opAsinh, opAcosh, opAtanh, opAcsch, opsech, opAcoth, opAbs, opFloor, opCeil, opRound,
        opFactorial, opGamma, opRandomRat, opRandomInt, opIntegral, opDerivative, opLimit, opSum, opProduct
    };

    /** @name ExpTreeNode
     *  @brief Structure for nodes in the expression tree
     *  @details This structure represents a node in the expression tree, which can be either a leaf node (containing a rational or special value) or an internal node (containing an operator and two child nodes)
     */
    struct ExpTreeNode {
        /** @name value value_t
         *  @brief Union for the value of the node
         *  @details This union is used to store either a rational number or a special value in the node
         */
        union value_t {
            Rational rat;
            SpecialVal spec;
            value_t() : spec(SpecialVal::spNull) {}
            ~value_t() {}
        }value;
        /** @name f
         *  @brief Flag for the node
         *  @details & 0x01 : 0 = rational, 1 = special value
         *           & 0x02 : 0 = leaf node, 1 = internal node
         */
        uint8_t f;
        ExpTreeNode* left;
        ExpTreeNode* right;
        operator_t op;
        ExpTreeNode() 
            : left(nullptr), right(nullptr), op(operator_t::opNull) { 
                f = 0x01;
        }
        ExpTreeNode(Rational val) 
            : left(nullptr), right(nullptr), op(operator_t::opNull) { 
                value.rat = val;
                f = 0x00;
        }
        ExpTreeNode(operator_t oper, ExpTreeNode *l, ExpTreeNode *r) 
            : left(l), right(r), op(oper) {
                f = 0x02;
        }
    };
} // namespace CAS

#endif // _CAS_EXPTREE_HPP_