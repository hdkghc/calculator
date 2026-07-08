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
#include <stdexcept>

namespace CAS {

    /**
     * @class  Parser
     * @brief  Recursive-descent parser that converts an enriched-bytecode
     *         expression string into a clean Exptree AST.
     *
     * @details
     * Input is the expression string produced by Keypad::Expbuild.
     * The output AST contains only Rational numbers (decimals converted
     * to fractions), Variables, and Functions/Operators.  All structural
     * tokens (\x03 block markers, parentheses) and SI prefixes (\x05)
     * are eliminated during parsing.  Implicit multiplication is made
     * explicit.
     *
     * <b>Operator precedence (low to high, left-associative unless noted):</b>
     *   1.  =                                      (right-assoc)
     *   2.  STO (\x03\x22)                          (right-assoc)
     *   3.  ,                                       (argument separator)
     *   4.  bor  bxor  bnor  bxnor
     *   5.  band  bnand
     *   6.  blshift  brshift
     *   7.  +  -
     *   8.  *  /
     *   8.5 implicit multiplication
     *   9.  -  +  bnot  (unary, right-assoc)
     *  10.  ^                                       (right-assoc)
     *  11.  fact  estx1  estx2  esty  (postfix)
     *  12.  function calls  (atoms)
     *
     * <b>frac conversion:</b> \x01fr(a,b) is converted to the binary
     * division operator /.
     */
    class Parser {
    public:
        /**
         * @brief  Parse an enriched-bytecode expression into an AST.
         * @param  exp Expression string from Keypad::Expbuild.
         * @return     Root node of the expression tree (caller owns it).
         * @throws std::runtime_error on any syntax error.
         */
        static Exptree* parse(const std::string &exp) {
            Parser p(exp);
            Exptree* tree = p.parseExpression();
            if (p._pos < (int16_t)p._exp.size()) {
                throw std::runtime_error(
                    "Unexpected token '" + p._tokStr() +
                    "' at position " + std::to_string(p._pos));
            }
            return tree;
        }

    private:
        const std::string &_exp;  ///< Expression string being parsed
        int16_t           _pos;  ///< Current byte position

        Parser(const std::string &exp) : _exp(exp), _pos(0) {}

        // =============================================================
        // Low-level helpers
        // =============================================================

        uint8_t _peek() const {
            return (_pos < (int16_t)_exp.size()) ? (uint8_t)_exp[_pos] : 0;
        }

