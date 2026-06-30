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

    // ========== Modifier bitmask (stored in m_flags) ==========
    constexpr uint8_t MASK_SHIFT  = 0x01;
    constexpr uint8_t MASK_ALPHA  = 0x02;
    constexpr uint8_t MASK_CTRL   = 0x04;
    constexpr uint8_t MASK_LOCK   = 0x08;
    constexpr uint8_t MASK_INSERT = 0x10;

    // ========== Function type ==========
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

    // ========== Token utilities ==========
    inline uint8_t tokenLen(char c) {
        uint8_t uc = static_cast<uint8_t>(c);
        if (uc >= 0x01 && uc <= 0x05) return 3;
        return 1;
    }
    inline uint8_t isTokenStart(char c) {
        uint8_t uc = static_cast<uint8_t>(c);
        return (uc >= 0x01 && uc <= 0x05) ? 1 : 0;
    }

    // ========== Expression builder ==========

    class ExpressionBuilder {
    public:
        ExpressionBuilder() : m_cursor(0), m_flags(0) {}

        void setExpr(const std::string& e) { m_expr = e; m_cursor = e.size(); }
        const std::string& expr() const { return m_expr; }
        uint16_t cursor() const { return m_cursor; }
        uint8_t flags() const { return m_flags; }

        // ========== Delete ==========
        void del() {
            if (m_cursor == 0) return;
            uint16_t p = m_cursor;
            if (m_expr[p-1] == '(') {
                uint16_t start = p - 1;
                while (start > 0) {
                    char c = m_expr[start-1];
                    if (isTokenStart(c)) start -= tokenLen(c);
                    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) start--;
                    else break;
                }
                if (start < p - 1) { m_expr.erase(start, p - start); m_cursor = start; return; }
            }
            if (isTokenStart(m_expr[p-1])) { uint8_t len = tokenLen(m_expr[p-1]); m_expr.erase(p-len, len); m_cursor -= len; }
            else { m_expr.erase(p-1, 1); m_cursor--; }
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
            if ((m_flags & MASK_INSERT) && m_cursor < m_expr.size()) {
                uint16_t p = m_cursor;
                uint8_t oldLen = isTokenStart(m_expr[p]) ? tokenLen(m_expr[p]) : 1;
                m_expr.replace(p, oldLen, s);
            } else {
                m_expr.insert(m_cursor, s);
            }
            m_cursor += s.size();
        }

        // ========== Handlers ==========
        void pressInfixOp(const std::string& op)     { insertStr(op); }
        void pressUnaryPrefix(const std::string& fn) { insertStr(fn + "("); }
        void pressUnaryPostfix(const std::string& spec) {
            if (spec == Spec::SQ)       insertStr("^2");
            else if (spec == Spec::CB)  insertStr("^3");
            else if (spec == Spec::INV) insertStr("^(-1)");
            else if (spec == Spec::CBRT) insertStr("cbrt(");
        }

        void pressTemplate(const std::string& fn, uint8_t nBoxes) {
            std::string tmpl = fn + "(";
            for (uint8_t i = 0; i < nBoxes; ++i) {
                if (i > 0) tmpl += ",";
                tmpl += Ctrl::BLOCKL;
                tmpl += Ctrl::BLOCKR;
            }
            tmpl += ")";
            insertStr(tmpl);
            m_cursor -= tmpl.size();
            m_cursor += fn.size() + 1 + 2;
        }

        void pressVoid(const std::string& fn)      { insertStr(fn + "()"); }
        void pressMultiArg(const std::string& fn)  { pressTemplate(fn, 1); }

        // ========== Main entry ==========

        /**
         *  @brief Process a key press
         *  @param row 0-5
         *  @param col 0-5
         *  @return 1 if consumed, 0 if empty
         */
        uint8_t pressKey(uint8_t row, uint8_t col) {
            uint8_t mod = m_flags & (MASK_SHIFT | MASK_ALPHA | MASK_CTRL);
            const std::string& key = getKey(row, col, mod);
            if (key.empty()) return 0;

            // Modifier keys
            if (key == Ctrl::SHIFT) { m_flags ^= MASK_SHIFT; return 1; }
            if (key == Ctrl::ALPHA) { m_flags ^= MASK_ALPHA; return 1; }
            if (key == Ctrl::CTRL)  { m_flags ^= MASK_CTRL;  return 1; }
            if (key == Ctrl::LOCK)  { m_flags ^= MASK_LOCK;  return 1; }
            if (key == Ctrl::INS)   { m_flags ^= MASK_INSERT; return 1; }

            // Control keys — pass through
            if (key == Ctrl::EXE || key == Ctrl::ON || key == Ctrl::OFF ||
                key == Ctrl::ABOUT || key == Ctrl::MENU || key == Ctrl::MODE ||
                key == Ctrl::SET || key == Ctrl::CLR || key == Ctrl::SOLV ||
                key == Ctrl::CALC || key == Ctrl::CONST || key == Ctrl::CONV ||
                key == Ctrl::OK || key == Ctrl::STO || key == Ctrl::RCL ||
                key == Ctrl::OPTN || key == Ctrl::FMT ||
                key == Ctrl::ZOOM_P || key == Ctrl::ZOOM_M ||
                key == Ctrl::Z_PLUS || key == Ctrl::Z_MINUS ||
                key == Ctrl::Y_PLUS || key == Ctrl::Y_MINUS) {
                goto release;
            }

            if (key == Ctrl::DEL)     { del(); goto release; }
            if (key == Ctrl::AC)      { ac();  goto release; }
            if (key == Ctrl::X_MINUS) { moveLeft();  goto release; }
            if (key == Ctrl::X_PLUS)  { moveRight(); goto release; }

            // Digits
            if (key.size() == 1 && key[0] >= '0' && key[0] <= '9') { insertStr(key); goto release; }
            if (key == ".")    { insertStr(".");    goto release; }
            if (key == Spec::EE) { insertStr("*10^"); goto release; }

            // Vars / constants
            if (key == "x" || key == "y" || key == "z" ||
                key == CAS::ConstName::i || key == CAS::ConstName::e ||
                key == CAS::ConstName::pi ||
                key == VarName::Ans || key == VarName::PAns ||
                key == VarName::theta || key == VarName::lambda ||
                key == VarName::mu || key == VarName::alpha) { insertStr(key); goto release; }

            // Infix ops
            if (key == "+" || key == "-" || key == "*" || key == "/" || key == "^" ||
                key == CAS::FuncName::dot) { pressInfixOp(key); goto release; }

            // Brackets / comma
            if (key == "(" || key == ")" || key == ",") { insertStr(key); goto release; }

            // Postfix
            if (key == Spec::SQ || key == Spec::CB || key == Spec::INV || key == Spec::CBRT) {
                pressUnaryPostfix(key); goto release;
            }

            if (key == "=") goto release;

            // Special templates
            if (key == CAS::FuncName::log)  { pressTemplate(CAS::FuncName::log, 2);  goto release; }
            if (key == CAS::FuncName::root) { pressTemplate(CAS::FuncName::root, 2); goto release; }

            if (key == Ctrl::FACTOR || key == Ctrl::EXPAND) { pressUnaryPrefix(key); goto release; }

            // Registry
            {
                const FuncInfo& fi = lookupFunc(key);
                if (fi.internal[0] == '\0') { insertStr(key); goto release; }
                switch (fi.type) {
                    case FT_OP_UNARY_PRE:   pressUnaryPrefix(fi.internal); break;
                    case FT_OP_BINARY_FUNC: pressTemplate(fi.internal, 2); break;
                    case FT_OP_MULTI:       pressMultiArg(fi.internal);    break;
                    case FT_OP_VOID:        pressVoid(fi.internal);        break;
                    case FT_OP_INDEXED:
                    case FT_OP_GRAPH:
                    case FT_OP_RANDINT:     pressTemplate(fi.internal, fi.numArgs > 0 ? fi.numArgs : 1); break;
                    default:                insertStr(fi.internal);        break;
                }
            }

        release:
            if (!(m_flags & MASK_LOCK)) m_flags &= ~(MASK_SHIFT | MASK_ALPHA | MASK_CTRL);
            return 1;
        }

    private:
        std::string m_expr;
        uint16_t m_cursor;
        uint8_t m_flags;
    };

} // namespace Keypad

#endif // _KEYPAD_HANDLER_HPP_