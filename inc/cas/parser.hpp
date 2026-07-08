/** @file /inc/cas/parser.hpp
 *  @brief Expression string to expression tree parser for the CAS module
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

#ifndef _CAS_PARSER_HPP_
#define _CAS_PARSER_HPP_

#include "cas/expdef.hpp"
#include "cas/exptree.hpp"
#include "cas/rational.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace CAS {

    /**
     * @class  Parser
     * @brief  Recursive-descent parser: enriched-bytecode expression → AST.
     *
     * @details
     * Input is from Keypad::Expbuild.  Output AST contains only Rational
     * numbers (decimals → fractions), Variables, and Functions/Operators.
     * All structural tokens (\x03 blocks, parentheses) and SI prefixes
     * (\x05) are eliminated.  Implicit multiplication is made explicit.
     * On error, parse() returns nullptr and getError() provides details.
     *
     * <b>Operator precedence (low to high):</b>
     *   1.  =                                      (right)
     *   2.  STO (\x03\x22)                          (right)
     *   3.  ,                                       (argument separator)
     *   4.  bor  bxor  bnor  bxnor
     *   5.  band  bnand
     *   6.  blshift  brshift
     *   7.  +  -
     *   8.  *  /
     *   8.5 implicit multiplication
     *   9.  -  +  bnot  (unary, right)
     *  10.  ^                                       (right)
     *  11.  fact  estx1  estx2  esty  (postfix)
     *  12.  function calls  (atoms)
     *
     * <b>frac:</b> \x01fr(a,b) → binary /.
     */
    class Parser {
        public:
            /**
             * @brief  Parse an expression string into an AST.
             * @param  exp Expression string from Keypad::Expbuild.
             * @return     Root node, or nullptr on error (see getError()).
             */
            static Exptree* parse(const std::string &exp) {
                Parser p(exp);
                _lastError.clear();
                Exptree* tree = p.parseExpression();
                if (tree && p._pos < (int16_t)p._exp.size()) {
                    p._setError("Unexpected token after expression");
                    delete tree;
                    return nullptr;
                }
                return tree;
            }

            /** @brief Error message from the last parse() call, or empty. */
            static const std::string& getError() { return _lastError; }

        private:
            const std::string &_exp;
            int16_t           _pos;
            static std::string _lastError;  ///< Static error buffer

            Parser(const std::string &exp) : _exp(exp), _pos(0) {}

            void _setError(const std::string &msg) const {
                int16_t start = std::max((int16_t)0, (int16_t)(_pos - 5));
                int16_t len   = std::min((int16_t)10, (int16_t)(_exp.size() - start));
                _lastError = msg + " at pos " + std::to_string(_pos) +
                            " near '" + _exp.substr(start, len) + "'";
            }

            // =============================================================
            // Low-level helpers
            // =============================================================

            uint8_t _peek() const {
                return (_pos < (int16_t)_exp.size()) ? (uint8_t)_exp[_pos] : 0;
            }
            uint8_t _peek(int16_t off) const {
                int16_t idx = _pos + off;
                return (idx >= 0 && idx < (int16_t)_exp.size()) ? (uint8_t)_exp[idx] : 0;
            }
            void _advance(int16_t n = 1) { _pos += n; }

            int16_t _tokWidth(int16_t pos) const {
                if (pos >= (int16_t)_exp.size()) return 0;
                uint8_t lead = (uint8_t)_exp[pos];
                if (lead >= 0x01 && lead <= 0x06) {
                    if (lead == 0x01 && pos + 2 < (int16_t)_exp.size()) return 3;
                    if (pos + 1 < (int16_t)_exp.size()) return 2;
                }
                return 1;
            }
            std::string _tokStr() const {
                int16_t w = _tokWidth(_pos);
                return _exp.substr(_pos, w);
            }

            bool _eof()      const { return _pos >= (int16_t)_exp.size(); }
            bool _isFunc()   const { return _peek() == 0x01; }
            bool _isBlockL() const { return _peek() == 0x03 && _peek(1) == 0x20; }
            bool _isBlockR() const { return _peek() == 0x03 && _peek(1) == 0x21; }
            bool _isSTO()    const { return _peek() == 0x03 && _peek(1) == 0x22; }

            static bool _isDigit(uint8_t c) { return c >= '0' && c <= '9'; }
            static bool _isHexDigit(uint8_t c) {
                return _isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
            }
            static bool _isIdStart(uint8_t c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
            static bool _isIdChar(uint8_t c)  { return _isIdStart(c) || _isDigit(c); }

            bool _canStartAtom(uint8_t c) const {
                return _isDigit(c) || c == '.' || _isIdStart(c) ||
                    c == '(' || (c >= 0x01 && c <= 0x06);
            }
            static bool _canImplicitMul(const Exptree* node) {
                return node &&
                    (node->valtp == Exptree::val_t::valRational ||
                        node->valtp == Exptree::val_t::valVariable ||
                        node->valtp == Exptree::val_t::valFunction);
            }

            // =============================================================
            // Recursive descent
            // =============================================================

            Exptree* parseExpression()      { return parseAssign(); }

            Exptree* parseAssign() {
                Exptree* left = parseComma(); if (!left) return nullptr;
                while (!_eof()) {
                    if (_peek() == '=') {
                        _advance();
                        Exptree* r = parseAssign(); if (!r) { delete left; return nullptr; }
                        left = _makeBinary("=", left, r);
                    } else if (_isSTO()) {
                        _advance(2);
                        Exptree* r = parseAssign(); if (!r) { delete left; return nullptr; }
                        left = _makeBinary("\x03\x22", left, r);
                    } else break;
                }
                return left;
            }

            Exptree* parseComma() {
                Exptree* left = parseBitwiseOr(); if (!left) return nullptr;
                if (_eof() || _peek() != ',') return left;
                Exptree* comma = _makeFunction(",", {left});
                while (!_eof() && _peek() == ',') {
                    _advance();
                    Exptree* arg = parseBitwiseOr(); if (!arg) { delete comma; return nullptr; }
                    comma->child.push_back(arg);
                }
                return comma;
            }

            Exptree* parseBitwiseOr() {
                Exptree* left = parseBitwiseAnd(); if (!left) return nullptr;
                while (_isFunc()) {
                    std::string fn = _tokStr();
                    if (fn != CAS::FuncName::bor  && fn != CAS::FuncName::bxor &&
                        fn != CAS::FuncName::bnor && fn != CAS::FuncName::bxnor) break;
                    _advance(3);
                    Exptree* r = parseBitwiseAnd(); if (!r) { delete left; return nullptr; }
                    left = _makeBinary(fn, left, r);
                }
                return left;
            }

            Exptree* parseBitwiseAnd() {
                Exptree* left = parseShift(); if (!left) return nullptr;
                while (_isFunc()) {
                    std::string fn = _tokStr();
                    if (fn != CAS::FuncName::band && fn != CAS::FuncName::bnand) break;
                    _advance(3);
                    Exptree* r = parseShift(); if (!r) { delete left; return nullptr; }
                    left = _makeBinary(fn, left, r);
                }
                return left;
            }

            Exptree* parseShift() {
                Exptree* left = parseAddSub(); if (!left) return nullptr;
                while (_isFunc()) {
                    std::string fn = _tokStr();
                    if (fn != CAS::FuncName::blshift && fn != CAS::FuncName::brshift) break;
                    _advance(3);
                    Exptree* r = parseAddSub(); if (!r) { delete left; return nullptr; }
                    left = _makeBinary(fn, left, r);
                }
                return left;
            }

            Exptree* parseAddSub() {
                Exptree* left = parseMulDiv(); if (!left) return nullptr;
                while (!_eof()) {
                    uint8_t c = _peek();
                    if (c == '+' || c == '-') {
                        _advance();
                        Exptree* r = parseMulDiv(); if (!r) { delete left; return nullptr; }
                        left = _makeBinary(std::string(1, (char)c), left, r);
                    } else break;
                }
                return left;
            }

            Exptree* parseMulDiv() {
                Exptree* left = parseUnary(); if (!left) return nullptr;
                while (!_eof()) {
                    uint8_t c = _peek();
                    if (c == '*' || c == '/') {
                        _advance();
                        Exptree* r = parseUnary(); if (!r) { delete left; return nullptr; }
                        left = _makeBinary(std::string(1, (char)c), left, r);
                        continue;
                    }
                    if (_canStartAtom(c) && _canImplicitMul(left)) {
                        Exptree* r = parseUnary(); if (!r) { delete left; return nullptr; }
                        left = _makeBinary("*", left, r);
                        continue;
                    }
                    break;
                }
                return left;
            }

            Exptree* parseUnary() {
                uint8_t c = _peek();
                if (c == '-') { _advance(); Exptree* a = parseUnary(); return a ? _makeUnary("-", a) : nullptr; }
                if (c == '+') { _advance(); return parseUnary(); }
                if (_isFunc() && _tokStr() == CAS::FuncName::bnot) {
                    _advance(3);
                    Exptree* a = parseUnary(); return a ? _makeUnary(CAS::FuncName::bnot, a) : nullptr;
                }
                return parsePower();
            }

            Exptree* parsePower() {
                Exptree* left = parsePostfix(); if (!left) return nullptr;
                if (!_eof() && _peek() == '^') {
                    _advance();
                    if (!_eof() && _isBlockL()) return parseBoxedPower(left);
                    Exptree* r = parsePower(); if (!r) { delete left; return nullptr; }
                    return _makeBinary("^", left, r);
                }
                return left;
            }

            Exptree* parseBoxedPower(Exptree* base) {
                if (!_isBlockL()) { _setError("Expected block after ^"); delete base; return nullptr; }
                _advance(BLKLEN);
                Exptree* exp = (!_eof() && !_isBlockR()) ? parseExpression() : nullptr;
                if (!exp) { _setError("Empty exponent in power"); delete base; return nullptr; }
                if (!_isBlockR()) { _setError("Expected \\x03\\x21 after exponent"); delete base; delete exp; return nullptr; }
                _advance(BLKLEN);
                return _makeBinary("^", base, exp);
            }

            Exptree* parsePostfix() {
                Exptree* node = parseAtom(); if (!node) return nullptr;
                while (_isFunc()) {
                    std::string fn = _tokStr();
                    if (fn == CAS::FuncName::fact || fn == CAS::FuncName::estx1 ||
                        fn == CAS::FuncName::estx2 || fn == CAS::FuncName::esty) {
                        _advance(3);
                        node = _makeUnary(fn, node);
                    } else break;
                }
                return node;
            }

            // =============================================================
            // Atoms
            // =============================================================

            Exptree* parseAtom() {
                if (_eof()) { _setError("Unexpected end of expression"); return nullptr; }
                uint8_t c = _peek();
                if (_isDigit(c) || (c == '.' && _isDigit(_peek(1)))) return parseNumber();
                if (c == '0' && (_peek(1) == 'x' || _peek(1) == 'X')) return parseHexNumber();
                if (_isIdStart(c)) return parseVariable();
                if (c >= 0x01 && c <= 0x06) return parseEnrichedToken();
                if (c == '(') return parseParenGroup();
                if (c == ' ') { _advance(); return parseAtom(); }
                _setError(std::string("Unexpected character '") + (char)c + "'");
                return nullptr;
            }

            Exptree* parseNumber() {
                int16_t start = _pos;
                while (!_eof() && (_isDigit(_peek()) || _peek() == '.')) _advance();
                std::string s = _exp.substr(start, _pos - start);
                if (std::count(s.begin(), s.end(), '.') > 1) {
                    _setError("Invalid number with multiple decimal points"); return nullptr;
                }
                return _makeRational(s);
            }

            Exptree* parseHexNumber() {
                _advance(2); int16_t start = _pos;
                while (!_eof() && _isHexDigit(_peek())) _advance();
                if (_pos == start) { _setError("Expected hex digits after 0x"); return nullptr; }
                return _makeRational(_exp.substr(start, _pos - start), 16);
            }

            Exptree* parseParenGroup() {
                _advance();
                Exptree* inner = parseExpression(); if (!inner) return nullptr;
                if (_eof() || _peek() != ')') {
                    _setError("Expected ')'"); delete inner; return nullptr;
                }
                _advance(); return inner;
            }

            Exptree* parseVariable() {
                int16_t start = _pos;
                while (!_eof() && _isIdChar(_peek())) _advance();
                Exptree* node = new Exptree();
                node->valtp = Exptree::val_t::valVariable;
                node->var   = _exp.substr(start, _pos - start);
                return node;
            }

            // =============================================================
            // Enriched tokens
            // =============================================================

            Exptree* parseEnrichedToken() {
                switch (_peek()) {
                    case 0x01: return parseFunction();
                    case 0x02: return parseConstant();
                    case 0x03: _setError("Unexpected control token"); return nullptr;
                    case 0x04: return parseConversion();
                    case 0x05: return parseSIPrefix();
                    case 0x06: return parseSpecialVar();
                    default:   _setError("Unknown enriched token"); return nullptr;
                }
            }

            Exptree* parseFunction() {
                std::string fn = _tokStr(); _advance(3);
                if (fn == CAS::FuncName::frac) {
                    if (_eof() || !_isBlockL()) { _setError("frac requires input blocks"); return nullptr; }
                    return parseFrac();
                }
                if (!_eof() && _isBlockL()) return parseBoxedFunction(fn);
                if (!_eof() && _peek() == '(') return parseParenFunction(fn);
                return _makeFunction(fn, {});
            }

            Exptree* parseFrac() {
                _advance(BLKLEN);
                Exptree* num = (!_eof() && !_isBlockR()) ? parseExpression() : nullptr;
                if (!_isBlockR()) { _setError("Expected \\x03\\x21 after frac numerator"); delete num; return nullptr; }
                _advance(BLKLEN);
                if (!_isBlockL()) { _setError("Expected second block for frac denominator"); delete num; return nullptr; }
                _advance(BLKLEN);
                Exptree* den = (!_eof() && !_isBlockR()) ? parseExpression() : nullptr;
                if (!_isBlockR()) { _setError("Expected \\x03\\x21 after frac denominator"); delete num; delete den; return nullptr; }
                _advance(BLKLEN);
                if (!num) { _setError("Missing numerator in frac"); return nullptr; }
                if (!den) { _setError("Missing denominator in frac"); delete num; return nullptr; }
                return _makeBinary("/", num, den);
            }

            Exptree* parseBoxedFunction(const std::string &fn) {
                std::vector<Exptree*> args;
                while (!_eof() && _isBlockL()) {
                    _advance(BLKLEN);
                    if (!_eof() && !_isBlockR()) {
                        Exptree* arg = parseExpression(); if (!arg) { for (auto* a : args) delete a; return nullptr; }
                        args.push_back(arg);
                    } else args.push_back(nullptr);
                    if (_eof() || !_isBlockR()) {
                        _setError("Expected \\x03\\x21 to close block");
                        for (auto* a : args) delete a; return nullptr;
                    }
                    _advance(BLKLEN);
                }
                if (args.empty()) { _setError("Function has no input blocks"); return nullptr; }
                return _makeFunction(fn, args);
            }

            Exptree* parseParenFunction(const std::string &fn) {
                _advance();
                std::vector<Exptree*> args;
                if (_peek() != ')') {
                    Exptree* a = parseExpression(); if (!a) return nullptr;
                    args.push_back(a);
                    while (!_eof() && _peek() == ',') {
                        _advance();
                        Exptree* a2 = parseExpression(); if (!a2) { for (auto* x : args) delete x; return nullptr; }
                        args.push_back(a2);
                    }
                }
                if (_eof() || _peek() != ')') {
                    _setError("Expected ')' after arguments");
                    for (auto* a : args) delete a; return nullptr;
                }
                _advance(); return _makeFunction(fn, args);
            }

            Exptree* parseConstant() {
                std::string tok = _tokStr(); _advance(2);
                Exptree* node = new Exptree(); node->valtp = Exptree::val_t::valVariable; node->var = tok;
                return node;
            }

            Exptree* parseConversion() {
                std::string tok = _tokStr(); _advance(2);
                Exptree* node = new Exptree(); node->valtp = Exptree::val_t::valFunction; node->var = tok;
                return node;
            }

            Exptree* parseSIPrefix() {
                _advance(1);
                int8_t exp = (int8_t)_peek(); _advance(1);
                Exptree* ten = new Exptree(); ten->valtp = Exptree::val_t::valRational; ten->value = Rational(10);
                Exptree* expNode = new Exptree(); expNode->valtp = Exptree::val_t::valRational; expNode->value = Rational((int)exp);
                Exptree* power = _makeBinary("^", ten, expNode);
                Exptree* mul = new Exptree(); mul->valtp = Exptree::val_t::valFunction; mul->var = "*"; mul->child = {power, nullptr};
                return mul;
            }

            Exptree* parseSpecialVar() {
                std::string tok = _tokStr(); _advance(2);
                Exptree* node = new Exptree(); node->valtp = Exptree::val_t::valVariable; node->var = tok;
                return node;
            }

            // =============================================================
            // Node factory
            // =============================================================

            Exptree* _makeBinary(const std::string &op, Exptree *l, Exptree *r) {
                Exptree* node = new Exptree(); node->valtp = Exptree::val_t::valFunction;
                node->var = op; node->child = {l, r}; return node;
            }
            Exptree* _makeUnary(const std::string &op, Exptree *arg) {
                Exptree* node = new Exptree(); node->valtp = Exptree::val_t::valFunction;
                node->var = op; node->child = {arg}; return node;
            }
            Exptree* _makeFunction(const std::string &fn, const std::vector<Exptree*> &args) {
                Exptree* node = new Exptree(); node->valtp = Exptree::val_t::valFunction;
                node->var = fn; node->child = args; return node;
            }

            Exptree* _makeRational(const std::string &str, int base = 10) {
                Exptree* node = new Exptree(); node->valtp = Exptree::val_t::valRational;
                if (base == 16) {
                    int64_t val = 0;
                    for (char c : str) { val *= 16; val += (c<='9') ? c-'0' : (c<='F' ? c-'A'+10 : c-'a'+10); }
                    node->value = Rational(val);
                } else {
                    size_t dot = str.find('.');
                    if (dot == std::string::npos) node->value = Rational(std::stoll(str));
                    else {
                        std::string ip = str.substr(0, dot), fp = str.substr(dot+1);
                        if (fp.empty()) node->value = Rational(std::stoll(ip));
                        else {
                            int64_t num = std::stoll(ip + fp), den = 1;
                            for (size_t i = 0; i < fp.size(); i++) den *= 10;
                            node->value = Rational(num, den);
                        }
                    }
                }
                return node;
            }

            static const int BLKLEN = 2;
    };

    inline std::string Parser::_lastError;

} // namespace CAS

#endif // _CAS_PARSER_HPP_