/** @file /inc/keypad_handler.hpp
 *  @brief Keypad input handler — builds internal expression strings from key presses
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

#ifndef _KEYPAD_HANDLER_HPP_
#define _KEYPAD_HANDLER_HPP_

#include "keypad.hpp"
#include <string>
#include <cstddef>
#include <cstdint>

namespace Keypad {

    constexpr uint8_t FLAG_SHIFT  = 0x01;
    constexpr uint8_t FLAG_ALPHA  = 0x02;
    constexpr uint8_t FLAG_CTRL   = 0x04;
    constexpr uint8_t FLAG_LOCK   = 0x08;
    constexpr uint8_t FLAG_INSERT = 0x10;

    enum FuncType : int8_t {
        FT_DIGIT = 0,
        FT_VAR_CONST,
        FT_OP_BINARY,
        FT_OP_UNARY_PRE,
        FT_OP_UNARY_POST,
        FT_OP_BINARY_FUNC,
        FT_OP_MULTI,
        FT_OP_INDEXED,
        FT_OP_GRAPH,
        FT_OP_VOID,
        FT_OP_RANDINT,
        FT_CONTROL,
    };

    struct FuncInfo {
        const char* internal;
        int8_t type;
        int8_t numArgs;
    };

    inline const FuncInfo& lookupFunc(const std::string& key) {
        static const FuncInfo funcs[] = {
            {CAS::FuncName::sin,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::cos,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::tan,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::asin,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::acos,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::atan,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::sinh,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::cosh,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::tanh,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::asinh,     FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::acosh,     FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::atanh,     FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::ln,        FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::log10,     FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::exp,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::sqrt,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::abs,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::floor,     FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::ceil,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::round,     FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::sign,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::fact,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::deg,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::rad,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::norm,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::det,       FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::transpose, FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::eigenval,  FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::eigenvec,  FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::adjoint,   FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::rank,      FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::realpart,  FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::imagpart,  FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::conjg,     FT_OP_UNARY_PRE, 1},
            {CAS::FuncName::arg,       FT_OP_UNARY_PRE, 1},

            {Spec::SQ,                 FT_OP_UNARY_POST, 0},
            {Spec::CB,                 FT_OP_UNARY_POST, 0},
            {Spec::INV,                FT_OP_UNARY_POST, 0},
            {Spec::CBRT,               FT_OP_UNARY_POST, 0},

            {CAS::FuncName::log,       FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::root,      FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::gcd,       FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::lcm,       FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::mod,       FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::permut,    FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::combin,    FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::polar,     FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::rect,      FT_OP_BINARY_FUNC, 2},
            {CAS::FuncName::dot,       FT_OP_BINARY, 2},
            {CAS::FuncName::angle,     FT_OP_BINARY_FUNC, 2},

            {CAS::FuncName::max,       FT_OP_MULTI, -2},
            {CAS::FuncName::min,       FT_OP_MULTI, -2},

            {CAS::FuncName::sum,       FT_OP_INDEXED, 4},
            {CAS::FuncName::prod,      FT_OP_INDEXED, 4},
            {CAS::FuncName::defint,    FT_OP_INDEXED, 4},
            {CAS::FuncName::diff,      FT_OP_INDEXED, 3},
            {CAS::FuncName::indefint,  FT_OP_INDEXED, 2},
            {CAS::FuncName::vector,    FT_OP_INDEXED, -1},
            {CAS::FuncName::matrix,    FT_OP_INDEXED, 2},

            {CAS::FuncName::randrat,   FT_OP_VOID, 0},
            {CAS::FuncName::randint,   FT_OP_RANDINT, 2},

            {CAS::FuncName::plot2d,    FT_OP_GRAPH, 7},
            {CAS::FuncName::plot3d,    FT_OP_GRAPH, 10},
        };
        static const FuncInfo unknown = {"", FT_DIGIT, 0};
        for (const auto& f : funcs) if (f.internal == key) return f;
        return unknown;
    }

    inline uint8_t tokenLen(char c) {
        uint8_t uc = static_cast<uint8_t>(c);
        return (uc >= 0x01 && uc <= 0x05) ? 2 : 1;
    }
    inline uint8_t isTokenStart(char c) {
        uint8_t uc = static_cast<uint8_t>(c);
        return (uc >= 0x01 && uc <= 0x05) ? 1 : 0;
    }

    class ExpressionBuilder {
    public:
        ExpressionBuilder() : m_cursor(0), m_insert(0) {}

        void setInsert(uint8_t on) { m_insert = on; }
        uint8_t insert() const { return m_insert; }
        void setExpr(const std::string& e) { m_expr = e; m_cursor = e.size(); }
        const std::string& expr() const { return m_expr; }
        uint16_t cursor() const { return m_cursor; }
        void setCursor(uint16_t p) { if (p <= m_expr.size()) m_cursor = p; }

        // ========== Token-level delete ==========
        /**
         *  @brief Delete one token left of cursor.
         *  @details sin( 被视为一个整体（函数名+左括号），一起删除。
         */
        void del() {
            if (m_cursor == 0) return;
            uint16_t p = m_cursor;

            // Check for BLOCKR at cursor-1 — delete matching BLOCKL..BLOCKR block
            if (p >= 2 && m_expr[p-2] == Ctrl::BLOCKL[0] && m_expr[p-1] == Ctrl::BLOCKL[1]) {
                // Cursor is after BLOCKL, find BLOCKR and delete whole block
                uint16_t end = p;
                while (end < m_expr.size() && !(m_expr[end] == Ctrl::BLOCKR[0] && m_expr[end+1] == Ctrl::BLOCKR[1])) {
                    end += isTokenStart(m_expr[end]) ? tokenLen(m_expr[end]) : 1;
                }
                if (end < m_expr.size()) end += 2;
                m_expr.erase(p, end - p);
                return;
            }

            // Check for function name before '(' — delete func + '(' together
            if (m_expr[p-1] == '(') {
                uint16_t start = p - 1;
                while (start > 0) {
                    char c = m_expr[start-1];
                    if (isTokenStart(c)) { start -= tokenLen(c); }
                    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { start--; }
                    else break;
                }
                if (start < p - 1) {
                    // Function name found — delete func + '('
                    m_expr.erase(start, p - start);
                    m_cursor = start;
                    return;
                }
            }

            // Default: delete one token
            if (isTokenStart(m_expr[p-1])) {
                uint8_t len = tokenLen(m_expr[p-1]);
                m_expr.erase(p - len, len);
                m_cursor -= len;
            } else {
                m_expr.erase(p - 1, 1);
                m_cursor--;
            }
        }

        void ac() { m_expr.clear(); m_cursor = 0; }

        void moveLeft() {
            if (m_cursor == 0) return;
            if (isTokenStart(m_expr[m_cursor-1])) m_cursor -= tokenLen(m_expr[m_cursor-1]);
            else m_cursor--;
        }
        void moveRight() {
            if (m_cursor >= m_expr.size()) return;
            if (isTokenStart(m_expr[m_cursor])) m_cursor += tokenLen(m_expr[m_cursor]);
            else m_cursor++;
        }

        void insertStr(const std::string& s) {
            if (m_insert && m_cursor < m_expr.size()) {
                uint16_t p = m_cursor;
                uint8_t oldLen = isTokenStart(m_expr[p]) ? tokenLen(m_expr[p]) : 1;
                m_expr.replace(p, oldLen, s);
            } else {
                m_expr.insert(m_cursor, s);
            }
            m_cursor += s.size();
        }

        // ========== Infix binary operator ( + - * / ^ dot ) ==========
        void pressInfixOp(const std::string& op) {
            insertStr(op);
        }

        // ========== Unary prefix with auto parens ==========
        /**
         *  @brief Insert sin(, cos(, sqrt(, etc.
         *  @details Function name + '(' inserted together. DEL removes both.
         */
        void pressUnaryPrefix(const std::string& fn) {
            std::string s = fn + "(";
            insertStr(s);
        }

        // ========== Unary postfix ^2 ^3 ^-1 ==========
        void pressUnaryPostfix(const std::string& spec) {
            if (spec == Spec::SQ)       insertStr("^2");
            else if (spec == Spec::CB)  insertStr("^3");
            else if (spec == Spec::INV) insertStr("^(-1)");
            else if (spec == Spec::CBRT) insertStr("cbrt(");
        }

        // ========== Binary function with templates ==========
        /**
         *  @brief Insert log_(a)(b) style template using BLOCKL/BLOCKR.
         *  @details log -> log_[BLOCKL]a[BLOCKR](BLOCKL)b[BLOCKR]
         *           User sees two input boxes. Cursor placed in first box (a).
         *           Internal log(2,8) means log_8(2), but user enters log(8,2).
         *           Here user sees: log[8](2) with 8 in first box, 2 in second.
         */
        void pressLogTemplate() {
            std::string tmpl;
            tmpl += CAS::FuncName::log;
            tmpl += "(";
            tmpl += Ctrl::BLOCKL;  // first input box: base
            tmpl += Ctrl::BLOCKR;
            tmpl += ",";
            tmpl += Ctrl::BLOCKL;  // second input box: argument
            tmpl += Ctrl::BLOCKR;
            tmpl += ")";
            insertStr(tmpl);
            // Cursor goes to first input box (after BLOCKL, before BLOCKR)
            m_cursor -= tmpl.size();
            m_cursor += 2 + 1 + 2; // log + ( + BLOCKL
        }

        /**
         *  @brief Insert root[n](x) template
         */
        void pressRootTemplate() {
            std::string tmpl;
            tmpl += CAS::FuncName::root;
            tmpl += "(";
            tmpl += Ctrl::BLOCKL;
            tmpl += Ctrl::BLOCKR;
            tmpl += ",";
            tmpl += Ctrl::BLOCKL;
            tmpl += Ctrl::BLOCKR;
            tmpl += ")";
            insertStr(tmpl);
            m_cursor -= tmpl.size();
            m_cursor += 2 + 1 + 2; // root( + BLOCKL
        }

        /**
         *  @brief Insert power ^(base,exp) template with input boxes
         */
        void pressPowerTemplate() {
            std::string tmpl = "^(";
            tmpl += Ctrl::BLOCKL;
            tmpl += Ctrl::BLOCKR;
            tmpl += ",";
            tmpl += Ctrl::BLOCKL;
            tmpl += Ctrl::BLOCKR;
            tmpl += ")";
            insertStr(tmpl);
            m_cursor -= tmpl.size();
            m_cursor += 2 + 2; // ^( + BLOCKL
        }

        /**
         *  @brief Insert fraction template (using internal / but displayed as fraction)
         */
        void pressFractionTemplate() {
            std::string tmpl = "/(";
            tmpl += Ctrl::BLOCKL;
            tmpl += Ctrl::BLOCKR;
            tmpl += ",";
            tmpl += Ctrl::BLOCKL;
            tmpl += Ctrl::BLOCKR;
            tmpl += ")";
            insertStr(tmpl);
            m_cursor -= tmpl.size();
            m_cursor += 2 + 2; // /( + BLOCKL
        }

        /**
         *  @brief Insert indexed template: sum(from,to,exp,var)
         */
        void pressIndexed(const std::string& fn, uint8_t nBoxes) {
            std::string tmpl = fn + "(";
            for (uint8_t i = 0; i < nBoxes; ++i) {
                if (i > 0) tmpl += ",";
                tmpl += Ctrl::BLOCKL;
                tmpl += Ctrl::BLOCKR;
            }
            tmpl += ")";
            insertStr(tmpl);
            m_cursor -= tmpl.size();
            m_cursor += fn.size() + 1 + 2; // fn( + BLOCKL
        }

        /**
         *  @brief Insert multi-arg template: max/min
         */
        void pressMultiArg(const std::string& fn) {
            std::string tmpl = fn + "(";
            tmpl += Ctrl::BLOCKL;
            tmpl += Ctrl::BLOCKR;
            tmpl += ")";
            insertStr(tmpl);
            m_cursor -= tmpl.size();
            m_cursor += fn.size() + 1 + 2; // fn( + BLOCKL
        }

        // ========== Main entry point ==========

        uint8_t pressKey(uint8_t row, uint8_t col, uint8_t flags) {
            uint8_t mod = flags & 0x07;
            const std::string& key = getKey(row, col, mod);
            if (key.empty()) return 0;

            m_insert = (flags & FLAG_INSERT) ? 1 : 0;

            // Control keys — pass through
            if (key == Ctrl::EXE || key == Ctrl::ON || key == Ctrl::OFF ||
                key == Ctrl::ABOUT || key == Ctrl::MENU || key == Ctrl::MODE ||
                key == Ctrl::SET || key == Ctrl::CLR || key == Ctrl::SOLV ||
                key == Ctrl::CALC || key == Ctrl::CONST || key == Ctrl::CONV ||
                key == Ctrl::OK || key == Ctrl::LOCK || key == Ctrl::STO ||
                key == Ctrl::RCL || key == Ctrl::OPTN || key == Ctrl::FMT ||
                key == Ctrl::BLOCKL || key == Ctrl::BLOCKR ||
                key == Ctrl::ZOOM_P || key == Ctrl::ZOOM_M ||
                key == Ctrl::Z_PLUS || key == Ctrl::Z_MINUS ||
                key == Ctrl::Y_PLUS || key == Ctrl::Y_MINUS) return 1;

            if (key == Ctrl::DEL)     { del(); return 1; }
            if (key == Ctrl::AC)      { ac(); return 1; }
            if (key == Ctrl::INS)     { m_insert = !m_insert; return 1; }
            if (key == Ctrl::X_MINUS) { moveLeft(); return 1; }
            if (key == Ctrl::X_PLUS)  { moveRight(); return 1; }

            // ASCII digits
            if (key.size() == 1 && key[0] >= '0' && key[0] <= '9') { insertStr(key); return 1; }
            if (key == ".")    { insertStr("."); return 1; }
            if (key == Spec::EE) { insertStr("*10^"); return 1; }

            // Variables and constants
            if (key == "x" || key == "y" || key == "z" ||
                key == CAS::ConstName::i || key == CAS::ConstName::e ||
                key == CAS::ConstName::pi ||
                key == VarName::Ans || key == VarName::PAns ||
                key == VarName::theta || key == VarName::lambda ||
                key == VarName::mu || key == VarName::alpha) { insertStr(key); return 1; }

            // Infix binary operators (incl. dot)
            if (key == "+" || key == "-" || key == "*" || key == "/" || key == "^" ||
                key == CAS::FuncName::dot) { pressInfixOp(key); return 1; }

            // Brackets / comma
            if (key == "(" || key == ")" || key == ",") { insertStr(key); return 1; }

            // Postfix specifiers
            if (key == Spec::SQ || key == Spec::CB || key == Spec::INV || key == Spec::CBRT) {
                pressUnaryPostfix(key); return 1;
            }

            // Equals
            if (key == "=") return 1;

            // Special template keys
            if (key == CAS::FuncName::log)   { pressLogTemplate(); return 1; }
            if (key == CAS::FuncName::root)  { pressRootTemplate(); return 1; }

            // FACTOR / EXPAND
            if (key == Ctrl::FACTOR || key == Ctrl::EXPAND) { pressUnaryPrefix(key); return 1; }

            // Lookup function registry
            const FuncInfo& fi = lookupFunc(key);
            if (fi.internal[0] == '\0') { insertStr(key); return 1; }

            switch (fi.type) {
                case FT_OP_UNARY_PRE:
                    pressUnaryPrefix(fi.internal); break;
                case FT_OP_BINARY_FUNC:
                    pressIndexed(fi.internal, 2); break;
                case FT_OP_MULTI:
                    pressMultiArg(fi.internal); break;
                case FT_OP_VOID:
                    insertStr(fi.internal + std::string("()")); break;
                case FT_OP_INDEXED:
                case FT_OP_GRAPH:
                case FT_OP_RANDINT:
                    pressIndexed(fi.internal, fi.numArgs > 0 ? fi.numArgs : 1); break;
                default:
                    insertStr(fi.internal); break;
            }
            return 1;
        }

    private:
        std::string m_expr;
        uint16_t m_cursor;
        uint8_t m_insert;
    };

} // namespace Keypad

#endif // _KEYPAD_HANDLER_HPP_