        uint8_t _peek(int16_t offset) const {
            int16_t idx = _pos + offset;
            return (idx >= 0 && idx < (int16_t)_exp.size())
                   ? (uint8_t)_exp[idx] : 0;
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
        static bool _isIdStart(uint8_t c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }
        static bool _isIdChar(uint8_t c) { return _isIdStart(c) || _isDigit(c); }

        bool _canStartAtom(uint8_t c) const {
            return _isDigit(c) || c == '.' || _isIdStart(c) ||
                   c == '(' || (c >= 0x01 && c <= 0x06);
        }

        static bool _canImplicitMul(const Exptree* node) {
            if (!node) return false;
            return node->valtp == Exptree::val_t::valRational ||
                   node->valtp == Exptree::val_t::valVariable ||
                   node->valtp == Exptree::val_t::valFunction;
        }

        [[noreturn]] void _error(const std::string &msg) const {
            int16_t start = std::max((int16_t)0, (int16_t)(_pos - 5));
            int16_t len   = std::min((int16_t)10, (int16_t)(_exp.size() - start));
            std::string ctx = _exp.substr(start, len);
            throw std::runtime_error(
                msg + " at position " + std::to_string(_pos) +
                " near '" + ctx + "'");
        }

        // =============================================================
        // Recursive descent
        // =============================================================

        /** @brief Entry point: lowest precedence first. */
        Exptree* parseExpression() {
            return parseAssign();
        }

        /** @brief Priority 1–2: = (equation), STO (assignment). */
        Exptree* parseAssign() {
            Exptree* left = parseComma();
            while (!_eof()) {
                if (_peek() == '=') {
                    _advance();
                    left = _makeBinary("=", left, parseAssign());
                } else if (_isSTO()) {
                    _advance(2);
                    left = _makeBinary("\x03\x22", left, parseAssign());
                } else break;
            }
            return left;
        }

        /** @brief Priority 3: comma (argument separator). */
        Exptree* parseComma() {
            Exptree* left = parseBitwiseOr();
            if (_eof() || _peek() != ',') return left;
            Exptree* comma = _makeFunction(",", {left});
            while (!_eof() && _peek() == ',') {
                _advance();
                comma->child.push_back(parseBitwiseOr());
            }
            return comma;
        }

        /** @brief Priority 4: bor, bxor, bnor, bxnor. */
        Exptree* parseBitwiseOr() {
            Exptree* left = parseBitwiseAnd();
            while (_isFunc()) {
                std::string fn = _tokStr();
                if (fn != CAS::FuncName::bor  && fn != CAS::FuncName::bxor &&
                    fn != CAS::FuncName::bnor && fn != CAS::FuncName::bxnor) break;
                _advance(3);
                left = _makeBinary(fn, left, parseBitwiseAnd());
            }
            return left;
        }

        /** @brief Priority 5: band, bnand. */
        Exptree* parseBitwiseAnd() {
            Exptree* left = parseShift();
            while (_isFunc()) {
                std::string fn = _tokStr();
                if (fn != CAS::FuncName::band && fn != CAS::FuncName::bnand) break;
                _advance(3);
                left = _makeBinary(fn, left, parseShift());
            }
            return left;
        }

        /** @brief Priority 6: blshift, brshift. */
        Exptree* parseShift() {
            Exptree* left = parseAddSub();
            while (_isFunc()) {
                std::string fn = _tokStr();
                if (fn != CAS::FuncName::blshift && fn != CAS::FuncName::brshift) break;
                _advance(3);
                left = _makeBinary(fn, left, parseAddSub());
            }
            return left;
        }

        /** @brief Priority 7: +, -. */
        Exptree* parseAddSub() {
            Exptree* left = parseMulDiv();
            while (!_eof()) {
                uint8_t c = _peek();
                if (c == '+' || c == '-') {
                    _advance();
                    left = _makeBinary(std::string(1, (char)c), left, parseMulDiv());
                } else break;
            }
            return left;
        }

        /** @brief Priority 8–8.5: *, /, implicit multiplication. */
        Exptree* parseMulDiv() {
            Exptree* left = parseUnary();
            while (!_eof()) {
                uint8_t c = _peek();
                if (c == '*' || c == '/') {
                    _advance();
                    left = _makeBinary(std::string(1, (char)c), left, parseUnary());
                    continue;
                }
                // Implicit multiplication
                if (_canStartAtom(c) && _canImplicitMul(left)) {
                    left = _makeBinary("*", left, parseUnary());
                    continue;
                }
                break;
            }
            return left;
        }

        /** @brief Priority 9: unary -, +, bnot (right-assoc). */
        Exptree* parseUnary() {
            uint8_t c = _peek();
            if (c == '-') {
                _advance();
                return _makeUnary("-", parseUnary());
            }
            if (c == '+') {
                _advance();
                return parseUnary();
            }
            if (_isFunc() && _tokStr() == CAS::FuncName::bnot) {
                _advance(3);
                return _makeUnary(CAS::FuncName::bnot, parseUnary());
            }
            return parsePower();
        }

        /** @brief Priority 10: ^ (right-assoc). */
        Exptree* parsePower() {
            Exptree* left = parsePostfix();
            if (!_eof() && _peek() == '^') {
                _advance();
                if (!_eof() && _isBlockL()) {
                    left = parseBoxedPower(left);
                } else {
                    left = _makeBinary("^", left, parsePower());
                }
            }
            return left;
        }

        /** @brief Boxed power: ^ \x03\x20 exponent \x03\x21 */
        Exptree* parseBoxedPower(Exptree* base) {
            if (!_isBlockL()) _error("Expected block after ^");
            _advance(BLKLEN);
            Exptree* exp = nullptr;
            if (!_eof() && !_isBlockR()) exp = parseExpression();
            if (!exp) _error("Empty exponent in power");
            if (!_isBlockR()) _error("Expected \\x03\\x21 to close exponent block");
            _advance(BLKLEN);
            return _makeBinary("^", base, exp);
        }

        /** @brief Priority 11: postfix fact, estx1, estx2, esty. */
        Exptree* parsePostfix() {
            Exptree* node = parseAtom();
            while (_isFunc()) {
                std::string fn = _tokStr();
                if (fn == CAS::FuncName::fact ||
                    fn == CAS::FuncName::estx1 ||
                    fn == CAS::FuncName::estx2 ||
                    fn == CAS::FuncName::esty) {
                    _advance(3);
                    node = _makeUnary(fn, node);
                } else break;
            }
            return node;
        }

        // =============================================================
        // Atom dispatcher
        // =============================================================

        Exptree* parseAtom() {
            if (_eof()) _error("Unexpected end of expression");
            uint8_t c = _peek();

            if (_isDigit(c) || (c == '.' && _isDigit(_peek(1))))
                return parseNumber();
            if (c == '0' && (_peek(1) == 'x' || _peek(1) == 'X'))
                return parseHexNumber();
            if (_isIdStart(c))
                return parseVariable();
            if (c >= 0x01 && c <= 0x06)
                return parseEnrichedToken();
            if (c == '(')
                return parseParenGroup();
            if (c == ' ') {
                _advance();
                return parseAtom();
            }
            _error(std::string("Unexpected character '") + (char)c + "'");
        }

        // =============================================================
        // Atom parsers
        // =============================================================

        /** @brief Decimal number → Rational. */
        Exptree* parseNumber() {
            int16_t start = _pos;
            while (!_eof() && (_isDigit(_peek()) || _peek() == '.')) _advance();
            std::string s = _exp.substr(start, _pos - start);
            if (std::count(s.begin(), s.end(), '.') > 1)
                _error("Invalid number with multiple decimal points");
            return _makeRational(s);
        }

        /** @brief Hex number 0x… → Rational integer. */
        Exptree* parseHexNumber() {
            _advance(2);
            int16_t start = _pos;
            while (!_eof() && _isHexDigit(_peek())) _advance();
            if (_pos == start) _error("Expected hex digits after 0x");
            return _makeRational(_exp.substr(start, _pos - start), 16);
        }

        /** @brief Parenthesised group (no AST node). */
        Exptree* parseParenGroup() {
            _advance();
            Exptree* inner = parseExpression();
            if (_eof() || _peek() != ')')
                _error("Expected ')' to close parenthesised expression");
            _advance();
            return inner;
        }

        /** @brief Variable identifier. */
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
                case 0x03: _error("Unexpected control token in expression");
                case 0x04: return parseConversion();
                case 0x05: return parseSIPrefix();
                case 0x06: return parseSpecialVar();
                default:   _error("Unknown enriched token");
            }
        }

