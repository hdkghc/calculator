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

    constexpr uint8_t M_SHIFT  = 0x01; ///< SHIFT modifier active
    constexpr uint8_t M_ALPHA  = 0x02; ///< ALPHA modifier active
    constexpr uint8_t M_CTRL   = 0x04; ///< CTRL modifier active
    constexpr uint8_t M_LOCK   = 0x08; ///< CTRL lock engaged
    constexpr uint8_t M_INSERT = 0x10; ///< Insert/Overwrite toggle
    constexpr uint8_t M_RCL    = 0x20; ///< RCL mode active

    constexpr uint8_t B_SUCCESS = 0x00; ///< Operation successful
    constexpr uint8_t B_PLOT2D  = 0x01; ///< Enter 2D plot menu
    constexpr uint8_t B_PLOT3D  = 0x02; ///< Enter 3D plot menu
    constexpr uint8_t B_SOLVE   = 0x03; ///< Solve equation
    constexpr uint8_t B_CONV    = 0x04; ///< Conversion menu
    constexpr uint8_t B_CONST   = 0x05; ///< Constants menu
    constexpr uint8_t B_EXEC    = 0x06; ///< Execute expression
    constexpr uint8_t B_OPTN    = 0x07; ///< Option menu
    constexpr uint8_t B_MENU    = 0x08; ///< Function menu
    constexpr uint8_t B_MODE    = 0x09; ///< Mode menu
    constexpr uint8_t B_FMT     = 0x0A; ///< Format switch/menu
    constexpr uint8_t B_ABOUT   = 0x0B; ///< About dialog
    constexpr uint8_t B_SET     = 0x0C; ///< Setup menu
    constexpr uint8_t B_CLEAR   = 0x0D; ///< Clear memory/settings
    constexpr uint8_t B_CALC    = 0x0E; ///< Calculate (like EXE)
    constexpr uint8_t B_FACTOR  = 0x0F; ///< Factorisation
    constexpr uint8_t B_EXPAND  = 0x10; ///< Expand expression
    constexpr uint8_t B_VECDEF  = 0x11; ///< Vector definition
    constexpr uint8_t B_MATDEF  = 0x12; ///< Matrix definition
    constexpr uint8_t B_HISTUP  = 0x13; ///< History up
    constexpr uint8_t B_HISTDN  = 0x14; ///< History down
    constexpr uint8_t B_VECEDT  = 0x15; ///< Edit vector
    constexpr uint8_t B_MATEDT  = 0x16; ///< Edit matrix
    constexpr uint8_t B_CLRSCR  = 0x17; ///< Clear screen (ON key)
    constexpr uint8_t B_OFF     = 0xFE; ///< Power off
    constexpr uint8_t B_ERROR   = 0xFF; ///< Generic error

    /**
     * @brief Expression builder: converts key presses into an internal
     *        expression string with cursor management
     */
    class Expbuild {
    public:
        std::string exp; ///< The internal expression string
        uint8_t     flg; ///< Modifier/status flags
        int16_t     cp;  ///< Cursor position (byte offset into exp)

        Expbuild() : flg(0), cp(0) {}

        /**
         * @brief Process a key press and build the expression string
         * @param r  Row index (0-5)
         * @param c  Column index (0-5)
         * @return   Status code (B_SUCCESS, B_EXEC, B_OFF, etc.)
         */
        uint8_t press(uint8_t r, uint8_t c) {
            std::string key = getKey(r, c, flg & 0x7);
            if (key.empty()) return B_ERROR;

            // --- Modifier keys ---
            if (key == Ctrl::SHIFT || key == Ctrl::ALPHA ||
                key == Ctrl::CTRL  || key == Ctrl::LOCK  ||
                key == Ctrl::INS   || key == Ctrl::RCL) {
                _Modifier(key);
                return B_SUCCESS;
            }

            // --- ON (clear screen) / OFF ---
            if (key == Ctrl::ON)  return B_CLRSCR;
            if (key == Ctrl::OFF) return B_OFF;

            // --- AC ---
            if (key == Ctrl::AC) { _AC(); return B_SUCCESS; }

            // --- DEL ---
            if (key == Ctrl::DEL) return _DEL();

            // --- Navigation ---
            if (key == Ctrl::X_PLUS  || key == Ctrl::X_MINUS ||
                key == Ctrl::Y_PLUS  || key == Ctrl::Y_MINUS) {
                return _Move(key);
            }

            // --- Execute ---
            if (key == Ctrl::EXE || key == Ctrl::OK) return B_EXEC;

            // --- Menu keys ---
            if (key == Ctrl::MENU)    return B_MENU;
            if (key == Ctrl::MODE)    return B_MODE;
            if (key == Ctrl::SET)     return B_SET;
            if (key == Ctrl::CLR)     return B_CLEAR;
            if (key == Ctrl::ABOUT)   return B_ABOUT;
            if (key == Ctrl::FMT)     return B_FMT;
            if (key == Ctrl::OPTN)    return B_OPTN;
            if (key == Ctrl::CONV)    return B_CONV;
            if (key == Ctrl::CONST)   return B_CONST;
            if (key == Ctrl::SOLV)    return B_SOLVE;
            if (key == Ctrl::CALC)    return B_CALC;
            if (key == Ctrl::FACTOR)  return B_FACTOR;
            if (key == Ctrl::EXPAND)  return B_EXPAND;

            // --- STO ---
            if (key == Ctrl::STO) { _insertAtCursor(key); return B_SUCCESS; }

            // --- GUI-edited objects ---
            if (key == CAS::FuncName::vector)  return B_VECDEF;
            if (key == CAS::FuncName::matrix)  return B_MATDEF;
            if (key == CAS::FuncName::plot2d)  return B_PLOT2D;
            if (key == CAS::FuncName::plot3d)  return B_PLOT3D;

            // --- Map special symbols ---
            if (key == Spec::SQ)  key = "^2";
            if (key == Spec::CB)  key = "^3";
            if (key == Spec::INV) key = "^(-1)";
            if (key == Spec::CBRT) {
                _insertBoxedFunc(CAS::FuncName::root, 2);
                exp.insert(cp, "3");
                cp += 1;
                _jumpToBlock(1);
                return B_SUCCESS;
            }
            if (key == Spec::EE) {
                _insertAtCursor("*10^");
                return B_SUCCESS;
            }

            // --- Boxed functions ---
            if (key == CAS::FuncName::abs || key == CAS::FuncName::sqrt) {
                return _insertBoxedFunc(key, 1);
            }
            if (key == CAS::FuncName::root || key == CAS::FuncName::log ||
                key == CAS::FuncName::permut || key == CAS::FuncName::combin ||
                key == CAS::FuncName::diff || key == CAS::FuncName::indefint) {
                return _insertBoxedFunc(key, 2);
            }
            if (key == CAS::FuncName::sum || key == CAS::FuncName::prod ||
                key == CAS::FuncName::defint) {
                return _insertBoxedFunc(key, 4);
            }

            // --- Parenthesised functions (auto left-paren) ---
            if (key.size() == 3 && (uint8_t)key[0] == 0x01) {
                if (key == CAS::FuncName::deg || key == CAS::FuncName::rad) {
                    _insertAtCursor(key);
                    return B_SUCCESS;
                }
                _insertAtCursor(key + "(");
                return B_SUCCESS;
            }

            // --- Infix/Postfix operators ---
            if (key == CAS::FuncName::dot ||
                key == CAS::FuncName::fact ||
                key == CAS::FuncName::estx1 || key == CAS::FuncName::estx2 ||
                key == CAS::FuncName::esty ||
                key == CAS::FuncName::band || key == CAS::FuncName::bor ||
                key == CAS::FuncName::bxor || key == CAS::FuncName::bnot ||
                key == CAS::FuncName::blshift || key == CAS::FuncName::brshift ||
                key == CAS::FuncName::bxnor || key == CAS::FuncName::bnand ||
                key == CAS::FuncName::bnor) {
                _insertAtCursor(key);
                return B_SUCCESS;
            }

            // --- Normal character ---
            _insertAtCursor(key);
            return B_SUCCESS;
        }

    private:
        // ==================== Insert Helpers ====================

        /**
         * @brief Insert a string at cursor, handling insert/overwrite mode
         * @param s  String to insert
         */
        void _insertAtCursor(const std::string &s) {
            if (s.empty()) return;
            if (flg & M_INSERT) {
                if (cp + (int16_t)s.size() <= (int16_t)exp.size())
                    exp.replace(cp, s.size(), s);
                else if (cp < (int16_t)exp.size())
                    exp.replace(cp, exp.size() - cp, s);
                else
                    exp.append(s);
            } else {
                exp.insert(cp, s);
            }
            cp += s.size();
            _releaseOneShot();
        }

        /**
         * @brief Insert a boxed function with given number of input blocks
         * @param func        Function name string (e.g. CAS::FuncName::sqrt)
         * @param num_blocks  Number of input boxes (1, 2, or 4)
         * @return            B_SUCCESS
         */
        uint8_t _insertBoxedFunc(const std::string &func, int num_blocks) {
            // Single-block + insert mode + cursor on operand → absorb it
            if (num_blocks == 1 && (flg & M_INSERT) && cp < (int16_t)exp.size()) {
                size_t end = _scanOperand(cp);
                if (end > (size_t)cp) {
                    std::string operand = exp.substr(cp, end - cp);
                    std::string insert_str = func + Ctrl::BLOCKL + operand + Ctrl::BLOCKR;
                    exp.replace(cp, end - cp, insert_str);
                    cp = cp + func.size() + 2;
                    _releaseOneShot();
                    return B_SUCCESS;
                }
            }

            // Normal insert: empty boxes
            std::string s = func;
            for (int i = 0; i < num_blocks; i++) {
                s += Ctrl::BLOCKL;
                s += Ctrl::BLOCKR;
            }
            _insertAtCursor(s);

            // Rewind cursor into first block
            cp -= num_blocks * 4;
            cp += func.size() + 2;
            return B_SUCCESS;
        }

        /**
         * @brief Scan right from a position to find the end of ONE operand
         * @param start  Starting position (cursor position)
         * @return       Position just past the end of the operand
         */
        size_t _scanOperand(size_t start) {
            if (start >= exp.size()) return start;

            size_t end = start;
            uint8_t first = (uint8_t)exp[start];

            if (first == '(') {
                int depth = 1;
                end = start + 1;
                while (end < exp.size() && depth > 0) {
                    if (exp[end] == '(') depth++;
                    else if (exp[end] == ')') depth--;
                    end++;
                }
            } else if (first == 0x01) {
                end = start + 2;
                if (end < exp.size() && (uint8_t)exp[end] == 0x03) {
                    int bdepth = 0;
                    while (end < exp.size()) {
                        if ((uint8_t)exp[end] == 0x03 && end + 1 < exp.size() &&
                            exp[end + 1] == Ctrl::BLOCKL[1]) {
                            bdepth++;
                        } else if ((uint8_t)exp[end] == 0x03 && end + 1 < exp.size() &&
                                   exp[end + 1] == Ctrl::BLOCKR[1]) {
                            bdepth--;
                            if (bdepth <= 0) { end += 2; break; }
                        } else if (exp[end] == '(') {
                            int pdepth = 1;
                            end++;
                            while (end < exp.size() && pdepth > 0) {
                                if (exp[end] == '(') pdepth++;
                                else if (exp[end] == ')') pdepth--;
                                end++;
                            }
                            continue;
                        }
                        end++;
                    }
                } else if (end < exp.size() && exp[end] == '(') {
                    int pdepth = 1;
                    end++;
                    while (end < exp.size() && pdepth > 0) {
                        if (exp[end] == '(') pdepth++;
                        else if (exp[end] == ')') pdepth--;
                        end++;
                    }
                }
            } else if ((first >= '0' && first <= '9') || first == '.' ||
                       (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z')) {
                end = start + 1;
                while (end < exp.size() &&
                       ((exp[end] >= '0' && exp[end] <= '9') || exp[end] == '.' ||
                        (exp[end] >= 'A' && exp[end] <= 'Z') || (exp[end] >= 'a' && exp[end] <= 'z'))) {
                    end++;
                }
            } else if (first >= 0x02 && first <= 0x05) {
                end = start + 2;
            }

            return end;
        }

        /** @brief Release one-shot modifiers (SHIFT, ALPHA, unlocked CTRL) */
        void _releaseOneShot() {
            if ((flg & M_SHIFT) && !(((flg & M_ALPHA) && (flg & M_CTRL)))) flg &= ~M_SHIFT;
            if ((flg & M_ALPHA) && !(flg & M_CTRL)) flg &= ~M_ALPHA;
            if ((flg & M_CTRL) && !(flg & M_LOCK) && !(flg & M_ALPHA)) flg &= ~M_CTRL;
            _Check();
        }

    protected:
        // ==================== Modifier / State ====================

        /** @brief Handle a modifier key press */
        void _Modifier(const std::string &k) {
            if (k == Ctrl::SHIFT) { flg ^= M_SHIFT; return; }
            if (k == Ctrl::ALPHA) { flg ^= M_ALPHA; return; }
            if (k == Ctrl::CTRL)  { flg ^= M_CTRL; flg &= ~M_LOCK; return; }
            if (k == Ctrl::LOCK)  { flg ^= M_LOCK; return; }
            if (k == Ctrl::INS)   { flg ^= M_INSERT; return; }
            if (k == Ctrl::RCL)   { flg ^= M_RCL; return; }
        }

        /** @brief Check and fix invalid modifier combinations */
        void _Check() {
            if ((flg & M_LOCK) && !(flg & M_CTRL)) flg &= ~M_LOCK;
        }

        /** @brief Process AC key: clear expression and reset all state */
        void _AC() {
            exp.clear();
            cp = 0;
            flg = 0;
        }

        // ==================== Cursor Helpers ====================

        /** @brief Get logical character width in bytes at given position */
        int _charWidth(int16_t pos) {
            if (pos < 0 || pos >= (int16_t)exp.size()) return 0;
            uint8_t c = (uint8_t)exp[pos];
            if (c >= 0x01 && c <= 0x05) return 2;
            return 1;
        }

        /** @brief Check if position is at a block boundary marker */
        bool _isBlockBoundary(int16_t pos) {
            if (pos <= 0 || pos >= (int16_t)exp.size()) return false;
            return ((uint8_t)exp[pos] == 0x03);
        }

        // ==================== Block Matching ====================

        /**
         * @brief Find the right block matching a left block
         * @param lpos  Position of the left block's \x03 marker
         * @return      Position of the matching right block's \x03 marker,
         *              or std::string::npos if not found
         */
        size_t _findRblock(size_t lpos) {
            uint16_t lCnt = 0;
            for (size_t i = lpos; i < exp.size() - 1; ++i) {
                if ((uint8_t)exp[i] == 0x03) {
                    if (exp[i + 1] == Ctrl::BLOCKL[1]) {
                        ++lCnt;
                    } else if (exp[i + 1] == Ctrl::BLOCKR[1]) {
                        if (lCnt == 0) return i;
                        --lCnt;
                    }
                }
            }
            return std::string::npos;
        }

        /**
         * @brief Find the left block matching a right block
         * @param rpos  Position of the right block's \x03 marker
         * @return      Position of the matching left block's \x03 marker,
         *              or std::string::npos if not found
         */
        size_t _findLblock(size_t rpos) {
            uint16_t rCnt = 0;
            for (size_t i = rpos; i > 0; --i) {
                if ((uint8_t)exp[i - 1] == 0x03) {
                    if (exp[i] == Ctrl::BLOCKR[1]) {
                        ++rCnt;
                    } else if (exp[i] == Ctrl::BLOCKL[1]) {
                        if (rCnt == 0) return i - 1;
                        --rCnt;
                    }
                }
            }
            return std::string::npos;
        }

        /**
         * @brief Get the function name that owns a block
         * @param _pos  Position of the block's \x03 marker
         * @return      Function name string, or empty if not found
         */
        std::string _getFunc(size_t _pos) {
            if (_pos >= exp.size() || (uint8_t)exp[_pos] != 0x03)
                return std::string();
            if (_pos + 1 >= exp.size())
                return std::string();
            if (exp[_pos + 1] == Ctrl::BLOCKR[1]) {
                _pos = _findLblock(_pos);
                if (_pos == std::string::npos) return std::string();
            }
            if (_pos >= 3 && (uint8_t)exp[_pos - 3] == 0x01)
                return exp.substr(_pos - 3, 3);
            if (_pos >= 2 && (uint8_t)exp[_pos - 2] == 0x01)
                return exp.substr(_pos - 2, 2);
            return std::string();
        }

        /**
         * @brief Collect all top-level lblock positions within a boxed function
         * @param func_start  Position of function prefix
         * @param blocks      Output vector of lblock positions
         */
        void _collectBlocks(int16_t func_start, std::vector<size_t> &blocks) {
            int depth = 0;
            for (size_t i = func_start; i < exp.size(); i++) {
                if ((uint8_t)exp[i] == 0x03) {
                    if (i + 1 < exp.size() && exp[i + 1] == Ctrl::BLOCKL[1]) {
                        if (depth == 0) blocks.push_back(i);
                        depth++;
                    } else if (i + 1 < exp.size() && exp[i + 1] == Ctrl::BLOCKR[1]) {
                        depth--;
                        if (depth <= 0 && !blocks.empty()) break;
                    }
                }
            }
        }

        /**
         * @brief Jump cursor to a specific block within the current function
         * @param target_block  Block index (0 = first operand)
         */
        void _jumpToBlock(int target_block) {
            int16_t func_start = -1;
            for (int16_t i = cp; i >= 0; i--) {
                if ((uint8_t)exp[i] == 0x03 && i + 1 < (int16_t)exp.size() &&
                    exp[i + 1] == Ctrl::BLOCKL[1]) {
                    if (i >= 3 && (uint8_t)exp[i - 3] == 0x01) func_start = i - 3;
                    else if (i >= 2 && (uint8_t)exp[i - 2] == 0x01) func_start = i - 2;
                    break;
                }
            }
            if (func_start < 0) return;

            std::vector<size_t> blocks;
            _collectBlocks(func_start, blocks);
            if (target_block >= 0 && target_block < (int)blocks.size())
                cp = blocks[target_block] + 2;
        }

        // ==================== DEL ====================

        /** @brief Process DEL key */
        uint8_t _DEL() {
            if (exp.empty() || cp <= 0 || cp > (int16_t)exp.size()) return B_ERROR;

            // 4-byte function prefix \x01??( with auto-paren
            if (cp >= 4 && (uint8_t)exp[cp - 4] == 0x01 && exp[cp - 1] == '(') {
                int depth = 1;
                size_t i = cp;
                while (i < exp.size() && depth > 0) {
                    if (exp[i] == '(') depth++;
                    else if (exp[i] == ')') depth--;
                    i++;
                }
                exp.erase(cp - 4, i - (cp - 4));
                cp -= 4;
                return B_SUCCESS;
            }

            // 3-byte function/infix/postfix \x01??
            if (cp >= 3 && (uint8_t)exp[cp - 3] == 0x01) {
                exp.erase(cp - 3, 3);
                cp -= 3;
                return B_SUCCESS;
            }

            // Block marker \x03
            if (cp >= 2 && (uint8_t)exp[cp - 2] == 0x03) {
                if (exp[cp - 1] == Ctrl::BLOCKL[1]) return _deleteLblock(cp - 2);
                if (exp[cp - 1] == Ctrl::BLOCKR[1]) {
                    std::string func = _getFunc(cp - 2);
                    if (func == CAS::FuncName::vector) return B_VECEDT;
                    if (func == CAS::FuncName::matrix) return B_MATEDT;
                    _Move(Ctrl::X_MINUS);
                    return B_SUCCESS;
                }
                if (exp[cp - 1] == Ctrl::STO[1]) {
                    exp.erase(cp - 2, 2);
                    cp -= 2;
                    return B_SUCCESS;
                }
                return B_ERROR;
            }

            // 2-byte token (const/var/spec) \x02-\x05
            if (cp >= 2 && (uint8_t)exp[cp - 2] >= 0x02 && (uint8_t)exp[cp - 2] <= 0x05) {
                exp.erase(cp - 2, 2);
                cp -= 2;
                return B_SUCCESS;
            }

            // Single character
            exp.erase(cp - 1, 1);
            --cp;
            return B_SUCCESS;
        }

        /**
         * @brief Delete a left block and its associated function structure
         * @param lpos  Position of the left block's \x03 marker
         * @return      Status code
         */
        uint8_t _deleteLblock(size_t lpos) {
            if (lpos >= 3 && (uint8_t)exp[lpos - 3] == 0x01) {
                std::string func = exp.substr(lpos - 3, 3);

                // Single block: sqrt, abs
                if (func == CAS::FuncName::sqrt || func == CAS::FuncName::abs) {
                    size_t rpos = _findRblock(lpos);
                    if (rpos == std::string::npos) return B_ERROR;
                    exp.erase(rpos, 2);
                    exp.erase(lpos - 3, lpos + 2 - (lpos - 3));
                    cp = lpos - 3;
                    return B_SUCCESS;
                }

                // Double block (non-log): root, permut, combin
                if (func == CAS::FuncName::root || func == CAS::FuncName::permut ||
                    func == CAS::FuncName::combin) {
                    size_t r1 = _findRblock(lpos);
                    if (r1 == std::string::npos) return B_ERROR;
                    size_t r2 = _findRblock(r1 + 2);
                    if (r2 != std::string::npos) {
                        exp.erase(r2, 2);         // 2nd rblock
                        exp.erase(r1, 4);         // 1st rblock + 2nd lblock
                        exp.erase(lpos - 3, lpos + 2 - (lpos - 3));
                        cp = lpos - 3;
                        return B_SUCCESS;
                    }
                    return B_ERROR;
                }

                // log: special cursor jump behavior
                if (func == CAS::FuncName::log) {
                    size_t r1 = _findRblock(lpos);
                    if (r1 == std::string::npos) return B_ERROR;
                    size_t r2 = _findRblock(r1 + 2);
                    if (r2 != std::string::npos) { cp = r2; return B_SUCCESS; }
                    return B_ERROR;
                }

                // Multi-block: sum, prod, defint, diff, indefint
                if (func == CAS::FuncName::sum || func == CAS::FuncName::prod ||
                    func == CAS::FuncName::defint || func == CAS::FuncName::diff ||
                    func == CAS::FuncName::indefint) {
                    size_t r1 = _findRblock(lpos);
                    if (r1 == std::string::npos) return B_ERROR;
                    size_t r2 = _findRblock(r1 + 2);
                    if (r2 == std::string::npos) return B_ERROR;
                    if (func != CAS::FuncName::diff && func != CAS::FuncName::indefint) {
                        r2 = _findRblock(r2 + 2);
                        if (r2 == std::string::npos) return B_ERROR;
                        r2 = _findRblock(r2 + 2);
                        if (r2 == std::string::npos) return B_ERROR;
                    }
                    exp.erase(r1, r2 - r1 + 2);
                    exp.erase(lpos - 3, lpos + 2 - (lpos - 3));
                    cp = lpos - 3;
                    return B_SUCCESS;
                }
            } else {
                // Standalone block (no function prefix)
                size_t rpos = _findRblock(lpos);
                if (rpos != std::string::npos) {
                    exp.erase(rpos, 2);
                    exp.erase(lpos, 2);
                    cp = lpos;
                    return B_SUCCESS;
                }
            }
            return B_ERROR;
        }

        // ==================== Move ====================

        /**
         * @brief Move cursor within expression (Casio-style)
         * 
         * X+/X- : Move right/left one logical character.
         * Y+    : If expression is empty, recall previous history entry.
         *         If inside a multi-block function, jump to the visually
         *         "upper" input box. Otherwise do nothing.
         * Y-    : If expression is empty, recall next history entry.
         *         If inside a multi-block function, jump to the visually
         *         "lower" input box. Otherwise do nothing.
         * 
         * Multi-block box layout:
         *   log [a][b] = log_b(a)       → [b] upper (base), [a] lower
         *   root [n][x]                 → [n] upper (index), [x] lower
         *   permut/combin [n][k]        → [k] upper, [n] lower
         *   sum/prod/defint [exp][var][from][to]
         *        Upper:  [to]      (blocks[3])
         *        Middle: [exp][var] (blocks[0], blocks[1])
         *        Lower:  [from]    (blocks[2])
         *   diff/indefint [exp][var]   → [exp] left, [var] right
         */
        uint8_t _Move(const std::string &k) {
            if (k == Ctrl::X_PLUS) {
                if (cp < (int16_t)exp.size()) cp += _charWidth(cp);
                return B_SUCCESS;
            }
            if (k == Ctrl::X_MINUS) {
                if (cp > 0) {
                    if (cp >= 2 && (uint8_t)exp[cp - 2] >= 0x01 && (uint8_t)exp[cp - 2] <= 0x05)
                        cp -= 2;
                    else
                        cp -= 1;
                }
                return B_SUCCESS;
            }

            bool up = (k == Ctrl::Y_PLUS);
            if (exp.empty()) return up ? B_HISTUP : B_HISTDN;

            int16_t func_start = -1;
            std::string func_name;
            for (int16_t i = cp; i >= 0; i--) {
                if ((uint8_t)exp[i] == 0x03 && i + 1 < (int16_t)exp.size() &&
                    exp[i + 1] == Ctrl::BLOCKL[1]) {
                    if (i >= 3 && (uint8_t)exp[i - 3] == 0x01) {
                        func_start = i - 3;
                        func_name = exp.substr(i - 3, 3);
                    } else if (i >= 2 && (uint8_t)exp[i - 2] == 0x01) {
                        func_start = i - 2;
                        func_name = exp.substr(i - 2, 2);
                    }
                    break;
                }
            }
            if (func_start < 0 || func_name.empty()) return B_SUCCESS;

            std::vector<size_t> blocks;
            _collectBlocks(func_start, blocks);
            int num_blocks = blocks.size();
            if (num_blocks <= 1) return B_SUCCESS;

            int current_block = -1;
            for (int i = num_blocks - 1; i >= 0; i--) {
                if (cp >= (int16_t)blocks[i]) { current_block = i; break; }
            }
            if (current_block < 0) return B_SUCCESS;

            int target = current_block;

            if (num_blocks == 2) {
                bool upper_is_1 = (func_name == CAS::FuncName::log ||
                                   func_name == CAS::FuncName::root ||
                                   func_name == CAS::FuncName::permut ||
                                   func_name == CAS::FuncName::combin);
                target = upper_is_1 ? (up ? 1 : 0) : (up ? 0 : 1);
            } else if (num_blocks == 4) {
                if (current_block == 3)      target = up ? 3 : 0;
                else if (current_block == 2) target = up ? 1 : 2;
                else                         target = up ? 3 : 2;
            }

            cp = blocks[target] + 2;
            return B_SUCCESS;
        }
    };

} // namespace Keypad

#endif // _EXPBUILD_HPP_