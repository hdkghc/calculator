/** @file /inc/expbuild.hpp
 *  @brief Builds internal expression strings from key presses
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

#ifndef _EXPBUILD_HPP_
#define _EXPBUILD_HPP_

#include "keypad.hpp"
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace Keypad {

    /** @name Modifier flags
     *  @{ */
    constexpr uint8_t M_SHIFT  = 0x01; ///< SHIFT modifier active (one-shot)
    constexpr uint8_t M_ALPHA  = 0x02; ///< ALPHA modifier active (one-shot)
    constexpr uint8_t M_CTRL   = 0x04; ///< CTRL modifier active
    constexpr uint8_t M_LOCK   = 0x08; ///< CTRL lock engaged
    constexpr uint8_t M_INSERT = 0x10; ///< Insert/Overwrite toggle
    constexpr uint8_t M_RCL    = 0x20; ///< RCL (recall) mode active
    /** @} */

    /** @name Return codes
     *  @{ */
    constexpr uint8_t B_SUCCESS = 0x00; ///< Operation completed successfully
    constexpr uint8_t B_PLOT2D  = 0x01; ///< Enter 2D plot definition
    constexpr uint8_t B_PLOT3D  = 0x02; ///< Enter 3D plot definition
    constexpr uint8_t B_SOLVE   = 0x03; ///< Solve equation
    constexpr uint8_t B_CONV    = 0x04; ///< Open conversion menu
    constexpr uint8_t B_CONST   = 0x05; ///< Open constants menu
    constexpr uint8_t B_EXEC    = 0x06; ///< Execute expression
    constexpr uint8_t B_OPTN    = 0x07; ///< Open option menu
    constexpr uint8_t B_MENU    = 0x08; ///< Open function menu
    constexpr uint8_t B_MODE    = 0x09; ///< Open mode menu
    constexpr uint8_t B_FMT     = 0x0A; ///< Format switch or menu
    constexpr uint8_t B_ABOUT   = 0x0B; ///< Show about dialog
    constexpr uint8_t B_SET     = 0x0C; ///< Open setup menu
    constexpr uint8_t B_CLEAR   = 0x0D; ///< Clear memory or settings
    constexpr uint8_t B_CALC    = 0x0E; ///< Calculate (like EXE)
    constexpr uint8_t B_FACTOR  = 0x0F; ///< Factorise expression
    constexpr uint8_t B_EXPAND  = 0x10; ///< Expand expression
    constexpr uint8_t B_VECDEF  = 0x11; ///< Enter vector definition GUI
    constexpr uint8_t B_MATDEF  = 0x12; ///< Enter matrix definition GUI
    constexpr uint8_t B_HISTUP  = 0x13; ///< Recall previous history entry
    constexpr uint8_t B_HISTDN  = 0x14; ///< Recall next history entry
    constexpr uint8_t B_VECEDT  = 0x15; ///< Edit existing vector
    constexpr uint8_t B_MATEDT  = 0x16; ///< Edit existing matrix
    constexpr uint8_t B_RESET   = 0x17; ///< Reset (ON key)
    constexpr uint8_t B_OFF     = 0xFE; ///< Power off
    constexpr uint8_t B_ERROR   = 0xFF; ///< Generic error
    /** @} */

    constexpr int BLKLEN = 2; ///< Byte length of \x03\x20 or \x03\x21

    // =================================================================
    // Token types
    // =================================================================

    /** @brief Token type classification. */
    enum class TokType : uint8_t {
        ASCII,      ///< Single-byte: digit, letter, operator, space, etc.
        FUNC,       ///< \x01xx — 3-byte function name
        CONST,      ///< \x02x  — 2-byte constant
        CTRL,       ///< \x03x  — 2-byte control (BLOCKL, BLOCKR, STO, …)
        CONV,       ///< \x04x  — 2-byte unit conversion
        SI,         ///< \x05x  — 2-byte SI prefix
        VAR         ///< \x06x  — 2-byte special variable
    };

    /** @brief A logical token in the expression string. */
    struct Token {
        TokType type;       ///< Token type
        int16_t beg;        ///< Starting byte offset in exp
        int16_t end;        ///< One-past-end byte offset (beg + width)
        int16_t pair;       ///< For BLOCKL/BLOCKR: matching token index; -1 otherwise

        int width() const { return end - beg; }
    };

    // =================================================================
    // Tokeniser
    // =================================================================

    /**
     * @brief  Parse exp into a flat list of logical tokens.
     * @param  exp Expression string.
     * @return     Vector of Token structs.
     *
     * BLOCKL and BLOCKR tokens receive their matching partner's index
     * in the @c pair field.  All other tokens have @c pair = -1.
     */
    inline std::vector<Token> _tokenize(const std::string &exp) {
        std::vector<Token> toks;
        int16_t pos = 0;
        int16_t n   = (int16_t)exp.size();

        while (pos < n) {
            Token t;
            t.beg  = pos;
            t.pair = -1;
            uint8_t lead = (uint8_t)exp[pos];

            if (lead >= 0x01 && lead <= 0x06) {
                if (lead == 0x01)      t.type = TokType::FUNC;
                else if (lead == 0x02) t.type = TokType::CONST;
                else if (lead == 0x03) t.type = TokType::CTRL;
                else if (lead == 0x04) t.type = TokType::CONV;
                else if (lead == 0x05) t.type = TokType::SI;
                else                   t.type = TokType::VAR;

                if (lead == 0x01 && pos + 2 < n)       t.end = pos + 3;
                else if (pos + 1 < n)                   t.end = pos + 2;
                else { t.end = n; toks.push_back(t); break; }
            } else {
                t.type = TokType::ASCII;
                t.end  = pos + 1;
            }
            toks.push_back(t);
            pos = t.end;
        }

        // Second pass: match BLOCKL ↔ BLOCKR
        std::vector<int16_t> stack;
        for (int i = 0; i < (int)toks.size(); i++) {
            Token &t = toks[i];
            if (t.type != TokType::CTRL) continue;
            if (exp[t.beg + 1] == Ctrl::BLOCKL[1]) {
                stack.push_back((int16_t)i);
            } else if (exp[t.beg + 1] == Ctrl::BLOCKR[1]) {
                if (!stack.empty()) {
                    int16_t li = stack.back(); stack.pop_back();
                    toks[li].pair = (int16_t)i;
                    toks[i].pair  = li;
                }
            }
        }
        return toks;
    }

    // =================================================================
    // Expbuild
    // =================================================================

    /**
     * @class  Expbuild
     * @brief  Converts key presses into an internal expression string with
     *         cursor management, replicating Casio calculator input logic.
     *
     * The expression uses an enriched bytecode.  The cursor @c cp is a byte
     * offset that is always kept on a token boundary by rebuilding tokens
     * after every mutation and using token-index-based navigation.
     */
    class Expbuild {
        public:
            std::string exp; ///< Internal expression string (enriched bytecode).
            uint8_t     flg; ///< Modifier / status flags.
            int16_t     cp;  ///< Cursor position (byte offset into exp).

            Expbuild() : flg(0), cp(0) {}

            /**
             * @brief  Process a single key press.
             * @param  r Row index (0 = top, 5 = bottom).
             * @param  c Column index (0 = left, 5 = right).
             * @return Status code.
             */
            uint8_t press(uint8_t r, uint8_t c) {
                return insert(getKey(r, c, flg & 0x7));
            }

            /**
             * @brief  Process a key string.
             * @param  k Key string from getKey() or external source.
             * @return Status code.
             */
            uint8_t insert(std::string k) {
                if (k.empty()) return B_SUCCESS;

                // ----- Modifier keys ------------------------------------
                if (k == Ctrl::SHIFT) { flg ^= M_SHIFT; return B_SUCCESS; }
                if (k == Ctrl::ALPHA) { flg ^= M_ALPHA; return B_SUCCESS; }
                if (k == Ctrl::CTRL)  { flg ^= M_CTRL; flg &= ~M_LOCK; return B_SUCCESS; }
                if (k == Ctrl::LOCK)  { flg ^= M_LOCK; return B_SUCCESS; }
                if (k == Ctrl::INS)   { flg ^= M_INSERT; return B_SUCCESS; }
                if (k == Ctrl::RCL)   { flg ^= M_RCL; return B_SUCCESS; }

                // ----- Global controls ----------------------------------
                if (k == Ctrl::ON)  return B_RESET;
                if (k == Ctrl::OFF) return B_OFF;
                if (k == Ctrl::AC)  { exp.clear(); cp = 0; flg = 0; return B_SUCCESS; }

                // ----- Navigation ---------------------------------------
                if (k == Ctrl::X_PLUS)  { _move( 1); _rel(); return B_SUCCESS; }
                if (k == Ctrl::X_MINUS) { _move(-1); _rel(); return B_SUCCESS; }
                if (k == Ctrl::Y_PLUS || k == Ctrl::Y_MINUS) {
                    if (exp.empty()) return (k == Ctrl::Y_PLUS) ? B_HISTUP : B_HISTDN;
                    _vert(k == Ctrl::Y_PLUS);
                    _rel();
                    return B_SUCCESS;
                }

                // ----- Deletion -----------------------------------------
                if (k == Ctrl::DEL) return _del();

                // ----- Execution ----------------------------------------
                if (k == Ctrl::EXE || k == Ctrl::OK) return B_EXEC;

                // ----- Menu / system keys -------------------------------
                if (k == Ctrl::MENU)    return B_MENU;
                if (k == Ctrl::MODE)    return B_MODE;
                if (k == Ctrl::SET)     return B_SET;
                if (k == Ctrl::CLR)     return B_CLEAR;
                if (k == Ctrl::ABOUT)   return B_ABOUT;
                if (k == Ctrl::FMT)     return B_FMT;
                if (k == Ctrl::OPTN)    return B_OPTN;
                if (k == Ctrl::CONV)    return B_CONV;
                if (k == Ctrl::CONST)   return B_CONST;
                if (k == Ctrl::SOLV)    return B_SOLVE;
                if (k == Ctrl::CALC)    return B_CALC;
                if (k == Ctrl::FACTOR)  return B_FACTOR;
                if (k == Ctrl::EXPAND)  return B_EXPAND;

                // ----- STO ----------------------------------------------
                if (k == Ctrl::STO) { _ins(k); return B_SUCCESS; }

                // Unknown \x03 control
                if (k.size() >= 1 && (uint8_t)k[0] == 0x03) return B_ERROR;

                // ----- GUI-edited objects -------------------------------
                if (k == CAS::FuncName::vector)  return B_VECDEF;
                if (k == CAS::FuncName::matrix)  return B_MATDEF;
                if (k == CAS::FuncName::plot2d)  return B_PLOT2D;
                if (k == CAS::FuncName::plot3d)  return B_PLOT3D;

                // ----- Spec → ASCII ------------------------------------
                if (k == Spec::SQ)  { _ins("^2");     return B_SUCCESS; }
                if (k == Spec::CB)  { _ins("^3");     return B_SUCCESS; }
                if (k == Spec::INV) { _ins("^(-1)");  return B_SUCCESS; }
                if (k == Spec::EE)  { _ins("*10^");   return B_SUCCESS; }
                if (k == Spec::CBRT) {
                    _boxedFunc(CAS::FuncName::root, 2);
                    _raw("3");
                    _jumpBlk(1);
                    return B_SUCCESS;
                }

                // ----- ^ operator (2-block boxed, ASCII prefix) ---------
                if (k == "^") { _boxedOp("^", 2); return B_SUCCESS; }

                // ----- Boxed functions (\x01xx + blocks) ----------------
                if (k == CAS::FuncName::abs || k == CAS::FuncName::sqrt)
                    return _boxedFunc(k, 1);
                if (k == CAS::FuncName::root || k == CAS::FuncName::log ||
                    k == CAS::FuncName::permut || k == CAS::FuncName::combin ||
                    k == CAS::FuncName::diff || k == CAS::FuncName::indefint)
                    return _boxedFunc(k, 2);
                if (k == CAS::FuncName::sum || k == CAS::FuncName::prod ||
                    k == CAS::FuncName::defint)
                    return _boxedFunc(k, 4);

                // ----- Parenthesised functions (\x01xx( atomic) ---------
                if (k.size() == 3 && (uint8_t)k[0] == 0x01) {
                    _ins((k == CAS::FuncName::deg || k == CAS::FuncName::rad)
                         ? k : k + "(");
                    return B_SUCCESS;
                }

                // ----- Everything else ----------------------------------
                _ins(k);
                return B_SUCCESS;
            }

        private:
            // =========================================================
            // Token helpers
            // =========================================================

            /** @brief Tokenise current expression. */
            std::vector<Token> _tok() const { return _tokenize(exp); }

            /**
             * @brief  Find the token index containing byte offset pos.
             * @param  toks Token list.
             * @param  pos  Byte offset.
             * @return      Token index, or toks.size() if pos is at end.
             */
            int _tokIdx(const std::vector<Token> &toks, int16_t pos) const {
                if (pos >= (int16_t)exp.size()) return (int)toks.size();
                for (int i = 0; i < (int)toks.size(); i++)
                    if (pos < toks[i].end) return i;
                return (int)toks.size();
            }

            /**
             * @brief  Collect BLOCKL token indices of a boxed function.
             * @param  toks   Token list.
             * @param  blkTi  Token index of the first BLOCKL.
             * @return        Vector of BLOCKL token indices.
             */
            std::vector<int> _funcBlocks(const std::vector<Token> &toks,
                                         int blkTi) const {
                std::vector<int> out;
                for (int i = blkTi; i < (int)toks.size(); i++) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        out.push_back(i);
                    } else if (toks[i].type == TokType::CTRL &&
                               exp[toks[i].beg + 1] == Ctrl::BLOCKR[1] &&
                               toks[i].pair == blkTi) {
                        break;
                    }
                }
                return out;
            }

            /**
             * @brief  Get the token index of the function/operator prefix
             *         for the boxed structure whose first BLOCKL is at blkTi.
             * @return Token index of the prefix, or -1.
             *
             * Prefix can be a \x01xx FUNC token, or an ASCII operator like '^'.
             */
            int _boxedPrefix(const std::vector<Token> &toks, int blkTi) const {
                if (blkTi <= 0) return -1;
                // \x01 function prefix
                if (toks[blkTi - 1].type == TokType::FUNC) return blkTi - 1;
                // ASCII operator prefix (e.g. '^')
                if (toks[blkTi - 1].type == TokType::ASCII) return blkTi - 1;
                return -1;
            }

            /**
             * @brief  Get the prefix string (function name or operator)
             *         for the boxed structure whose first BLOCKL is at blkTi.
             */
            std::string _boxedName(const std::vector<Token> &toks, int blkTi) const {
                int pi = _boxedPrefix(toks, blkTi);
                if (pi < 0) return "";
                return exp.substr(toks[pi].beg, toks[pi].width());
            }

            // =========================================================
            // Insert helpers
            // =========================================================

            /** Raw insert, no modifier release */
            void _raw(const std::string &s) {
                if (s.empty()) return;
                if (flg & M_INSERT) {
                    auto toks = _tok();
                    int ti = _tokIdx(toks, cp);
                    int16_t w = 0;
                    for (size_t j = 0; j < s.size() && ti < (int)toks.size(); j++, ti++)
                        w += toks[ti].width();
                    if (w > 0) exp.replace(cp, w, s);
                    else       exp.insert(cp, s);
                } else {
                    exp.insert(cp, s);
                }
                cp += (int16_t)s.size();
            }

            /** Insert with modifier release */
            void _ins(const std::string &s) { _raw(s); _rel(); }

            /**
             * @brief  Insert a boxed function with \x01 prefix and n blocks.
             */
            uint8_t _boxedFunc(const std::string &fn, int n) {
                return _boxedInsert(fn, n, true);
            }

            /**
             * @brief  Insert a boxed operator with ASCII prefix and n blocks.
             */
            uint8_t _boxedOp(const std::string &op, int n) {
                return _boxedInsert(op, n, false);
            }

            /**
             * @brief  Generic boxed insertion.
             * @param  s    Prefix string (function name or operator).
             * @param  n    Number of blocks.
             * @param  isFn True if prefix is \x01 function (3 bytes), false if ASCII.
             */
            uint8_t _boxedInsert(const std::string &s, int n, bool isFn) {
                // Overwrite absorption (1-block only)
                if (n == 1 && (flg & M_INSERT) && cp < (int16_t)exp.size()) {
                    int16_t end = _scanOp(cp);
                    if (end > cp) {
                        std::string op = exp.substr(cp, end - cp);
                        std::string w  = s + Ctrl::BLOCKL + op + Ctrl::BLOCKR;
                        exp.replace(cp, end - cp, w);
                        cp = cp + (int16_t)s.size() + BLKLEN;
                        _rel();
                        return B_SUCCESS;
                    }
                }

                int16_t insPos = cp;
                std::string ins = s;
                for (int i = 0; i < n; i++) { ins += Ctrl::BLOCKL; ins += Ctrl::BLOCKR; }
                _raw(ins);
                cp = insPos + (int16_t)s.size() + BLKLEN;
                _rel();
                return B_SUCCESS;
            }

            /** Scan forward one operand */
            int16_t _scanOp(int16_t pos) const {
                if (pos >= (int16_t)exp.size()) return pos;
                uint8_t lead = (uint8_t)exp[pos];

                if (lead == '(') {
                    int d = 1; int16_t i = pos + 1;
                    while (i < (int16_t)exp.size() && d > 0) {
                        if (exp[i] == '(') d++; else if (exp[i] == ')') d--;
                        i++;
                    }
                    return i;
                }
                if (lead == 0x01 && pos + 2 < (int16_t)exp.size()) {
                    int16_t i = pos + 3;
                    if (i < (int16_t)exp.size() && (uint8_t)exp[i] == 0x03 &&
                        exp[i + 1] == Ctrl::BLOCKL[1]) {
                        int d = 0;
                        while (i < (int16_t)exp.size()) {
                            if ((uint8_t)exp[i] == 0x03 && exp[i+1] == Ctrl::BLOCKL[1]) d++;
                            else if ((uint8_t)exp[i] == 0x03 && exp[i+1] == Ctrl::BLOCKR[1]) {
                                if (--d <= 0) { i += BLKLEN; break; }
                            } else if (exp[i] == '(') {
                                int pd = 1; i++;
                                while (i < (int16_t)exp.size() && pd > 0) {
                                    if (exp[i] == '(') pd++; else if (exp[i] == ')') pd--;
                                    i++;
                                }
                                continue;
                            }
                            i++;
                        }
                        return i;
                    }
                    if (i < (int16_t)exp.size() && exp[i] == '(') {
                        int pd = 1; i++;
                        while (i < (int16_t)exp.size() && pd > 0) {
                            if (exp[i] == '(') pd++; else if (exp[i] == ')') pd--;
                            i++;
                        }
                        return i;
                    }
                    return i;
                }
                if ((lead >= '0' && lead <= '9') || lead == '.' ||
                    (lead >= 'A' && lead <= 'Z') || (lead >= 'a' && lead <= 'z')) {
                    int16_t i = pos + 1;
                    while (i < (int16_t)exp.size() &&
                           ((exp[i] >= '0' && exp[i] <= '9') || exp[i] == '.' ||
                            (exp[i] >= 'A' && exp[i] <= 'Z') ||
                            (exp[i] >= 'a' && exp[i] <= 'z'))) i++;
                    return i;
                }
                if (lead >= 0x02 && lead <= 0x06) return pos + 2;
                return pos + 1;
            }

            // =========================================================
            // Modifier state machine
            // =========================================================

            void _rel() {
                if ((flg & M_SHIFT) && !((flg & M_ALPHA) && (flg & M_CTRL)))
                    flg &= ~M_SHIFT;
                if ((flg & M_ALPHA) && !(flg & M_CTRL))
                    flg &= ~M_ALPHA;
                if ((flg & M_CTRL) && !(flg & M_LOCK) && !(flg & M_ALPHA))
                    flg &= ~M_CTRL;
                if ((flg & M_LOCK) && ((flg & 0x07) != M_CTRL))
                    flg &= ~M_LOCK;
            }

            // =========================================================
            // Cursor movement
            // =========================================================

            /**
             * @brief  Move cursor by delta tokens (+1 right, -1 left).
             */
            void _move(int delta) {
                auto toks = _tok();
                int ti = _tokIdx(toks, cp);
                int n  = (int)toks.size();

                if (delta > 0) {
                    if (ti >= n) return;
                    const Token &t = toks[ti];

                    // \x01xx( → skip func + '('
                    if (t.type == TokType::FUNC && ti + 1 < n &&
                        toks[ti + 1].type == TokType::ASCII &&
                        exp[toks[ti + 1].beg] == '(') {
                        cp = toks[ti + 1].end;
                    }
                    // Boxed prefix → enter first block
                    else if (_isBoxedPrefix(toks, ti)) {
                        std::string nm = _boxedNameAt(toks, ti);
                        if (nm == CAS::FuncName::vector ||
                            nm == CAS::FuncName::matrix) {
                            cp = _boxedEnd(toks, ti);
                        } else {
                            cp = toks[ti + 1].end;
                        }
                    }
                    // BLOCKR → next block, or exit
                    else if (t.type == TokType::CTRL &&
                             exp[t.beg + 1] == Ctrl::BLOCKR[1]) {
                        int nextLi = _nextSiblingBlockL(toks, ti);
                        if (nextLi >= 0) {
                            cp = toks[nextLi].end;
                        } else {
                            cp = t.end;
                        }
                    }
                    else {
                        cp = t.end;
                    }
                } else {
                    if (ti <= 0) { cp = 0; return; }
                    const Token &t = toks[ti - 1];

                    // '(' bound to \x01 → skip func too
                    if (t.type == TokType::ASCII && exp[t.beg] == '(' &&
                        ti - 2 >= 0 && toks[ti - 2].type == TokType::FUNC) {
                        cp = toks[ti - 2].beg;
                    }
                    // BLOCKR → enter the block
                    else if (t.type == TokType::CTRL &&
                             exp[t.beg + 1] == Ctrl::BLOCKR[1]) {
                        int li = t.pair;
                        if (li >= 0) {
                            cp = (toks[li].end == toks[ti - 1].beg)
                                 ? toks[li].end
                                 : toks[ti - 1].beg;
                        } else {
                            cp = t.beg;
                        }
                    }
                    // BLOCKL → previous block's end, or before prefix
                    else if (t.type == TokType::CTRL &&
                             exp[t.beg + 1] == Ctrl::BLOCKL[1]) {
                        int prevRi = _prevSiblingBlockR(toks, ti - 1);
                        if (prevRi >= 0) {
                            int prevLi = toks[prevRi].pair;
                            cp = (prevLi >= 0 &&
                                  toks[prevLi].end == toks[prevRi].beg)
                                 ? toks[prevLi].end
                                 : toks[prevRi].beg;
                        } else {
                            int pi = _boxedPrefix(toks, ti - 1);
                            cp = (pi >= 0) ? toks[pi].beg : t.beg;
                        }
                    }
                    else {
                        cp = t.beg;
                    }
                }
            }

            /**
             * @brief  Find the next sibling BLOCKL after a BLOCKR.
             * @param  toks Token list.
             * @param  ri   Token index of the BLOCKR.
             * @return      Token index of next BLOCKL in the same boxed structure,
             *              or -1 if this is the last block.
             */
            int _nextSiblingBlockL(const std::vector<Token> &toks, int ri) const {
                if (ri + 1 >= (int)toks.size()) return -1;
                const Token &next = toks[ri + 1];
                if (next.type == TokType::CTRL &&
                    exp[next.beg + 1] == Ctrl::BLOCKL[1]) {
                    return ri + 1;
                }
                return -1;
            }
            
            /**
             * @brief  Find the previous sibling BLOCKR before a BLOCKL.
             * @param  toks Token list.
             * @param  li   Token index of the current BLOCKL.
             * @return      Token index of previous BLOCKR in the same boxed
             *              structure, or -1 if this is the first block.
             */
            int _prevSiblingBlockR(const std::vector<Token> &toks, int li) const {
                if (li - 1 < 0) return -1;
                const Token &prev = toks[li - 1];
                if (prev.type == TokType::CTRL &&
                    exp[prev.beg + 1] == Ctrl::BLOCKR[1]) {
                    return li - 1;
                }
                return -1;
            }

            bool _isBoxedPrefix(const std::vector<Token> &toks, int ti) const {
                int n = (int)toks.size();
                if (ti + 1 >= n) return false;
                if (toks[ti + 1].type != TokType::CTRL) return false;
                if (exp[toks[ti + 1].beg + 1] != Ctrl::BLOCKL[1]) return false;
                return (toks[ti].type == TokType::FUNC ||
                        toks[ti].type == TokType::ASCII);
            }

            std::string _boxedNameAt(const std::vector<Token> &toks, int ti) const {
                return exp.substr(toks[ti].beg, toks[ti].width());
            }

            int16_t _boxedEnd(const std::vector<Token> &toks, int ti) const {
                int n = (int)toks.size();
                for (int i = ti + 1; i < n; i++) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKR[1] &&
                        toks[i].pair <= ti + 1) {
                        return toks[i].end;
                    }
                }
                return toks[ti].end;
            }

            /** Skip past a boxed structure (prefix + all blocks) */
            int16_t _skipBoxed(const std::vector<Token> &toks, int prefixTi) const {
                int n = (int)toks.size();
                for (int i = prefixTi + 1; i < n; i++) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                        // Outermost BLOCKR
                        if (toks[i].pair >= 0 && toks[i].pair <= prefixTi + 1)
                            return toks[i].end;
                    }
                }
                return toks[prefixTi].end;
            }

            /**
             * @brief  Vertical movement (Y+/Y-).
             * @param  up true = up, false = down.
             */
            void _vert(bool up) {
                auto toks = _tok();
                int ti = _tokIdx(toks, cp);

                // Find enclosing BLOCKL
                int blkTi = -1;
                for (int i = ti - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL) {
                        if (exp[toks[i].beg + 1] == Ctrl::BLOCKL[1])
                            { blkTi = i; break; }
                        if (exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) break;
                    }
                }
                if (blkTi < 0) return;

                int pi = _boxedPrefix(toks, blkTi);
                if (pi < 0) return;
                std::string nm = _boxedName(toks, blkTi);

                // diff / indefint — horizontal only
                if (nm == CAS::FuncName::diff || nm == CAS::FuncName::indefint) return;

                std::vector<int> blks = _funcBlocks(toks, blkTi);
                int nb = (int)blks.size();
                if (nb <= 1) return;

                // Find current block
                int cur = -1;
                for (int idx = 0; idx < nb; idx++) {
                    int16_t rbeg = toks[blks[idx]].beg;
                    int16_t rend = toks[toks[blks[idx]].pair].end;
                    if (cp >= rbeg && cp <= rend) { cur = idx; break; }
                }
                if (cur < 0) return;

                int tgt = cur;
                if (nb == 2) {
                    tgt = up ? 1 : 0;
                } else if (nb == 4) {
                    if (cur == 3)      tgt = up ? 3 : 0;
                    else if (cur == 2) tgt = up ? 0 : 2;
                    else               tgt = up ? 3 : 2;
                }
                cp = toks[blks[tgt]].end;
            }

            /** Jump to block idx inside enclosing boxed function */
            void _jumpBlk(int idx) {
                auto toks = _tok();
                int ti = _tokIdx(toks, cp);
                for (int i = ti - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        int pi = _boxedPrefix(toks, i);
                        if (pi < 0) break;
                        std::vector<int> blks = _funcBlocks(toks, i);
                        if (idx >= 0 && idx < (int)blks.size())
                            cp = toks[blks[idx]].end;
                        return;
                    }
                }
            }

            // =========================================================
            // Deletion
            // =========================================================

            /**
             * @brief  DEL key handler.
             *
             * Boxed-function DEL rules:
             *   - Cursor after BLOCKR → jump into block.
             *     log: jump to the *other* block.
             *     vector/matrix: return edit code.
             *   - Cursor in empty block → delete whole function/operator.
             *   - Cursor after content inside block → backspace one token.
             *   - Outside any block → ordinary backspace.
             */
            uint8_t _del() {
                if (exp.empty() || cp <= 0) return B_ERROR;

                auto toks = _tok();
                int ti = _tokIdx(toks, cp);

                // --- Case 1: cp right after a BLOCKR → jump into block ---
                if (ti > 0 && toks[ti - 1].type == TokType::CTRL &&
                    exp[toks[ti - 1].beg + 1] == Ctrl::BLOCKR[1]) {
                    int ri = ti - 1;
                    int li = toks[ri].pair;
                    if (li < 0) return B_ERROR;
                    int pi = _boxedPrefix(toks, li);
                    if (pi < 0) return B_ERROR;
                    std::string nm = _boxedName(toks, li);

                    // vector/matrix → edit
                    if (nm == CAS::FuncName::vector) return B_VECEDT;
                    if (nm == CAS::FuncName::matrix) return B_MATEDT;

                    // log: jump to the *other* block
                    if (nm == CAS::FuncName::log) {
                        std::vector<int> blks = _funcBlocks(toks, li);
                        int curBlk = -1;
                        for (int b = 0; b < (int)blks.size(); b++)
                            if (blks[b] == li) { curBlk = b; break; }
                        int other = (curBlk == 0) ? 1 : 0;
                        if (other < (int)blks.size()) {
                            int obli = blks[other];
                            int obri = toks[obli].pair;
                            cp = (toks[obli].end == toks[obri].beg)
                                 ? toks[obli].end   // empty → after BLOCKL
                                 : toks[obri].beg;  // end of content
                        }
                        return B_SUCCESS;
                    }

                    // All others: jump into this block
                    cp = (toks[li].end == toks[ri].beg)
                         ? toks[li].end   // empty
                         : toks[ri].beg;  // end of content
                    return B_SUCCESS;
                }

                // --- Case 2: inside a block ---
                for (int i = ti - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        int li = i;
                        int ri = toks[li].pair;
                        if (ri < 0) break;

                        // Cursor at very start of a block
                        if (cp == toks[li].end) {
                            // Single block → delete function, keep content
                            if (_isSingleBlock(toks, li)) {
                                return _delBoxed(toks, li);
                            }

                            // Multi-block: determine behavior based on function
                            int pi = _boxedPrefix(toks, li);
                            std::string nm = (pi >= 0) ? _boxedNameAt(toks, pi) : "";

                            // log: block1 → jump to block2; block2 → delete function
                            bool logStyle = (nm == CAS::FuncName::log);
                            // root/permut/combin: block1 → delete function; block2 → jump to block1
                            // (these are the opposite of log)

                            int blkIdx = _blockIndex(toks, li);
                            bool isFirstBlock = (blkIdx == 0);

                            if (logStyle) {
                                if (isFirstBlock) {
                                    // Jump to block2 content end
                                    return _jumpToOtherBlockEnd(toks, li);
                                } else {
                                    // Delete function, keep all content
                                    return _delBoxed(toks, li);
                                }
                            } else {
                                // root/permut/combin style
                                if (isFirstBlock) {
                                    // Delete function, keep all content
                                    return _delBoxed(toks, li);
                                } else {
                                    // Jump to block1 content end
                                    return _jumpToOtherBlockEnd(toks, li);
                                }
                            }
                        }

                        // Inside block with content → backspace
                        if (cp > toks[li].end && cp <= toks[ri].beg) {
                            if (ti == 0) return B_ERROR;
                            return _delTok(toks, ti - 1);
                        }
                        break;
                    }
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) break;
                }

                // --- Case 3: ordinary backspace ---
                if (ti == 0) return B_ERROR;
                return _delTok(toks, ti - 1);
            }

            /** True if the boxed structure starting at BLOCKL li has only one block */
            bool _isSingleBlock(const std::vector<Token> &toks, int li) const {
                int depth = 0;
                int count = 0;
                for (int i = li; i < (int)toks.size(); i++) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        if (depth == 0) count++;
                        depth++;
                    } else if (toks[i].type == TokType::CTRL &&
                               exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                        depth--;
                        if (depth < 0) break;
                    }
                }
                return count == 1;
            }

            /** Find the OTHER block's BLOCKL index in a 2-block structure */
            int _otherBlock(const std::vector<Token> &toks, int li) const {
                std::vector<int> blks;
                int depth = 0;
                for (int i = li; i < (int)toks.size(); i++) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        if (depth == 0) blks.push_back(i);
                        depth++;
                    } else if (toks[i].type == TokType::CTRL &&
                               exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                        depth--;
                        if (depth < 0) break;
                    }
                }
                if (blks.size() != 2) return -1;
                return (blks[0] == li) ? blks[1] : blks[0];
            }

            /**
             * @brief  Get the index (0-based) of a BLOCKL within its boxed structure.
             */
            int _blockIndex(const std::vector<Token> &toks, int li) const {
                int idx = 0;
                int depth = 0;
                for (int i = li; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL) {
                        if (exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) depth++;
                        else if (exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                            if (depth == 0) idx++;
                            else depth--;
                        }
                    }
                }
                return idx - 1; // 0-based
            }

            /**
             * @brief  Jump cursor to the OTHER block's content end in a 2-block structure.
             */
            uint8_t _jumpToOtherBlockEnd(const std::vector<Token> &toks, int li) {
                std::vector<int> blks;
                int depth = 0;
                for (int i = li; i < (int)toks.size(); i++) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        if (depth == 0) blks.push_back(i);
                        depth++;
                    } else if (toks[i].type == TokType::CTRL &&
                               exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                        depth--;
                        if (depth < 0) break;
                    }
                }
                if (blks.size() != 2) return B_ERROR;
                int otherLi = (blks[0] == li) ? blks[1] : blks[0];
                int otherRi = toks[otherLi].pair;
                cp = (toks[otherLi].end == toks[otherRi].beg)
                     ? toks[otherLi].end   // empty
                     : toks[otherRi].beg;  // end of content
                return B_SUCCESS;
            }

            /**
             * @brief  Delete a boxed function/operator given the first BLOCKL.
             *         If the block is empty, other blocks' content is kept.
             */
            uint8_t _delBoxed(const std::vector<Token> &toks, int delLi) {
                int pi = _boxedPrefix(toks, delLi);
                if (pi < 0) return B_ERROR;

                std::vector<int> blks;
                int depth = 0;
                int lastRi = -1;
                for (int i = delLi; i < (int)toks.size(); i++) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        if (depth == 0) blks.push_back(i);
                        depth++;
                    } else if (toks[i].type == TokType::CTRL &&
                               exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                        depth--;
                        if (depth == 0) lastRi = i;
                        if (depth < 0) break;
                    }
                }
                if (lastRi < 0) return B_ERROR;

                // Collect content from ALL blocks
                std::string kept;
                for (int b = 0; b < (int)blks.size(); b++) {
                    int ri = toks[blks[b]].pair;
                    if (ri >= 0 && toks[blks[b]].end < toks[ri].beg) {
                        kept += exp.substr(toks[blks[b]].end,
                                           toks[ri].beg - toks[blks[b]].end);
                    }
                }

                exp.erase(toks[pi].beg, toks[lastRi].end - toks[pi].beg);
                exp.insert(toks[pi].beg, kept);
                cp = toks[pi].beg + (int16_t)kept.size();
                return B_SUCCESS;
            }

            /**
             * @brief  Backspace-delete one token at index ti.
             */
            uint8_t _delTok(const std::vector<Token> &toks, int ti) {
                const Token &t = toks[ti];

                // '(' bound to \x01 function → delete \x01xx(...)
                if (t.type == TokType::ASCII && exp[t.beg] == '(' &&
                    ti > 0 && toks[ti - 1].type == TokType::FUNC) {
                    int d = 1;
                    int16_t i = t.end;
                    while (i < (int16_t)exp.size() && d > 0) {
                        if (exp[i] == '(') d++;
                        else if (exp[i] == ')') d--;
                        i++;
                    }
                    exp.erase(toks[ti - 1].beg, i - toks[ti - 1].beg);
                    cp = toks[ti - 1].beg;
                    return B_SUCCESS;
                }

                exp.erase(t.beg, t.width());
                cp = t.beg;
                return B_SUCCESS;
            }
    };

} // namespace Keypad

#endif // _EXPBUILD_HPP_