        /** @brief \x01 function: boxed, parenthesised, or bare. */
        Exptree* parseFunction() {
            std::string fn = _tokStr();
            _advance(3);

            // frac(a,b) → /  (binary division)
            if (fn == CAS::FuncName::frac) {
                if (_eof() || !_isBlockL())
                    _error("frac requires input blocks");
                return parseFrac();
            }

            if (!_eof() && _isBlockL())
                return parseBoxedFunction(fn);
            if (!_eof() && _peek() == '(')
                return parseParenFunction(fn);
            return _makeFunction(fn, {});
        }

        /** @brief frac: \x01fr \x03\x20 a \x03\x21 \x03\x20 b \x03\x21 → a / b */
        Exptree* parseFrac() {
            _advance(BLKLEN); // skip first \x03\x20
            Exptree* num = nullptr;
            if (!_eof() && !_isBlockR()) num = parseExpression();
            if (!_isBlockR()) _error("Expected \\x03\\x21 after frac numerator");
            _advance(BLKLEN);

            if (!_isBlockL()) _error("Expected second block for frac denominator");
            _advance(BLKLEN);
            Exptree* den = nullptr;
            if (!_eof() && !_isBlockR()) den = parseExpression();
            if (!_isBlockR()) _error("Expected \\x03\\x21 after frac denominator");
            _advance(BLKLEN);

            if (!num) _error("Missing numerator in frac");
            if (!den) _error("Missing denominator in frac");
            return _makeBinary("/", num, den);
        }

        /** @brief Boxed function: \x01xx \x03\x20 arg \x03\x21 ... */
        Exptree* parseBoxedFunction(const std::string &fn) {
            std::vector<Exptree*> args;
            while (!_eof() && _isBlockL()) {
                _advance(BLKLEN);
                if (!_eof() && !_isBlockR())
                    args.push_back(parseExpression());
                else
                    args.push_back(nullptr);
                if (_eof() || !_isBlockR())
                    _error("Expected \\x03\\x21 to close input block");
                _advance(BLKLEN);
            }
            if (args.empty())
                _error("Function has no input blocks");
            return _makeFunction(fn, args);
        }

