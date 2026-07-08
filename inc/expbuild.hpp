/** @file /inc/expbuild.hpp
 *  @brief Builds internal expression strings from key presses
 *  @author hdkghc
 *  @version 0.2
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
     *  @brief Bitmask flags for the modifier state byte.
     *  @{ */
    constexpr uint8_t M_SHIFT  = 0x01; ///< SHIFT modifier active (one-shot)
    constexpr uint8_t M_ALPHA  = 0x02; ///< ALPHA modifier active (one-shot)
    constexpr uint8_t M_CTRL   = 0x04; ///< CTRL modifier active
    constexpr uint8_t M_LOCK   = 0x08; ///< CTRL lock engaged
    constexpr uint8_t M_INSERT = 0x10; ///< Insert/Overwrite toggle
    constexpr uint8_t M_RCL    = 0x20; ///< RCL (recall) mode active
    /** @} */

    /** @name Return codes
     *  @brief Status codes returned by Expbuild::press().
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
    constexpr uint8_t B_CLRSCR  = 0x17; ///< Clear screen (ON key)
    constexpr uint8_t B_OFF     = 0xFE; ///< Power off
    constexpr uint8_t B_ERROR   = 0xFF; ///< Generic error
    /** @} */

    /** @brief Byte length of a block marker.
     *  @details Both \x03\x20 (left) and \x03\x21 (right) are 2 bytes. */
    constexpr int BLKLEN = 2;

    // =================================================================
    // Character classification helpers
    //
    // Byte layout of enriched expression tokens:
    //   Lead byte    Payload      Meaning                  Width
    //   ---------------------------------------------------------
    //   \x01         2 bytes      CAS function name        3 B
    //   \x02         1 byte       Math/physical constant   2 B
    //   \x03         1 byte       Control char             2 B
    //   \x04         1 byte       Unit conversion          2 B
    //   \x05         1 byte       SI prefix                2 B
    //   \x06         1 byte       Special variable         2 B
    //   ASCII        —            Ordinary character       1 B
    //
    // Atomic groups (cursor must never split them):
    //   • Every multi-byte token (\x01…\x06)
    //   • \x01xx( — a parenthesised function call where
    //     the opening parenthesis is bound to the function name.
    // =================================================================

    /**
     * @brief  Return the byte width of the logical character at pos.
     * @param  e   Expression string.
     * @param  pos Byte offset to examine.
     * @return     1, 2, or 3; 0 if pos is out of range.
     */
    inline int _chW(const std::string &e, int16_t pos) {
        if (pos < 0 || pos >= (int16_t)e.size()) return 0;
        uint8_t lead = (uint8_t)e[pos];
        if (lead >= 0x01 && lead <= 0x06) {
            if (lead == 0x01 && pos + 2 < (int16_t)e.size()) return 3;
            if (pos + 1 < (int16_t)e.size()) return 2;
        }
        return 1;
    }

    // =================================================================
    // Block marker detection
    // =================================================================

    /** @brief True if pos is at any \x03 control character. */
    inline bool _isBlk(const std::string &e, int16_t pos) {
        return pos >= 0 && pos + 1 < (int16_t)e.size() && (uint8_t)e[pos] == 0x03;
    }

    /** @brief True if pos is a left input-block marker (\x03\x20). */
    inline bool _isLblk(const std::string &e, int16_t pos) {
        return _isBlk(e, pos) && e[pos + 1] == Ctrl::BLOCKL[1];
    }

    /** @brief True if pos is a right input-block marker (\x03\x21). */
    inline bool _isRblk(const std::string &e, int16_t pos) {
        return _isBlk(e, pos) && e[pos + 1] == Ctrl::BLOCKR[1];
    }

    /**
     * @brief  True if the '(' at pos is bound to a preceding \x01 function.
     * @details In \x01xx( the '(' is part of the atomic unit and the cursor
     *          may never stop between the function name and the parenthesis.
     */
    inline bool _isFuncParen(const std::string &e, int16_t pos) {
        return pos >= 3 && pos < (int16_t)e.size() &&
               e[pos] == '(' && (uint8_t)e[pos - 3] == 0x01;
    }

    // =================================================================
    // Block matching
    // =================================================================

    /**
     * @brief  Find the right block that matches the left block at lpos.
     * @param  e    Expression string.
     * @param  lpos Position of the left block's \x03 marker.
     * @return      Position of the matching \x03 marker, or -1.
     */
    inline int16_t _findR(const std::string &e, int16_t lpos) {
        int d = 0;
        for (int16_t i = lpos; i + 1 < (int16_t)e.size(); i++) {
            if ((uint8_t)e[i] == 0x03) {
                if (e[i + 1] == Ctrl::BLOCKL[1]) d++;
                else if (e[i + 1] == Ctrl::BLOCKR[1]) {
                    if (d == 0) return i;
                    d--;
                }
            }
        }
        return -1;
    }

    /**
     * @brief  Find the left block that matches the right block at rpos.
     * @param  e    Expression string.
     * @param  rpos Position of the right block's \x03 marker.
     * @return      Position of the matching \x03 marker, or -1.
     */
    inline int16_t _findL(const std::string &e, int16_t rpos) {
        int d = 0;
        for (int16_t i = rpos; i >= 0; i--) {
            if ((uint8_t)e[i] == 0x03) {
                if (e[i + 1] == Ctrl::BLOCKR[1]) d++;
                else if (e[i + 1] == Ctrl::BLOCKL[1]) {
                    if (d == 0) return i;
                    d--;
                }
            }
        }
        return -1;
    }

    /**
     * @brief  Collect the top-level left-block positions of a boxed function.
     * @param  e         Expression string.
     * @param  funcStart Byte offset of the function's \x01 prefix.
     * @param  out       Output vector filled with \x03 positions (ascending).
     */
    inline void _blocks(const std::string &e, int16_t funcStart,
                        std::vector<int16_t> &out) {
        int d = 0;
        for (int16_t i = funcStart; i < (int16_t)e.size(); i++) {
            if (_isLblk(e, i)) {
                if (d == 0) out.push_back(i);
                d++;
            } else if (_isRblk(e, i)) {
                d--;
                if (d <= 0 && !out.empty()) break;
            }
        }
    }

    /**
     * @brief  Return the function name that owns the block at pos.
     * @param  e   Expression string.
     * @param  pos Position of either left or right block \x03 marker.
     * @return     3-byte function name, or empty string.
     */
    inline std::string _fnAtBlk(const std::string &e, int16_t pos) {
        if (!_isBlk(e, pos)) return "";
        int16_t lpos = _isLblk(e, pos) ? pos : _findL(e, pos);
        if (lpos < 3 || (uint8_t)e[lpos - 3] != 0x01) return "";
        return e.substr(lpos - 3, 3);
    }

    // =================================================================
    // Expbuild
    // =================================================================

    /**
     * @class  Expbuild
     * @brief  Converts key presses into an internal expression string with
     *         cursor management, replicating Casio calculator input logic.
     *
     * The expression uses an enriched bytecode where bytes 0x01–0x06
     * introduce multi-byte tokens (see the table above).  ASCII bytes
     * represent ordinary digits, letters, operators, and spaces.
     *
     * The cursor (@c cp) is a byte offset and always sits on a logical
     * character boundary — never inside a multi-byte token or between a
     * function name and its opening parenthesis.
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
             * @return Status code (B_SUCCESS, B_EXEC, …).
             *
             * Modifier keys toggle internal flags.  Printable keys are inserted
             * at the cursor position, respecting insert/overwrite mode.
             * Special keys trigger navigation, deletion, or menu actions.
             */
            uint8_t press(uint8_t r, uint8_t c) {
                std::string k = getKey(r, c, flg & 0x7);
                if (k.empty()) return B_SUCCESS;

                // ----- Modifier keys ----------------------------------------
                if (k == Ctrl::SHIFT) { flg ^= M_SHIFT; return B_SUCCESS; }
                if (k == Ctrl::ALPHA) { flg ^= M_ALPHA; return B_SUCCESS; }
                if (k == Ctrl::CTRL)  { flg ^= M_CTRL; flg &= ~M_LOCK; return B_SUCCESS; }
                if (k == Ctrl::LOCK)  { flg ^= M_LOCK; return B_SUCCESS; }
                if (k == Ctrl::INS)   { flg ^= M_INSERT; return B_SUCCESS; }
                if (k == Ctrl::RCL)   { flg ^= M_RCL; return B_SUCCESS; }

                // ----- Global controls --------------------------------------
                if (k == Ctrl::ON)  return B_CLRSCR;
                if (k == Ctrl::OFF) return B_OFF;
                if (k == Ctrl::AC)  { exp.clear(); cp = 0; flg = 0; return B_SUCCESS; }

                // ----- Navigation -------------------------------------------
                if (k == Ctrl::X_PLUS)  { _right(); return B_SUCCESS; }
                if (k == Ctrl::X_MINUS) { _left();  return B_SUCCESS; }
                if (k == Ctrl::Y_PLUS || k == Ctrl::Y_MINUS) {
                    if (exp.empty()) return (k == Ctrl::Y_PLUS) ? B_HISTUP : B_HISTDN;
                    _vert(k == Ctrl::Y_PLUS);
                    return B_SUCCESS;
                }

                // ----- Deletion ---------------------------------------------
                if (k == Ctrl::DEL) return _del();

                // ----- Execution --------------------------------------------
                if (k == Ctrl::EXE || k == Ctrl::OK) return B_EXEC;

                // ----- Menu / system keys -----------------------------------
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

                // ----- STO (store arrow, enters expression as \x03\x22) -----
                if (k == Ctrl::STO) { _ins(k); return B_SUCCESS; }

                // ----- GUI-edited objects (deferred to external editor) -----
                if (k == CAS::FuncName::vector)  return B_VECDEF;
                if (k == CAS::FuncName::matrix)  return B_MATDEF;
                if (k == CAS::FuncName::plot2d)  return B_PLOT2D;
                if (k == CAS::FuncName::plot3d)  return B_PLOT3D;

                // ----- Spec symbol → ASCII substitution ---------------------
                if (k == Spec::SQ)  { _ins("^2");     return B_SUCCESS; }
                if (k == Spec::CB)  { _ins("^3");     return B_SUCCESS; }
                if (k == Spec::INV) { _ins("^(-1)");  return B_SUCCESS; }
                if (k == Spec::EE)  { _ins("*10^");   return B_SUCCESS; }
                if (k == Spec::CBRT) {
                    _boxed(CAS::FuncName::root, 2);
                    _raw("3");
                    _jumpBlk(1);
                    return B_SUCCESS;
                }

                // ----- Boxed functions (with \x03\x20…\x03\x21 blocks) ------
                // 1-block
                if (k == CAS::FuncName::abs || k == CAS::FuncName::sqrt)
                    return _boxed(k, 1);
                // 2-block
                if (k == CAS::FuncName::root || k == CAS::FuncName::log ||
                    k == CAS::FuncName::permut || k == CAS::FuncName::combin ||
                    k == CAS::FuncName::diff || k == CAS::FuncName::indefint)
                    return _boxed(k, 2);
                // 4-block
                if (k == CAS::FuncName::sum || k == CAS::FuncName::prod ||
                    k == CAS::FuncName::defint)
                    return _boxed(k, 4);

                // ----- Parenthesised functions (\x01xx( atomic) -------------
                if (k.size() == 3 && (uint8_t)k[0] == 0x01) {
                    if (k == CAS::FuncName::deg || k == CAS::FuncName::rad) {
                        _ins(k);            // no parenthesis
                    } else {
                        _ins(k + "(");      // \x01xx( — atomic unit
                    }
                    return B_SUCCESS;
                }

                // ----- Everything else --------------------------------------
                // Digits, letters, operators, ',', ';', '=', space,
                // \x02 constants, \x04 conversions, \x05 SI prefixes,
                // \x06 special variables.
                _ins(k);
                return B_SUCCESS;
            }

        private:
            // =============================================================
            // Insert helpers
            // =============================================================

            /**
             * @brief  Raw string insertion at cursor (no modifier release).
             * @param  s String to insert.
             * @details In overwrite mode replaces as many display-width bytes
             *          as @p s occupies; otherwise shifts content right.
             */
            void _raw(const std::string &s) {
                if (s.empty()) return;
                if (flg & M_INSERT) {
                    int16_t w = 0, p = cp;
                    for (size_t i = 0; i < s.size() && p < (int16_t)exp.size(); i++) {
                        int cw = _chW(exp, p);
                        w += cw; p += cw;
                    }
                    if (w > 0) exp.replace(cp, w, s);
                    else       exp.insert(cp, s);
                } else {
                    exp.insert(cp, s);
                }
                cp += (int16_t)s.size();
            }

            /**
             * @brief  Insert string and release one-shot modifiers.
             * @param  s String to insert.
             */
            void _ins(const std::string &s) { _raw(s); _rel(); }

            /**
             * @brief  Insert a boxed function with @p n empty input blocks.
             * @param  fn 3-byte function name (\x01xx).
             * @param  n  Number of blocks (1, 2, or 4).
             * @return    B_SUCCESS.
             *
             * @details The function is inserted as
             *          @c fn + (\\x03\\x20\\x03\\x21) × n.
             *          The cursor lands inside the first block, immediately
             *          after its opening \\x03\\x20.
             *
             *          In overwrite mode with a single block, attempts to
             *          absorb the operand under the cursor into the block.
             */
            uint8_t _boxed(const std::string &fn, int n) {
                // Overwrite absorption (1-block only)
                if (n == 1 && (flg & M_INSERT) && cp < (int16_t)exp.size()) {
                    int16_t end = _scanOp(cp);
                    if (end > cp) {
                        std::string op = exp.substr(cp, end - cp);
                        std::string w  = fn + Ctrl::BLOCKL + op + Ctrl::BLOCKR;
                        exp.replace(cp, end - cp, w);
                        cp = cp + (int16_t)fn.size() + BLKLEN;
                        _rel();
                        return B_SUCCESS;
                    }
                }

                int16_t insPos = cp;
                std::string s = fn;
                for (int i = 0; i < n; i++) { s += Ctrl::BLOCKL; s += Ctrl::BLOCKR; }
                _raw(s);
                // Cursor into first block: skip function name + first \x03\x20
                cp = insPos + (int16_t)fn.size() + BLKLEN;
                _rel();
                return B_SUCCESS;
            }

            /**
             * @brief  Scan forward from @p pos to find the end of one operand.
             * @param  pos Starting byte offset.
             * @return     Offset just past the operand.
             *
             * Handles parenthesised groups, boxed functions, numbers,
             * identifiers, and multi-byte tokens.
             */
            int16_t _scanOp(int16_t pos) const {
                if (pos >= (int16_t)exp.size()) return pos;
                uint8_t lead = (uint8_t)exp[pos];

                // Parenthesised expression
                if (lead == '(') {
                    int d = 1; int16_t i = pos + 1;
                    while (i < (int16_t)exp.size() && d > 0) {
                        if (exp[i] == '(') d++;
                        else if (exp[i] == ')') d--;
                        i++;
                    }
                    return i;
                }

                // Boxed function or parenthesised function
                if (lead == 0x01 && pos + 2 < (int16_t)exp.size()) {
                    int16_t i = pos + 3;
                    if (i < (int16_t)exp.size() && _isLblk(exp, i)) {
                        int d = 0;
                        while (i < (int16_t)exp.size()) {
                            if (_isLblk(exp, i)) d++;
                            else if (_isRblk(exp, i)) {
                                if (--d <= 0) { i += BLKLEN; break; }
                            } else if (exp[i] == '(') {
                                int pd = 1; i++;
                                while (i < (int16_t)exp.size() && pd > 0) {
                                    if (exp[i] == '(') pd++;
                                    else if (exp[i] == ')') pd--;
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
                            if (exp[i] == '(') pd++;
                            else if (exp[i] == ')') pd--;
                            i++;
                        }
                        return i;
                    }
                    return i;
                }

                // Number or identifier (e.g. "hello", "h2", "3.14")
                if ((lead >= '0' && lead <= '9') || lead == '.' ||
                    (lead >= 'A' && lead <= 'Z') || (lead >= 'a' && lead <= 'z')) {
                    int16_t i = pos + 1;
                    while (i < (int16_t)exp.size() &&
                        ((exp[i] >= '0' && exp[i] <= '9') || exp[i] == '.' ||
                            (exp[i] >= 'A' && exp[i] <= 'Z') ||
                            (exp[i] >= 'a' && exp[i] <= 'z'))) i++;
                    return i;
                }

                // Multi-byte token (\x02…\x06)
                if (lead >= 0x02 && lead <= 0x06) return pos + 2;

                // Single-byte character
                return pos + 1;
            }

            // =============================================================
            // Modifier state machine
            // =============================================================

            /**
             * @brief  Release one-shot modifiers after a non-modifier key.
             *
             * SHIFT and ALPHA latch for one key press unless held together
             * with CTRL.  CTRL latches for one press unless ALPHA is active
             * or the CTRL lock is on.
             */
            void _rel() {
                // SHIFT: released unless SHIFT+ALPHA+CTRL held
                if ((flg & M_SHIFT) && !((flg & M_ALPHA) && (flg & M_CTRL)))
                    flg &= ~M_SHIFT;
                // ALPHA: released unless CTRL held
                if ((flg & M_ALPHA) && !(flg & M_CTRL))
                    flg &= ~M_ALPHA;
                // CTRL: released if not locked and ALPHA not held
                if ((flg & M_CTRL) && !(flg & M_LOCK) && !(flg & M_ALPHA))
                    flg &= ~M_CTRL;
                // LOCK is meaningless without CTRL
                if ((flg & M_LOCK) && !(flg & M_CTRL))
                    flg &= ~M_LOCK;
            }

            // =============================================================
            // Cursor movement
            // =============================================================

            /**
             * @brief  Move cursor left by one logical character.
             *
             * Skips over multi-byte tokens as a whole.  If the character is
             * '(' and it is bound to a \x01 function, skips the entire
             * \x01xx( unit.
             *
             * For vector/matrix boxed functions, treats the whole function
             * as an opaque block — cursor jumps over it in one step.
             */
            void _left() {
                if (cp <= 0) return;
                for (int16_t i = cp - 1; i >= 0; i--) {
                    int cw = _chW(exp, i);
                    if (i + cw == cp) {
                        if (_isFuncParen(exp, i)) {
                            cp = i - 3;          // skip \x01xx( as a unit
                        } else if (_isLblk(exp, i)) {
                            // Skip the whole boxed function (for vector/matrix)
                            std::string fn = _fnAtBlk(exp, i);
                            if (fn == CAS::FuncName::vector ||
                                fn == CAS::FuncName::matrix) {
                                std::vector<int16_t> blk;
                                _blocks(exp, i, blk); // funcStart is actually i?
                                // func is at i-3
                                int16_t r = _findR(exp, blk.empty() ? i : blk.back());
                                cp = (i >= 3) ? (i - 3) : 0;
                                continue;
                            }
                            cp = i;
                        } else {
                            cp = i;
                        }
                        return;
                    }
                }
                cp = 0;
            }

            /**
             * @brief  Move cursor right by one logical character.
             *
             * If the cursor sits on a \x01 function whose next byte is '(',
             * skips the whole \x01xx( unit.
             *
             * For vector/matrix boxed functions, treats them as opaque blocks
             * and jumps over the entire function in one step.
             */
            void _right() {
                if (cp >= (int16_t)exp.size()) return;

                uint8_t lead = (uint8_t)exp[cp];

                // \x01xx( — skip function + '('
                if (lead == 0x01 && cp + 3 < (int16_t)exp.size() &&
                    exp[cp + 3] == '(') {
                    cp += 4;
                    return;
                }

                // Boxed vector/matrix → skip whole function
                if (lead == 0x01 && cp + 2 < (int16_t)exp.size()) {
                    std::string fn = exp.substr(cp, 3);
                    if (fn == CAS::FuncName::vector ||
                        fn == CAS::FuncName::matrix) {
                        // Find end of the function
                        if (cp + 3 < (int16_t)exp.size() && _isLblk(exp, cp + 3)) {
                            std::vector<int16_t> blk;
                            _blocks(exp, cp, blk);
                            if (!blk.empty()) {
                                int16_t r = _findR(exp, blk.back());
                                if (r >= 0) { cp = r + BLKLEN; return; }
                            }
                        }
                        cp += 3; // fallback: just skip function name
                        return;
                    }
                }

                cp += _chW(exp, cp);
                if (cp > (int16_t)exp.size()) cp = (int16_t)exp.size();
            }

            /**
             * @brief  Vertical cursor movement (Y+/Y- keys).
             * @param  up @c true for upward, @c false for downward.
             *
             * Only effective inside boxed functions with multiple input
             * blocks that are stacked vertically.  Visual layouts:
             *
             * <b>2-block functions</b> (blocks[1] upper, blocks[0] lower):
             *   @li log [a][b] = log_b(a)
             *   @li root [n][x]
             *   @li permut / combin [n][k]
             *
             * <b>4-block functions</b> (sum / prod / defint):
             *   @li Upper:  [to]       (blocks[3])
             *   @li Middle: [exp][var]  (blocks[0], blocks[1])
             *   @li Lower:  [from]     (blocks[2])
             *
             * @note diff / indefint are 2-block but arranged horizontally
             *       (exp left, var right) — vertical moves do nothing.
             * @note Single-block functions and non-boxed expressions are
             *       not affected; the call is silently ignored.
             */
            void _vert(bool up) {
                // Locate the enclosing boxed function
                int16_t funcStart = -1;
                for (int16_t i = cp - 1; i >= 0; i--) {
                    if (_isLblk(exp, i)) {
                        if (i >= 3 && (uint8_t)exp[i - 3] == 0x01)
                            funcStart = i - 3;
                        break;
                    }
                    if (_isRblk(exp, i)) break;
                }
                if (funcStart < 0) return;

                std::string fn = exp.substr(funcStart, 3);
                std::vector<int16_t> blk;
                _blocks(exp, funcStart, blk);
                int nb = (int)blk.size();
                if (nb <= 1) return;

                // diff / indefint — horizontal only
                if (nb == 2 && (fn == CAS::FuncName::diff ||
                                fn == CAS::FuncName::indefint)) return;

                // Find current block
                int cur = -1;
                for (int idx = 0; idx < nb; idx++) {
                    int16_t r = _findR(exp, blk[idx]);
                    if (r >= 0 && cp >= blk[idx] && cp <= r + BLKLEN)
                        { cur = idx; break; }
                }
                if (cur < 0) return;

                int tgt = cur;
                if (nb == 2) {
                    // blocks[1] = upper, blocks[0] = lower
                    tgt = up ? 1 : 0;
                } else if (nb == 4) {
                    // sum / prod / defint layout
                    if (cur == 3)      tgt = up ? 3 : 0;     // to ↔ exp
                    else if (cur == 2) tgt = up ? 0 : 2;     // from ↔ exp
                    else               tgt = up ? 3 : 2;     // exp/var → to/from
                }
                cp = blk[tgt] + BLKLEN;
            }

            /**
             * @brief  Jump cursor to a specific block inside the enclosing
             *         boxed function.
             * @param  idx Zero-based block index.
             */
            void _jumpBlk(int idx) {
                int16_t funcStart = -1;
                for (int16_t i = cp; i >= 0; i--) {
                    if (_isLblk(exp, i)) {
                        if (i >= 3 && (uint8_t)exp[i - 3] == 0x01)
                            funcStart = i - 3;
                        break;
                    }
                }
                if (funcStart < 0) return;
                std::vector<int16_t> blk;
                _blocks(exp, funcStart, blk);
                if (idx >= 0 && idx < (int)blk.size())
                    cp = blk[idx] + BLKLEN;
            }

            // =============================================================
            // Deletion
            // =============================================================

            /**
             * @brief  Handle the DEL (backspace) key.
             * @return B_SUCCESS, B_VECEDT, B_MATEDT, or B_ERROR.
             *
             * Deletion order:
             *   1. Cursor immediately after a BLOCKR (\x03\x21):
             *      - log:  jump cursor into that block (Casio quirk).
             *      - vector / matrix:  return B_VECEDT / B_MATEDT
             *        to invoke the GUI editor.
             *      - Other boxed functions: delete the whole function.
             *   2. Cursor inside a block (between BLOCKL and BLOCKR):
             *      delete the enclosing boxed function.
             *   3. Otherwise: backspace one logical character to the left.
             */
            uint8_t _del() {
                if (exp.empty() || cp <= 0) return B_ERROR;

                // --- Case 1: cursor right after a BLOCKR ------------------
                if (cp >= BLKLEN && _isRblk(exp, cp - BLKLEN)) {
                    int16_t rp = cp - BLKLEN;
                    std::string fn = _fnAtBlk(exp, rp);
                    if (fn == CAS::FuncName::vector) return B_VECEDT;
                    if (fn == CAS::FuncName::matrix) return B_MATEDT;
                    if (fn == CAS::FuncName::log) {
                        cp = rp + BLKLEN;    // jump into block (Casio log quirk)
                        return B_SUCCESS;
                    }
                    return _delFunc(rp);
                }

                // --- Case 2: cursor inside a block -------------------------
                for (int16_t i = cp - 1; i >= 0; i--) {
                    if (_isLblk(exp, i)) {
                        int16_t r = _findR(exp, i);
                        if (r >= 0 && cp <= r + BLKLEN && cp >= i + BLKLEN)
                            return _delFunc(i);
                        break;
                    }
                    if (_isRblk(exp, i)) break; // outside any block
                }

                // --- Case 3: ordinary backspace ----------------------------
                return _delChar();
            }

            /**
             * @brief  Delete an entire boxed function given its left block.
             * @param  lpos Position of the left block's \x03 marker.
             * @return      B_SUCCESS, or B_ERROR on malformed expression.
             */
            uint8_t _delFunc(int16_t blockPos) {
                // blockPos may be either BLOCKL or BLOCKR — normalise to BLOCKL
                int16_t lpos = _isLblk(exp, blockPos) ? blockPos : _findL(exp, blockPos);
                if (lpos < 0 || lpos < 3 || (uint8_t)exp[lpos - 3] != 0x01) return B_ERROR;
                
                std::vector<int16_t> blk;
                _blocks(exp, lpos - 3, blk);
                if (blk.empty()) return B_ERROR;
                int16_t lastR = _findR(exp, blk.back());
                if (lastR < 0) return B_ERROR;
                exp.erase(lpos - 3, lastR + BLKLEN - (lpos - 3));
                cp = lpos - 3;
                return B_SUCCESS;
            }

            /**
             * @brief  Delete one logical character to the left of the cursor.
             * @return B_SUCCESS or B_ERROR.
             *
             * If the character is '(' bound to a \x01 function, deletes the
             * entire \x01xx(...) unit.
             */
            uint8_t _delChar() {
                // Find the byte where the character left of cp begins
                int16_t start = cp;
                for (int16_t i = cp - 1; i >= 0; i--) {
                    int cw = _chW(exp, i);
                    if (i + cw == cp) { start = i; break; }
                }
                if (start == cp) return B_ERROR;
                int len = cp - start;

                // '(' bound to \x01 function → delete \x01xx( … )
                if (_isFuncParen(exp, start)) {
                    int d = 1; int16_t i = start + 1;
                    while (i < (int16_t)exp.size() && d > 0) {
                        if (exp[i] == '(') d++;
                        else if (exp[i] == ')') d--;
                        i++;
                    }
                    exp.erase(start - 3, i - (start - 3));
                    cp = start - 3;
                    return B_SUCCESS;
                }

                exp.erase(start, len);
                cp = start;
                return B_SUCCESS;
            }
    };

} // namespace Keypad

#endif // _EXPBUILD_HPP_