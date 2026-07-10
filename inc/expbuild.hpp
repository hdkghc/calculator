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
    constexpr uint8_t B_RESET   = 0x17; ///< Reset (ON key)
    constexpr uint8_t B_OFF     = 0xFE; ///< Power off
    constexpr uint8_t B_ERROR   = 0xFF; ///< Generic error
    /** @} */

    /** @brief Byte length of a block marker.
     *  @details Both \x03\x20 (left) and \x03\x21 (right) are 2 bytes. */
    constexpr int BLKLEN = 2;

    // =================================================================
    // Token types
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
    //   • Every multi-byte token (\x01…\x06).
    //   • \x01xx( — parenthesised function call where the
    //     opening parenthesis is bound to the function name.
    // =================================================================

    /** @brief Logical classification of a token in the expression string. */
    enum class TokType : uint8_t {
        ASCII,      ///< Single-byte: digit, letter, operator, space, etc.
        FUNC,       ///< \x01xx — 3-byte function name
        CONST,      ///< \x02x  — 2-byte constant
        CTRL,       ///< \x03x  — 2-byte control (BLOCKL, BLOCKR, STO, …)
        CONV,       ///< \x04x  — 2-byte unit conversion
        SI,         ///< \x05x  — 2-byte SI prefix
        VAR         ///< \x06x  — 2-byte special variable
    };

    /**
     * @brief  A single logical token within the expression string.
     * @details Each token spans a contiguous byte range [beg, end).
     *          For BLOCKL/BLOCKR tokens, @c pair holds the index of the
     *          matching partner in the token list; for all other tokens
     *          it is -1.
     */
    struct Token {
        TokType type;       ///< Token classification
        int16_t beg;        ///< Starting byte offset in exp
        int16_t end;        ///< One-past-end byte offset
        int16_t pair;       ///< Matching BLOCKL/BLOCKR index, or -1

        /** @brief Byte width of this token (end - beg). */
        int width() const { return end - beg; }
    };

    // =================================================================
    // Tokeniser
    // =================================================================

    /**
     * @brief  Parse the expression string into a flat list of tokens.
     * @param  exp Expression string in enriched bytecode.
     * @return     Vector of Token structs with block pairs resolved.
     *
     * @details Two-pass algorithm:
     *          1. Scan byte-by-byte, creating tokens based on lead bytes.
     *          2. Match \x03\x20 (BLOCKL) with \x03\x21 (BLOCKR) using
     *             a stack, filling the @c pair field of each.
     */
    inline std::vector<Token> _tokenize(const std::string &exp) {
        std::vector<Token> toks;
        int16_t pos = 0;
        int16_t n   = (int16_t)exp.size();

        // ---- Pass 1: build flat token list ----
        while (pos < n) {
            Token t;
            t.beg  = pos;
            t.pair = -1;
            uint8_t lead = (uint8_t)exp[pos];

            if (lead >= 0x01 && lead <= 0x06) {
                // Multi-byte enriched token
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
                // Ordinary ASCII character
                t.type = TokType::ASCII;
                t.end  = pos + 1;
            }
            toks.push_back(t);
            pos = t.end;
        }

        // ---- Pass 2: match BLOCKL ↔ BLOCKR ----
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
     * @brief  Converts key presses into an internal expression string
     *         with cursor management, replicating Casio calculator logic.
     *
     * @details
     * The expression is stored in an enriched bytecode (see token table
     * above).  The cursor position @c cp is a byte offset that is always
     * aligned to a logical character boundary.
     *
     * <b>Boxed function insertion with operand absorption:</b>
     * When inserting a multi-block boxed function (root, permut, combin,
     * diff, indefint, sum, prod, defint), the operand before the cursor
     * is absorbed into the first block and the operand after the cursor
     * into the second block.  For 4-block functions blocks 3 and 4 are
     * left empty.  After insertion the cursor lands in the block that
     * received no content (so the user can fill it), or in the first
     * block if both were filled.
     *
     * Single-block functions (sqrt, abs) and the @c log function are
     * excluded from absorption — they always insert empty blocks.
     *
     * <b>Boxed function DEL rules</b> (Casio-style):
     *   - Cursor after BLOCKR → jump into the block (end of content).
     *     log: jumps to the *other* block (visual layout: log_b(a)
     *     shows block 2 as the base on the left, block 1 as the
     *     argument on the right).
     *   - Cursor at block start (immediately after BLOCKL):
     *     • Single-block (sqrt, abs) → delete function, keep content.
     *     • Multi-block, block with content:
     *       - log:       block 1 → jump to block 2;
     *                    block 2 → delete function, keep block 1 only.
     *       - root etc.: block 1 → delete function, keep all content;
     *                    block 2 → jump to block 1.
     *     • Multi-block, empty block → delete function, keep other
     *       blocks' content (log keeps only block 1).
     *   - Cursor inside block content → ordinary backspace.
     *
     * <b>Vertical movement (Y+/Y-)</b> is only active inside multi-block
     * boxed functions.  Layout:
     *   • 2-block: blocks[1] upper, blocks[0] lower.
     *   • 4-block (sum/prod/defint): blocks[3] upper, blocks[2] lower,
     *     blocks[0][1] middle.
     *   • diff/indefint are horizontal — no vertical movement.
     *
     * <b>Undo/Redo</b> saves the complete state (expression, cursor,
     * modifiers) before each mutating operation.
     */
    class Expbuild {
        public:
            std::string exp; ///< Internal expression string (enriched bytecode).
            uint8_t     flg; ///< Modifier / status flags.
            int16_t     cp;  ///< Cursor position (byte offset into exp).

            Expbuild() : flg(0), cp(0) {}

            /**
             * @brief  Process a single key press from the physical keypad.
             * @param  r Row index (0 = top row, 5 = bottom row).
             * @param  c Column index (0 = leftmost, 5 = rightmost).
             * @return Status code (B_SUCCESS, B_EXEC, B_VECDEF, …).
             */
            uint8_t press(uint8_t r, uint8_t c) {
                return insert(getKey(r, c, flg & 0x7));
            }

            /**
             * @brief  Process a key string (from getKey() or programmatic input).
             * @param  k Key string as returned by the key matrix lookup.
             * @return Status code.
             *
             * @details Dispatches the key to the appropriate handler:
             *          modifier toggle, undo/redo, navigation, deletion,
             *          execution, menu invocation, or character insertion.
             *          State-changing operations are preceded by an undo
             *          snapshot save.
             */
            uint8_t insert(std::string k) {
                if (k.empty()) return B_SUCCESS;

                // ----- Modifier keys (toggle internal flags) ------
                if (k == Ctrl::SHIFT) { flg ^= M_SHIFT; return B_SUCCESS; }
                if (k == Ctrl::ALPHA) { flg ^= M_ALPHA; return B_SUCCESS; }
                if (k == Ctrl::CTRL)  { flg ^= M_CTRL; flg &= ~M_LOCK; return B_SUCCESS; }
                if (k == Ctrl::LOCK)  { flg ^= M_LOCK; return B_SUCCESS; }
                if (k == Ctrl::INS)   { flg ^= M_INSERT; flg &= ~M_SHIFT; return B_SUCCESS; }
                if (k == Ctrl::RCL)   { flg ^= M_RCL; _rel(); return B_SUCCESS; }

                // ----- Undo / Redo (full state snapshot) ------------------------
                if (k == Ctrl::UNDO) { _rel(); return _undo(); }
                if (k == Ctrl::REDO) { _rel(); return _redo(); }

                // ----- Global controls ------------------------------------------
                if (k == Ctrl::ON)  return B_RESET;
                if (k == Ctrl::OFF) return B_OFF;
                if (k == Ctrl::AC)  { _saveState(); exp.clear(); cp = 0; flg = 0; return B_SUCCESS; }

                // ----- Navigation (no state change, no undo) --------------------
                if (k == Ctrl::X_PLUS)  { _move( 1); _rel(); return B_SUCCESS; }
                if (k == Ctrl::X_MINUS) { _move(-1); _rel(); return B_SUCCESS; }
                if (k == Ctrl::Y_PLUS || k == Ctrl::Y_MINUS) {
                    if (exp.empty()) return (k == Ctrl::Y_PLUS) ? B_HISTUP : B_HISTDN;
                    _vert(k == Ctrl::Y_PLUS);
                    _rel();
                    return B_SUCCESS;
                }

                // ----- Deletion (state-changing → save undo) --------------------
                if (k == Ctrl::DEL) { _saveState(); return _del(); }

                // ----- Execution ------------------------------------------------
                if (k == Ctrl::EXE || k == Ctrl::OK) { _rel(); return B_EXEC; }

                // ----- Menu / system keys (no undo needed) ----------------------
                if (k == Ctrl::MENU)    { _rel(); return B_MENU; }
                if (k == Ctrl::MODE)    { _rel(); return B_MODE; }
                if (k == Ctrl::SET)     { _rel(); return B_SET; }
                if (k == Ctrl::CLR)     { _rel(); return B_CLEAR; }
                if (k == Ctrl::ABOUT)   { _rel(); return B_ABOUT; }
                if (k == Ctrl::FMT)     { _rel(); return B_FMT; }
                if (k == Ctrl::OPTN)    { _rel(); return B_OPTN; }
                if (k == Ctrl::CONV)    { _rel(); return B_CONV; }
                if (k == Ctrl::CONST)   { _rel(); return B_CONST; }
                if (k == Ctrl::SOLV)    { _rel(); return B_SOLVE; }
                if (k == Ctrl::CALC)    { _rel(); return B_CALC; }
                if (k == Ctrl::FACTOR)  { _rel(); return B_FACTOR; }
                if (k == Ctrl::EXPAND)  { _rel(); return B_EXPAND; }

                // ----- Insertions (state-changing → save undo) ------------------
                _saveState();

                // STO (store arrow, enters expression as \x03\x22)
                if (k == Ctrl::STO) { _ins(k); return B_SUCCESS; }

                // Unknown \x03 control — should not happen in normal use
                if (k.size() >= 1 && (uint8_t)k[0] == 0x03) return B_ERROR;

                // GUI-edited objects (deferred to external editor)
                if (k == CAS::FuncName::vector)  { _rel(); return B_VECDEF; }
                if (k == CAS::FuncName::matrix)  { _rel(); return B_MATDEF; }
                if (k == CAS::FuncName::plot2d)  { _rel(); return B_PLOT2D; }
                if (k == CAS::FuncName::plot3d)  { _rel(); return B_PLOT3D; }

                // Spec symbol → ASCII substitution
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

                // ----- Boxed functions ------------------------------------------
                // 1-block (no absorption — always empty box)
                if (k == CAS::FuncName::abs || k == CAS::FuncName::sqrt)
                    return _boxedFuncAbsorb(k, 1);

                // 2-block with operand absorption (root, permut, combin, diff, indefint)
                if (k == CAS::FuncName::root || k == CAS::FuncName::permut ||
                    k == CAS::FuncName::combin || k == CAS::FuncName::diff ||
                    k == CAS::FuncName::indefint)
                    return _boxedFuncAbsorb(k, 2);

                // log: 2-block, no absorption
                if (k == CAS::FuncName::log)
                    return _boxedFunc(k, 2);

                // 4-block with absorption (sum, prod, defint)
                if (k == CAS::FuncName::sum || k == CAS::FuncName::prod ||
                    k == CAS::FuncName::defint)
                    return _boxedFuncAbsorb(k, 4);

                // ----- Parenthesised functions (\x01xx( as an atomic unit) ------
                if (k.size() == 3 && (uint8_t)k[0] == 0x01) {
                    if (k == CAS::FuncName::deg || k == CAS::FuncName::rad) {
                        _ins(k);            // no parenthesis
                    } else {
                        _ins(k + "(");      // \x01xx( — atomic unit
                    }
                    return B_SUCCESS;
                }

                // ----- Everything else ------------------------------------------
                // Digits, letters, operators (including ^), ',', ';', '=', space,
                // \x02 constants, \x04 conversions, \x05 SI prefixes,
                // \x06 special variables.
                _ins(k);
                return B_SUCCESS;
            }

        private:
            // =============================================================
            // Undo / Redo (full state snapshot)
            // =============================================================

            /** @brief A complete state snapshot for undo/redo. */
            struct Snapshot {
                std::string exp;  ///< Expression string
                int16_t     cp;   ///< Cursor position
            };

            std::vector<Snapshot> _undoStack; ///< Stack of previous states
            std::vector<Snapshot> _redoStack; ///< Stack of undone states
            static const int MAX_UNDO = 100;  ///< Maximum undo history depth

            /**
             * @brief  Save current state to the undo stack and clear redo.
             * @details Called before every mutating operation (insert, delete,
             *          clear).  Navigation and modifier keys do not trigger
             *          a save.
             */
            void _saveState() {
                if (_undoStack.size() >= MAX_UNDO)
                    _undoStack.erase(_undoStack.begin());
                _undoStack.push_back({exp, cp});
                _redoStack.clear();
            }

            /**
             * @brief  Restore the most recent undo state.
             * @return B_SUCCESS, or B_ERROR if the undo stack is empty.
             */
            uint8_t _undo() {
                if (_undoStack.empty()) return B_ERROR;
                _redoStack.push_back({exp, cp});
                Snapshot &s = _undoStack.back();
                exp = s.exp; cp = s.cp; flg = 0;
                _undoStack.pop_back();
                return B_SUCCESS;
            }

            /**
             * @brief  Restore the most recent redo state.
             * @return B_SUCCESS, or B_ERROR if the redo stack is empty.
             */
            uint8_t _redo() {
                if (_redoStack.empty()) return B_ERROR;
                _undoStack.push_back({exp, cp});
                Snapshot &s = _redoStack.back();
                exp = s.exp; cp = s.cp;
                _redoStack.pop_back();
                return B_SUCCESS;
            }

            // =============================================================
            // Token helpers
            // =============================================================

            /** @brief Tokenise the current expression string. */
            std::vector<Token> _tok() const { return _tokenize(exp); }

            /**
             * @brief  Find the token index that contains byte offset @p pos.
             * @param  toks Token list.
             * @param  pos  Byte offset into exp.
             * @return      Token index, or toks.size() if pos is at the end.
             */
            int _tokIdx(const std::vector<Token> &toks, int16_t pos) const {
                if (pos >= (int16_t)exp.size()) return (int)toks.size();
                for (int i = 0; i < (int)toks.size(); i++)
                    if (pos < toks[i].end) return i;
                return (int)toks.size();
            }

            /**
             * @brief  Check if the token at @p ti begins a boxed structure.
             * @details A boxed prefix is a FUNC token or an ASCII operator
             *          that is immediately followed by a BLOCKL token.
             */
            bool _isBoxedPrefix(const std::vector<Token> &toks, int ti) const {
                int n = (int)toks.size();
                if (ti + 1 >= n) return false;
                if (toks[ti + 1].type != TokType::CTRL) return false;
                if (exp[toks[ti + 1].beg + 1] != Ctrl::BLOCKL[1]) return false;
                return (toks[ti].type == TokType::FUNC ||
                        toks[ti].type == TokType::ASCII);
            }

            /**
             * @brief  Locate the first (leftmost) BLOCKL of the boxed
             *         structure that contains the BLOCKL at @p blkTi.
             * @param  toks  Token list.
             * @param  blkTi Token index of any BLOCKL in the structure.
             * @return       Token index of the first BLOCKL.
             *
             * @details Walks left from @p blkTi, tracking nesting depth
             *          to find the outermost BLOCKL belonging to the same
             *          boxed function or operator.
             */
            int _firstBlockL(const std::vector<Token> &toks, int blkTi) const {
                int firstLi = blkTi, depth = 0;
                for (int i = blkTi - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL) {
                        if (exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                            depth++;
                        } else if (exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                            if (depth == 0) { firstLi = i; break; }
                            else { depth--; if (depth == 0) firstLi = i; }
                        }
                    } else {
                        // Non-CTRL token: only break if at depth 0
                        if (depth == 0) break;
                    }
                }
                return firstLi;
            }

            /**
             * @brief  Find the prefix token of the boxed structure containing
             *         the BLOCKL at @p blkTi.
             * @param  toks  Token list.
             * @param  blkTi Token index of any BLOCKL in the structure.
             * @return       Token index of the FUNC or ASCII prefix, or -1
             *               if the structure is malformed.
             */
            int _boxedPrefix(const std::vector<Token> &toks, int blkTi) const {
                int firstLi = blkTi, depth = 0;
                for (int i = blkTi - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL) {
                        if (exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                            depth++;
                        } else if (exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                            if (depth == 0) { firstLi = i; break; }
                            else { depth--; if (depth == 0) firstLi = i; }
                        }
                    } else {
                        if (depth == 0) break;
                    }
                }
                if (firstLi <= 0) return -1;
                if (toks[firstLi - 1].type == TokType::FUNC ||
                    (toks[firstLi - 1].type == TokType::ASCII &&
                        exp[toks[firstLi - 1].beg] == '^')) return firstLi - 1;
                return -1;
            }

            /**
             * @brief  Get the prefix string (function name or operator)
             *         at token index @p ti.
             */
            std::string _boxedNameAt(const std::vector<Token> &toks, int ti) const {
                return exp.substr(toks[ti].beg, toks[ti].width());
            }

            /**
             * @brief  Collect all top-level BLOCKL indices and the last BLOCKR
             *         of the boxed structure starting at @p firstLi.
             * @param[in]  toks    Token list.
             * @param[in]  firstLi Token index of the first BLOCKL.
             * @param[out] blks    Filled with BLOCKL token indices (ascending).
             * @param[out] lastRi  Set to the index of the outermost BLOCKR,
             *                     or -1 if not found.
             */
            void _collectBlocks(const std::vector<Token> &toks, int firstLi,
                                std::vector<int> &blks, int &lastRi) const {
                int depth = 0;
                lastRi = -1;
                for (int i = firstLi; i < (int)toks.size(); i++) {
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
            }

            /**
             * @brief  Check whether the boxed structure has exactly one block.
             * @param  toks Token list.
             * @param  li   Token index of any BLOCKL in the structure.
             * @return      @c true if the function has a single input block.
             */
            bool _isSingleBlock(const std::vector<Token> &toks, int li) const {
                int firstLi = _firstBlockL(toks, li);
                int depth = 0, count = 0;
                for (int i = firstLi; i < (int)toks.size(); i++) {
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

            /**
             * @brief  Get the 0-based index of a BLOCKL within its structure.
             * @param  toks Token list.
             * @param  li   Token index of the BLOCKL.
             * @return      0 for the first block, 1 for the second, etc.,
             *              or -1 if not found.
             */
            int _blockIndex(const std::vector<Token> &toks, int li) const {
                int firstLi = _firstBlockL(toks, li);
                std::vector<int> blks; int lastRi;
                _collectBlocks(toks, firstLi, blks, lastRi);
                for (int b = 0; b < (int)blks.size(); b++)
                    if (blks[b] == li) return b;
                return -1;
            }

            /**
             * @brief  Find the previous sibling BLOCKR before a BLOCKL,
             *         correctly skipping nested boxes.
             * @param  toks Token list.
             * @param  li   Token index of the current BLOCKL.
             * @return      Token index of the previous BLOCKR in the same
             *              boxed structure, or -1 if this is the first block.
             */
            int _prevSiblingBlockR(const std::vector<Token> &toks, int li) const {
                int depth = 0;
                for (int i = li - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL) {
                        if (exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                            if (depth == 0) return i;
                            depth++;
                        } else if (exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                            depth--;
                        }
                    }
                }
                return -1;
            }

            /**
             * @brief  Find the next sibling BLOCKL after a BLOCKR,
             *         correctly skipping nested boxes.
             * @param  toks Token list.
             * @param  ri   Token index of the current BLOCKR.
             * @return      Token index of the next BLOCKL in the same
             *              boxed structure, or -1 if this is the last block.
             */
            int _nextSiblingBlockL(const std::vector<Token> &toks, int ri) const {
                int depth = 0;
                for (int i = ri + 1; i < (int)toks.size(); i++) {
                    if (toks[i].type == TokType::CTRL) {
                        if (exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                            if (depth == 0) return i;
                            depth++;
                        } else if (exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) {
                            depth--;
                        }
                    }
                }
                return -1;
            }

            /**
             * @brief  Get the byte offset just past the outermost BLOCKR
             *         of the boxed structure starting at prefix @p ti.
             */
            int16_t _boxedEnd(const std::vector<Token> &toks, int ti) const {
                int n = (int)toks.size();
                for (int i = ti + 1; i < n; i++)
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKR[1] &&
                        toks[i].pair <= ti + 1) return toks[i].end;
                return toks[ti].end;
            }

            /**
             * @brief  Jump the cursor to the *other* block's content end
             *         within a 2-block boxed structure.
             * @param  toks Token list.
             * @param  li   Token index of the current BLOCKL.
             * @return      B_SUCCESS, or B_ERROR if the structure is not
             *              exactly 2 blocks.
             *
             * @details For log, this accounts for the visual layout where
             *          block 2 is displayed as the base (left side of the
             *          rendered log_b(a) notation) and block 1 as the
             *          argument (right side).  The "other" block means the
             *          sibling block in the same function.
             */
            uint8_t _jumpToOtherBlockEnd(const std::vector<Token> &toks, int li) {
                int firstLi = _firstBlockL(toks, li);
                std::vector<int> blks; int lastRi;
                _collectBlocks(toks, firstLi, blks, lastRi);
                if (blks.size() != 2) return B_ERROR;
                int otherLi = (blks[0] == li) ? blks[1] : blks[0];
                int otherRi = toks[otherLi].pair;
                cp = (toks[otherLi].end == toks[otherRi].beg)
                    ? toks[otherLi].end   // empty → after BLOCKL
                    : toks[otherRi].beg;  // end of content
                return B_SUCCESS;
            }

            // =============================================================
            // Insert helpers
            // =============================================================

            /**
             * @brief  Raw string insertion at cursor (no modifier release).
             * @param  s String to insert.
             * @details Always inserts — never overwrites.  The M_INSERT flag
             *          only affects boxed function operand absorption, not
             *          ordinary character input.
             */
            void _raw(const std::string &s) {
                if (s.empty()) return;
                exp.insert(cp, s);
                cp += (int16_t)s.size();
            }

            /**
             * @brief  Insert string and release one-shot modifiers.
             * @param  s String to insert.
             */
            void _ins(const std::string &s) { _raw(s); _rel(); }

            /**
             * @brief  Insert a boxed function without operand absorption.
             * @param  fn 3-byte function name (\x01xx).
             * @param  n  Number of input blocks (1, 2, or 4).
             * @return    B_SUCCESS.
             *
             * @details Used for sqrt, abs, log, and the ^ operator.
             *          All blocks are inserted empty; the cursor lands
             *          inside the first block.
             */
            uint8_t _boxedFunc(const std::string &fn, int n) {
                int16_t insPos = cp;
                std::string s = fn;
                for (int i = 0; i < n; i++) { s += Ctrl::BLOCKL; s += Ctrl::BLOCKR; }
                _raw(s);
                cp = insPos + (int16_t)fn.size() + BLKLEN;
                _rel();
                return B_SUCCESS;
            }

            /**
             * @brief  Insert a boxed function with operand absorption.
             * @param  fn 3-byte function name (\x01xx).
             * @param  n  Number of input blocks (2 or 4).
             * @return    B_SUCCESS.
             *
             * @details The operand immediately before the cursor is absorbed
             *          into the first block; the operand immediately after
             *          the cursor is absorbed into the second block.
             *          Blocks beyond the second are left empty.
             *
             *          After insertion the cursor is placed in the block
             *          that needs filling: block 2 if the after-operand was
             *          empty, otherwise block 1.
             *
             *          An operand is one complete token — a number,
             *          identifier, parenthesised expression, function call,
             *          or multi-byte constant/variable.
             */
            uint8_t _boxedFuncAbsorb(const std::string &fn, int n) {
                bool doInsert = (flg & M_INSERT);
                int16_t beforeStart = cp, afterEnd = cp;
                std::string before, after;

                if (n == 2 && fn != CAS::FuncName::log) {
                    beforeStart = _scanOpBack(cp);
                    afterEnd    = _scanOp(cp);
                    before = (beforeStart < cp) ? exp.substr(beforeStart, cp - beforeStart) : "";
                    after  = (afterEnd > cp)    ? exp.substr(cp, afterEnd - cp)       : "";
                } else if (doInsert) {
                    afterEnd = _scanOp(cp);
                    after = (afterEnd > cp) ? exp.substr(cp, afterEnd - cp) : "";
                    if (after.empty()) { flg &= ~M_INSERT; doInsert = false; }
                }

                if (afterEnd > cp)       exp.erase(cp, afterEnd - cp);
                if (beforeStart < cp)    { exp.erase(beforeStart, cp - beforeStart); cp = beforeStart; }

                int16_t insPos = cp;
                std::string s = fn;
                if (n == 1) {
                    s += Ctrl::BLOCKL; s += after; s += Ctrl::BLOCKR;
                } else {
                    s += Ctrl::BLOCKL; s += before; s += Ctrl::BLOCKR;
                    s += Ctrl::BLOCKL; s += after;  s += Ctrl::BLOCKR;
                    for (int i = 2; i < n; i++) { s += Ctrl::BLOCKL; s += Ctrl::BLOCKR; }
                }
                _raw(s);
                cp = insPos + (int16_t)fn.size() + BLKLEN;

                if (doInsert) flg &= ~M_INSERT;
                _rel();
                return B_SUCCESS;
            }

            /**
             * @brief  Scan forward from @p pos to find the end of one operand.
             * @param  pos Starting byte offset.
             * @return     Offset just past the operand.
             *
             * @details Recognises parenthesised groups, boxed functions,
             *          numbers, identifiers (e.g. "hello", "h2"), and
             *          multi-byte tokens (\x02…\x06).
             */
            int16_t _scanOp(int16_t pos) const {
                if (pos >= (int16_t)exp.size()) return pos;
                uint8_t lead = (uint8_t)exp[pos];

                // Parenthesised expression: ( … )
                if (lead == '(') {
                    int d = 1; int16_t i = pos + 1;
                    while (i < (int16_t)exp.size() && d > 0) {
                        if (exp[i] == '(') d++; else if (exp[i] == ')') d--;
                        i++;
                    }
                    return i;
                }

                // Function: \x01xx followed by blocks or '('
                if (lead == 0x01 && pos + 2 < (int16_t)exp.size()) {
                    int16_t i = pos + 3;
                    // Boxed function (\x01xx\x03\x20…)
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
                    // Parenthesised function (\x01xx(…)
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

                // Single-byte character (operator, comma, etc.)
                return pos + 1;
            }

            /**
             * @brief  Scan backward from @p pos to find the start of one operand.
             * @param  pos Ending byte offset (cursor position).
             * @return     Offset of the operand's first byte.
             *
             * @details Symmetric to _scanOp().  Recognises parenthesised groups,
             *          boxed functions, numbers, identifiers, and multi-byte
             *          tokens by scanning in reverse from @p pos.
             */
            int16_t _scanOpBack(int16_t pos) const {
                if (pos <= 0) return 0;
                int16_t i = pos - 1;
                uint8_t lead = (uint8_t)exp[i];

                // Multi-byte token (\x02–\x06, 2 bytes)
                if (lead >= 0x02 && lead <= 0x06) return i;

                // \x01 function — find its start
                if (i >= 2 && (uint8_t)exp[i - 2] == 0x01) {
                    // Check if this function is followed by '('
                    int16_t funcStart = i - 2;
                    if (funcStart + 3 < (int16_t)exp.size() && exp[funcStart + 3] == '(') {
                        // \x01xx( — find the matching ')'
                        int d = 1; i = funcStart + 4;
                        while (i < (int16_t)exp.size() && d > 0) {
                            if (exp[i] == '(') d++; else if (exp[i] == ')') d--;
                            i++;
                        }
                        return funcStart; // return \x01 position
                    }
                    // Standalone \x01 function without '('
                    return funcStart;
                }
                if (i >= 1 && (uint8_t)exp[i - 1] == 0x01) return i - 1;
                if ((uint8_t)exp[i] == 0x01) return i;

                // Closing parenthesis ')' → find matching '('
                if (lead == ')') {
                    int d = 1; i--;
                    while (i >= 0 && d > 0) {
                        if (exp[i] == ')') d++;
                        else if (exp[i] == '(') d--;
                        if (d > 0) i--;
                    }
                    // Now i is at '('. Check if preceded by \x01 function
                    if (i >= 3 && (uint8_t)exp[i - 3] == 0x01) {
                        return i - 3; // include the \x01 function
                    }
                    return i;
                }

                // Number or identifier
                if ((lead >= '0' && lead <= '9') || lead == '.' ||
                    (lead >= 'A' && lead <= 'Z') || (lead >= 'a' && lead <= 'z')) {
                    while (i > 0 &&
                        ((exp[i - 1] >= '0' && exp[i - 1] <= '9') ||
                            exp[i - 1] == '.' ||
                            (exp[i - 1] >= 'A' && exp[i - 1] <= 'Z') ||
                            (exp[i - 1] >= 'a' && exp[i - 1] <= 'z'))) i--;
                    return i;
                }

                return i;
            }

            // =============================================================
            // Modifier state machine
            // =============================================================

            /**
             * @brief  Release one-shot modifiers after a printable key.
             *
             * @details SHIFT and ALPHA latch for one key press unless held
             *          together with CTRL.  CTRL latches for one press
             *          unless ALPHA is also active or CTRL-LOCK is on.
             *          LOCK is cleared automatically if CTRL is released.
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
                if ((flg & M_LOCK) && ((flg & 0x07) != M_CTRL))
                    flg &= ~M_LOCK;
            }

            // =============================================================
            // Cursor movement
            // =============================================================

            /**
             * @brief  Move cursor by delta tokens (+1 right, -1 left).
             *
             * @details Uses tokenisation so the cursor always lands on a valid
             *          token boundary.
             *
             * Right-move special cases:
             *   - \x01xx( → skip function + '(' as a 4-byte unit.
             *   - Boxed prefix → enter first block (vector/matrix skip whole;
             *     log enters block 2, i.e. the base, which is the leftmost
             *     block visually).
             *   - BLOCKR → next sibling block, or exit boxed structure.
             *     log: BLOCKR of block 2 (base) → enter block 1 (argument);
             *          BLOCKR of block 1 → exit (same as normal).
             *
             * Left-move special cases:
             *   - '(' bound to \x01 → skip function name too.
             *   - BLOCKR → enter the block.
             *   - BLOCKL → previous sibling block's end, or before prefix.
             *     log: BLOCKL of block 1 (argument) → enter block 2 (base);
             *          BLOCKL of block 2 → before prefix (same as normal).
             */
            void _move(int delta) {
                auto toks = _tok();
                int ti = _tokIdx(toks, cp);
                int n  = (int)toks.size();

                if (delta > 0) {
                    // ---- Right ----
                    if (ti >= n) return;
                    const Token &t = toks[ti];

                    // \x01xx( → skip func + '(' as a unit
                    if (t.type == TokType::FUNC && ti + 1 < n &&
                        toks[ti + 1].type == TokType::ASCII &&
                        exp[toks[ti + 1].beg] == '(') {
                        cp = toks[ti + 1].end;
                    }
                    // Boxed structure prefix → enter first visible block
                    else if (_isBoxedPrefix(toks, ti)) {
                        std::string nm = _boxedNameAt(toks, ti);
                        if (nm == CAS::FuncName::vector ||
                            nm == CAS::FuncName::matrix) {
                            cp = _boxedEnd(toks, ti);
                        } else if (nm == CAS::FuncName::log) {
                            std::vector<int> blks; int lastRi;
                            _collectBlocks(toks, ti + 1, blks, lastRi);
                            if (blks.size() >= 2)
                                cp = toks[blks[1]].end; // enter block 2
                            else
                                cp = toks[ti + 1].end;
                        } else {
                            cp = toks[ti + 1].end;      // enter block 1
                        }
                    }
                    // BLOCKR → next visual block, or exit
                    else if (t.type == TokType::CTRL &&
                             exp[t.beg + 1] == Ctrl::BLOCKR[1]) {
                        int li = t.pair;
                        int pi = (li >= 0) ? _boxedPrefix(toks, li) : -1;
                        std::string nm = (pi >= 0) ? _boxedNameAt(toks, pi) : "";
                        if (nm == CAS::FuncName::log) {
                            int idx = _blockIndex(toks, li);
                            if (idx == 1) {
                                std::vector<int> blks; int lastRi;
                                _collectBlocks(toks, _firstBlockL(toks, li), blks, lastRi);
                                if (blks.size() >= 2)
                                    cp = toks[blks[0]].end;
                                else
                                    cp = t.end;
                            } else {
                                std::vector<int> blks; int lastRi;
                                _collectBlocks(toks, _firstBlockL(toks, li), blks, lastRi);
                                cp = toks[lastRi].end;
                            }
                        } else {
                            int nextLi = _nextSiblingBlockL(toks, ti);
                            if (nextLi >= 0) cp = toks[nextLi].end;
                            else cp = t.end;
                        }
                    }
                    else {
                        cp = t.end;
                    }
                } else {
                    // ---- Left ----
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
                            int pi = _boxedPrefix(toks, li);
                            std::string nm = (pi >= 0) ? _boxedNameAt(toks, pi) : "";
                            if (nm == CAS::FuncName::log) {
                                _jumpToOtherBlockEnd(toks, li);
                            } else {
                                cp = (toks[li].end == toks[ti - 1].beg)
                                     ? toks[li].end
                                     : toks[ti - 1].beg;
                            }
                        } else {
                            cp = t.beg;
                        }
                    }
                    // BLOCKL → previous visual block, or before prefix
                    else if (t.type == TokType::CTRL &&
                             exp[t.beg + 1] == Ctrl::BLOCKL[1]) {
                        int pi = _boxedPrefix(toks, ti - 1);
                        std::string nm = (pi >= 0) ? _boxedNameAt(toks, pi) : "";
                        if (nm == CAS::FuncName::log) {
                            int idx = _blockIndex(toks, ti - 1);
                            if (idx == 0) {
                                std::vector<int> blks; int lastRi;
                                _collectBlocks(toks, _firstBlockL(toks, ti - 1), blks, lastRi);
                                if (blks.size() >= 2) {
                                    int otherRi = toks[blks[1]].pair;
                                    cp = (toks[blks[1]].end == toks[otherRi].beg)
                                         ? toks[blks[1]].end
                                         : toks[otherRi].beg;
                                }
                            } else {
                                // BLOCKL of block 2 (base) in log: jump to end of entire log
                                std::vector<int> blks; int lastRi;
                                _collectBlocks(toks, _firstBlockL(toks, ti - 1), blks, lastRi);
                                cp = toks[lastRi].end;
                            }
                        } else {
                            int prevRi = _prevSiblingBlockR(toks, ti - 1);
                            if (prevRi >= 0) {
                                int prevLi = toks[prevRi].pair;
                                cp = (prevLi >= 0 &&
                                      toks[prevLi].end == toks[prevRi].beg)
                                     ? toks[prevLi].end
                                     : toks[prevRi].beg;
                            } else {
                                // Nested inside an outer box?
                                int depth = 0;
                                int outerL = -1;
                                for (int j = pi - 1; j >= 0; j--) {
                                    if (toks[j].type == TokType::CTRL) {
                                        if (exp[toks[j].beg + 1] == Ctrl::BLOCKR[1]) depth++;
                                        else if (exp[toks[j].beg + 1] == Ctrl::BLOCKL[1]) {
                                            if (depth == 0) { outerL = j; break; }
                                            else depth--;
                                        }
                                    }
                                }
                                if (outerL >= 0) {
                                    cp = toks[outerL].end;
                                } else {
                                    cp = (pi >= 0) ? toks[pi].beg : t.beg;
                                }
                            }
                        }
                    }
                    else {
                        cp = t.beg;
                    }
                }
            }

            /**
             * @brief  Vertical cursor movement (Y+/Y- keys).
             * @param  up @c true for upward, @c false for downward.
             *
             * @details Only effective inside boxed functions with multiple
             *          input blocks that are stacked vertically.
             *
             * <b>2-block layout</b> (blocks[1] upper, blocks[0] lower):
             *   • log [a][b] = log_b(a)
             *   • root [n][x]
             *   • permut / combin [n][k]
             *
             * <b>4-block layout</b> (sum / prod / defint):
             *   • Upper:  [to]       (blocks[3])
             *   • Middle: [exp][var]  (blocks[0], blocks[1])
             *   • Lower:  [from]     (blocks[2])
             *
             * @note diff / indefint are 2-block but arranged horizontally
             *       (exp left, var right) — vertical moves are ignored.
             */
            void _vert(bool up) {
                auto toks = _tok();
                int ti = _tokIdx(toks, cp);

                // Locate the enclosing BLOCKL
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
                std::string nm = _boxedNameAt(toks, pi);

                // diff / indefint are horizontal — no vertical movement
                if (nm == CAS::FuncName::diff || nm == CAS::FuncName::indefint)
                    return;

                int firstLi = _firstBlockL(toks, blkTi);
                std::vector<int> blks; int lastRi;
                _collectBlocks(toks, firstLi, blks, lastRi);
                int nb = (int)blks.size();
                if (nb <= 1) return;

                // Find current block index
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

            /**
             * @brief  Jump cursor to a specific block inside the enclosing
             *         boxed function.
             * @param  idx Zero-based block index (0 = first block).
             */
            void _jumpBlk(int idx) {
                auto toks = _tok();
                int ti = _tokIdx(toks, cp);
                for (int i = ti - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        int firstLi = _firstBlockL(toks, i);
                        std::vector<int> blks; int lastRi;
                        _collectBlocks(toks, firstLi, blks, lastRi);
                        if (idx >= 0 && idx < (int)blks.size())
                            { cp = toks[blks[idx]].end; return; }
                    }
                }
            }

            // =============================================================
            // Deletion
            // =============================================================

            /**
             * @brief  Handle the DEL (backspace) key.
             * @return B_SUCCESS, B_VECEDT, B_MATEDT, or B_ERROR.
             *
             * @details Deletion is context-sensitive based on cursor position
             *          relative to boxed function blocks.  See the class
             *          documentation for the complete rule table.
             */
            uint8_t _del() {
                if (exp.empty() || cp <= 0) return B_ERROR;

                auto toks = _tok();
                int ti = _tokIdx(toks, cp);

                // --- Case 1: cursor right after a BLOCKR → jump into block ----
                if (ti > 0 && toks[ti - 1].type == TokType::CTRL &&
                    exp[toks[ti - 1].beg + 1] == Ctrl::BLOCKR[1]) {
                    int ri = ti - 1;
                    int li = toks[ri].pair;
                    if (li < 0) return B_ERROR;
                    int pi = _boxedPrefix(toks, li);
                    if (pi < 0) return B_ERROR;
                    std::string nm = _boxedNameAt(toks, pi);

                    // vector/matrix → invoke external editor
                    if (nm == CAS::FuncName::vector) return B_VECEDT;
                    if (nm == CAS::FuncName::matrix) return B_MATEDT;

                    // log: jump to the OTHER block (visual log_b(a) layout)
                    if (nm == CAS::FuncName::log)
                        return _jumpToOtherBlockEnd(toks, li);

                    // All other boxed: jump into this block
                    cp = (toks[li].end == toks[ri].beg)
                        ? toks[li].end   // empty → after BLOCKL
                        : toks[ri].beg;  // end of content
                    return B_SUCCESS;
                }

                // --- Case 2: cursor at or inside a block -----------------------
                for (int i = ti - 1; i >= 0; i--) {
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKL[1]) {
                        int li = i;
                        int ri = toks[li].pair;
                        if (ri < 0) break;

                        // Cursor at very start of a block (cp == BLOCKL.end)
                        if (cp == toks[li].end) {
                            // Single block → delete function, keep content
                            if (_isSingleBlock(toks, li))
                                return _delBoxed(toks, li);

                            // Multi-block
                            int pi = _boxedPrefix(toks, li);
                            std::string nm = (pi >= 0) ? _boxedNameAt(toks, pi) : "";
                            bool logStyle = (nm == CAS::FuncName::log);
                            bool isFirst  = (_blockIndex(toks, li) == 0);

                            if (logStyle) {
                                // log: block 1 (argument) → jump to block 2
                                //      block 2 (base)     → delete, keep block 1
                                if (isFirst)
                                    return _jumpToOtherBlockEnd(toks, li);
                                else
                                    return _delBoxed(toks, li);
                            } else {
                                // root/permut/combin: block 1 → delete all
                                //                     block 2 → jump to block 1
                                if (isFirst)
                                    return _delBoxed(toks, li);
                                else
                                    return _jumpToOtherBlockEnd(toks, li);
                            }
                        }

                        // Inside block with content → ordinary backspace
                        if (cp > toks[li].end && cp <= toks[ri].beg) {
                            if (ti == 0) return B_ERROR;
                            return _delTok(toks, ti - 1);
                        }
                        break;
                    }
                    if (toks[i].type == TokType::CTRL &&
                        exp[toks[i].beg + 1] == Ctrl::BLOCKR[1]) break;
                }

                // --- Case 3: ordinary backspace ---------------------------------
                if (ti == 0) return B_ERROR;
                return _delTok(toks, ti - 1);
            }

            /**
             * @brief  Delete an entire boxed function/operator, keeping
             *         non-deleted block contents.
             * @param  toks  Token list.
             * @param  delLi Token index of any BLOCKL in the structure.
             * @return       B_SUCCESS or B_ERROR.
             *
             * @details For log, only block 0 (argument / lower) content is
             *          kept, matching the visual log_b(a) deletion behaviour.
             *          For all other functions, all block contents are kept.
             *          The cursor is placed at the beginning of the kept
             *          content.
             */
            uint8_t _delBoxed(const std::vector<Token> &toks, int delLi) {
                int pi = _boxedPrefix(toks, delLi);
                if (pi < 0) return B_ERROR;
                std::string nm = _boxedNameAt(toks, pi);
                bool logStyle = (nm == CAS::FuncName::log);

                int firstLi = _firstBlockL(toks, delLi);
                std::vector<int> blks; int lastRi;
                _collectBlocks(toks, firstLi, blks, lastRi);
                if (lastRi < 0) return B_ERROR;

                // Collect content from blocks to keep
                std::string kept;
                for (int b = 0; b < (int)blks.size(); b++) {
                    // log: only keep block 0 (argument / lower)
                    if (logStyle && b != 0) continue;
                    int ri = toks[blks[b]].pair;
                    if (ri >= 0 && toks[blks[b]].end < toks[ri].beg) {
                        kept += exp.substr(toks[blks[b]].end,
                                        toks[ri].beg - toks[blks[b]].end);
                    }
                }

                int16_t eraseStart = toks[pi].beg;
                exp.erase(eraseStart, toks[lastRi].end - eraseStart);
                exp.insert(eraseStart, kept);
                // Cursor at the beginning of the kept content
                cp = eraseStart;
                return B_SUCCESS;
            }

            /**
             * @brief  Backspace-delete one token at index @p ti.
             * @param  toks Token list.
             * @param  ti   Token index to delete.
             * @return      B_SUCCESS or B_ERROR.
             *
             * @details If the token is '(' bound to a \x01 function, only
             *          \x01xx( is deleted — the content inside the
             *          parentheses is preserved.
             */
            uint8_t _delTok(const std::vector<Token> &toks, int ti) {
                const Token &t = toks[ti];

                // '(' bound to \x01 function → delete \x01xx( only,
                // leaving the parenthesised content intact.
                if (t.type == TokType::ASCII && exp[t.beg] == '(' &&
                    ti > 0 && toks[ti - 1].type == TokType::FUNC) {
                    exp.erase(toks[ti - 1].beg, t.end - toks[ti - 1].beg);
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