        /** @brief Parenthesised function: \x01xx ( arg, … ) */
        Exptree* parseParenFunction(const std::string &fn) {
            _advance();
            std::vector<Exptree*> args;
            if (_peek() != ')') {
                args.push_back(parseExpression());
                while (!_eof() && _peek() == ',') {
                    _advance();
                    args.push_back(parseExpression());
                }
            }
            if (_eof() || _peek() != ')')
                _error("Expected ')' to close arguments");
            _advance();
            return _makeFunction(fn, args);
        }

        /** @brief \x02 constant. */
        Exptree* parseConstant() {
            std::string tok = _tokStr();
            _advance(2);
            Exptree* node = new Exptree();
            node->valtp = Exptree::val_t::valVariable;
            node->var   = tok;
            return node;
        }

        /** @brief \x04 unit conversion. */
        Exptree* parseConversion() {
            std::string tok = _tokStr();
            _advance(2);
            Exptree* node = new Exptree();
            node->valtp = Exptree::val_t::valFunction;
            node->var   = tok;
            return node;
        }

        /**
         * @brief  \x05 SI prefix → *10^exp.
         *
         * Builds a partial "*" node with 10^exp as left child.
         * The right child is filled by implicit multiplication in
         * parseMulDiv() when the following atom is parsed.
         */
        Exptree* parseSIPrefix() {
            _advance(1); // skip \x05
            uint8_t expByte = _peek();
            _advance(1);
            int8_t exp = (int8_t)expByte;

            Exptree* ten = new Exptree();
            ten->valtp = Exptree::val_t::valRational;
            ten->value = Rational(10);

            Exptree* expNode = new Exptree();
            expNode->valtp = Exptree::val_t::valRational;
            expNode->value = Rational((int)exp);

            Exptree* power = _makeBinary("^", ten, expNode);
            Exptree* mul   = new Exptree();
            mul->valtp = Exptree::val_t::valFunction;
            mul->var   = "*";
            mul->child = {power, nullptr};
            return mul;
        }

        /** @brief \x06 special variable. */
        Exptree* parseSpecialVar() {
            std::string tok = _tokStr();
            _advance(2);
            Exptree* node = new Exptree();
            node->valtp = Exptree::val_t::valVariable;
            node->var   = tok;
            return node;
        }

        // =============================================================
        // Node factory
        // =============================================================

        Exptree* _makeBinary(const std::string &op, Exptree *l, Exptree *r) {
            Exptree* node = new Exptree();
            node->valtp = Exptree::val_t::valFunction;
            node->var   = op;
            node->child = {l, r};
            return node;
        }

        Exptree* _makeUnary(const std::string &op, Exptree *arg) {
            Exptree* node = new Exptree();
            node->valtp = Exptree::val_t::valFunction;
            node->var   = op;
            node->child = {arg};
            return node;
        }

        Exptree* _makeFunction(const std::string &fn,
                               const std::vector<Exptree*> &args) {
            Exptree* node = new Exptree();
            node->valtp = Exptree::val_t::valFunction;
            node->var   = fn;
            node->child = args;
            return node;
        }

        /**
         * @brief  Convert decimal/hex string to Rational node.
         * @param  str  Number string (e.g. "3.14", "FF").
         * @param  base 10 or 16.
         */
        Exptree* _makeRational(const std::string &str, int base = 10) {
            Exptree* node = new Exptree();
            node->valtp = Exptree::val_t::valRational;
            if (base == 16) {
                int64_t val = 0;
                for (char c : str) {
                    val *= 16;
                    if (c >= '0' && c <= '9')      val += c - '0';
                    else if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
                    else if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
                }
                node->value = Rational(val);
            } else {
                size_t dot = str.find('.');
                if (dot == std::string::npos) {
                    node->value = Rational(std::stoll(str));
                } else {
                    std::string intPart  = str.substr(0, dot);
                    std::string fracPart = str.substr(dot + 1);
                    if (fracPart.empty()) {
                        node->value = Rational(std::stoll(intPart));
                    } else {
                        int64_t num = std::stoll(intPart + fracPart);
                        int64_t den = 1;
                        for (size_t i = 0; i < fracPart.size(); i++) den *= 10;
                        node->value = Rational(num, den);
                    }
                }
            }
            return node;
        }

        static const int BLKLEN = 2;
    };

} // namespace CAS

#endif // _CAS_PARSER_HPP_