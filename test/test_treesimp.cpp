/** @file /test/test_treesimp.cpp
 *  @brief Interactive test for expression tree simplification
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

#include "cas/treesimp.hpp"
#include "cas/exptree.hpp"
#include "cas/expdef.hpp"
#include "cas/expand.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cctype>

using namespace CAS;

// ========== Forward declarations ==========

/**
 *  @brief Parse an S-expression string into an Exptree
 *  @param str Input string like "+(1,*(4,/(sqrt(999),9)))"
 *  @return Root of parsed expression tree, or nullptr on error
 */
Exptree* parseExpr(const std::string& str, size_t& pos);

/**
 *  @brief Print an expression tree in S-expression format
 *  @param node Root node to print
 */
void printExpr(Exptree* node);

// ========== Parser ==========

/**
 *  @brief Skip whitespace
 */
static void skipSpace(const std::string& str, size_t& pos) {
    while (pos < str.size() && std::isspace(static_cast<unsigned char>(str[pos]))) {
        pos++;
    }
}

/**
 *  @brief Read an identifier (function name or variable)
 */
static std::string readIdent(const std::string& str, size_t& pos) {
    std::string ident;
    while (pos < str.size() && (std::isalnum(static_cast<unsigned char>(str[pos])) || str[pos] == '_')) {
        ident += str[pos];
        pos++;
    }
    return ident;
}

/**
 *  @brief Read a rational number (integer or fraction like 3/4)
 */
static Rational readRational(const std::string& str, size_t& pos) {
    bool negative = false;
    if (pos < str.size() && str[pos] == '-') {
        negative = true;
        pos++;
    }

    Intg num(0);
    while (pos < str.size() && std::isdigit(static_cast<unsigned char>(str[pos]))) {
        num = num * Intg(10) + Intg(static_cast<int>(str[pos] - '0'));
        pos++;
    }

    if (pos < str.size() && str[pos] == '/') {
        pos++; // skip '/'
        Intg den(0);
        while (pos < str.size() && std::isdigit(static_cast<unsigned char>(str[pos]))) {
            den = den * Intg(10) + Intg(static_cast<int>(str[pos] - '0'));
            pos++;
        }
        if (negative) num = Intg(0) - num;
        return Rational(num, den);
    }

    if (negative) num = Intg(0) - num;
    return Rational(num);
}

/**
 *  @brief Parse a single token: number, variable, or function call
 */
Exptree* parseToken(const std::string& str, size_t& pos) {
    skipSpace(str, pos);
    if (pos >= str.size()) return nullptr;

    // Function call or operator: ident(args...) or +(args...)
    if (std::isalpha(static_cast<unsigned char>(str[pos])) ||
        str[pos] == '+' || str[pos] == '-' || str[pos] == '*' ||
        str[pos] == '/' || str[pos] == '^') {
        // Check if '-' is followed by digit — it's a negative number
        if (str[pos] == '-' && pos + 1 < str.size() && std::isdigit(str[pos + 1])) {
            Rational r = readRational(str, pos);
            return SimpUtil::makeRational(r);
        }
        std::string funcName;
        if (str[pos] == '+' || str[pos] == '-' || str[pos] == '*' ||
            str[pos] == '/' || str[pos] == '^') {
            funcName = str[pos];
            pos++;
        } else {
            funcName = readIdent(str, pos);
        }
        skipSpace(str, pos);

        if (pos < str.size() && str[pos] == '(') {
            pos++;

            const char* internalName = funcName.c_str();

            if (funcName == "sin") internalName = FuncName::sin;
            else if (funcName == "cos") internalName = FuncName::cos;
            else if (funcName == "tan") internalName = FuncName::tan;
            else if (funcName == "asin") internalName = FuncName::asin;
            else if (funcName == "acos") internalName = FuncName::acos;
            else if (funcName == "atan") internalName = FuncName::atan;
            else if (funcName == "sinh") internalName = FuncName::sinh;
            else if (funcName == "cosh") internalName = FuncName::cosh;
            else if (funcName == "tanh") internalName = FuncName::tanh;
            else if (funcName == "asinh") internalName = FuncName::asinh;
            else if (funcName == "acosh") internalName = FuncName::acosh;
            else if (funcName == "atanh") internalName = FuncName::atanh;
            else if (funcName == "ln") internalName = FuncName::ln;
            else if (funcName == "log") internalName = FuncName::log;
            else if (funcName == "log10") internalName = FuncName::log10;
            else if (funcName == "exp") internalName = FuncName::exp;
            else if (funcName == "sqrt") internalName = FuncName::sqrt;
            else if (funcName == "root") internalName = FuncName::root;
            else if (funcName == "abs") internalName = FuncName::abs;
            else if (funcName == "floor") internalName = FuncName::floor;
            else if (funcName == "ceil") internalName = FuncName::ceil;
            else if (funcName == "round") internalName = FuncName::round;
            else if (funcName == "sign") internalName = FuncName::sign;
            else if (funcName == "max") internalName = FuncName::max;
            else if (funcName == "min") internalName = FuncName::min;
            else if (funcName == "frac") internalName = FuncName::frac;
            else if (funcName == "fact") internalName = FuncName::fact;
            else if (funcName == "gcd") internalName = FuncName::gcd;
            else if (funcName == "lcm") internalName = FuncName::lcm;
            else if (funcName == "mod") internalName = FuncName::mod;
            else if (funcName == "permut") internalName = FuncName::permut;
            else if (funcName == "combin") internalName = FuncName::combin;
            else if (funcName == "deg") internalName = FuncName::deg;
            else if (funcName == "rad") internalName = FuncName::rad;
            else if (funcName == "polar") internalName = FuncName::polar;
            else if (funcName == "rect") internalName = FuncName::rect;
            else if (funcName == "realpart") internalName = FuncName::realpart;
            else if (funcName == "imagpart") internalName = FuncName::imagpart;
            else if (funcName == "conjg") internalName = FuncName::conjg;
            else if (funcName == "arg") internalName = FuncName::arg;
            else if (funcName == "vector") internalName = FuncName::vector;
            else if (funcName == "matrix") internalName = FuncName::matrix;
            else if (funcName == "dot") internalName = FuncName::dot;
            else if (funcName == "angle") internalName = FuncName::angle;
            else if (funcName == "det") internalName = FuncName::det;
            else if (funcName == "transpose") internalName = FuncName::transpose;
            else if (funcName == "randrat") internalName = FuncName::randrat;
            else if (funcName == "randint") internalName = FuncName::randint;
            else if (funcName == "norm") internalName = FuncName::norm;
            else if (funcName == "eigenval") internalName = FuncName::eigenval;
            else if (funcName == "eigenvec") internalName = FuncName::eigenvec;
            else if (funcName == "adjoint") internalName = FuncName::adjoint;
            else if (funcName == "rank") internalName = FuncName::rank;

            Exptree* node = SimpUtil::makeFunction(internalName);

            // Parse arguments
            while (pos < str.size() && str[pos] != ')') {
                Exptree* arg = parseToken(str, pos);
                if (arg) {
                    node->child.push_back(arg);
                }
                skipSpace(str, pos);
                if (pos < str.size() && str[pos] == ',') {
                    pos++; // skip ','
                }
            }
            if (pos < str.size() && str[pos] == ')') {
                pos++; // skip ')'
            }

            return node;
        } else {
            // Variable or constant
            const char* varName = funcName.c_str();
            if (funcName == "pi") varName = ConstName::pi;
            else if (funcName == "e") varName = ConstName::e;
            else if (funcName == "i") varName = ConstName::i;
            else if (funcName == "phi") varName = ConstName::phi;
            Exptree *node;
            if (funcName == "inf") node = SimpUtil::makeRational(Rational(Intg("inf")));
            else node = SimpUtil::makeVariable(varName);
            return node;
        }
    }

    // Number
    if (std::isdigit(static_cast<unsigned char>(str[pos])) || str[pos] == '-') {
        Rational r = readRational(str, pos);
        return SimpUtil::makeRational(r);
    }

    // Unknown
    pos++;
    return nullptr;
}

/**
 *  @brief Parse a full S-expression
 */
Exptree* parseExpr(const std::string& str, size_t& pos) {
    return parseToken(str, pos);
}

// ========== Printer ==========

/**
 *  @brief Get display name from internal function code
 */
static const char* getDisplayName(const std::string& var) {
    // Constants
    if (var == ConstName::e)        return "e";
    if (var == ConstName::pi)       return "pi";
    if (var == ConstName::i)        return "i";
    if (var == ConstName::phi)      return "phi";
    // Functions
    if (var == FuncName::sin)       return "sin";
    if (var == FuncName::cos)       return "cos";
    if (var == FuncName::tan)       return "tan";
    if (var == FuncName::asin)      return "asin";
    if (var == FuncName::acos)      return "acos";
    if (var == FuncName::atan)      return "atan";
    if (var == FuncName::sinh)      return "sinh";
    if (var == FuncName::cosh)      return "cosh";
    if (var == FuncName::tanh)      return "tanh";
    if (var == FuncName::asinh)     return "asinh";
    if (var == FuncName::acosh)     return "acosh";
    if (var == FuncName::atanh)     return "atanh";
    if (var == FuncName::ln)        return "ln";
    if (var == FuncName::log)       return "log";
    if (var == FuncName::log10)     return "log10";
    if (var == FuncName::exp)       return "exp";
    if (var == FuncName::sqrt)      return "sqrt";
    if (var == FuncName::root)      return "root";
    if (var == FuncName::abs)       return "abs";
    if (var == FuncName::floor)     return "floor";
    if (var == FuncName::ceil)      return "ceil";
    if (var == FuncName::round)     return "round";
    if (var == FuncName::sign)      return "sign";
    if (var == FuncName::max)       return "max";
    if (var == FuncName::min)       return "min";
    if (var == FuncName::frac)      return "frac";
    if (var == FuncName::fact)      return "fact";
    if (var == FuncName::gcd)       return "gcd";
    if (var == FuncName::lcm)       return "lcm";
    if (var == FuncName::mod)       return "mod";
    if (var == FuncName::permut)    return "permut";
    if (var == FuncName::combin)    return "combin";
    if (var == FuncName::deg)       return "deg";
    if (var == FuncName::rad)       return "rad";
    if (var == FuncName::polar)     return "polar";
    if (var == FuncName::rect)      return "rect";
    if (var == FuncName::realpart)  return "realpart";
    if (var == FuncName::imagpart)  return "imagpart";
    if (var == FuncName::conjg)     return "conjg";
    if (var == FuncName::arg)       return "arg";
    if (var == FuncName::vector)    return "vector";
    if (var == FuncName::matrix)    return "matrix";
    if (var == FuncName::dot)       return "dot";
    if (var == FuncName::angle)     return "angle";
    if (var == FuncName::det)       return "det";
    if (var == FuncName::transpose) return "transpose";
    if (var == FuncName::randrat)   return "randrat";
    if (var == FuncName::randint)   return "randint";
    if (var == FuncName::norm)      return "norm";
    if (var == FuncName::eigenval)  return "eigenval";
    if (var == FuncName::eigenvec)  return "eigenvec";
    if (var == FuncName::adjoint)   return "adjoint";
    if (var == FuncName::rank)      return "rank";
    if (var == FuncName::defint)    return "dint";
    return var.c_str();
}

/**
 *  @brief Print expression tree in S-expression format
 */
void printExpr(Exptree* node) {
    if (!node) {
        std::cout << "null";
        return;
    }

    if (node->valtp == Exptree::val_t::valRational) {
        Intg num = node->value.numerator();
        Intg den = node->value.den;
        std::cout << std::string(num);
        if (den != Intg(1)) {
            std::cout << "/" << std::string(den);
        }
        return;
    }

    if (node->valtp == Exptree::val_t::valVariable) {
        std::cout << getDisplayName(node->var);
        return;
    }

    if (node->valtp == Exptree::val_t::valFunction) {
        std::cout << getDisplayName(node->var) << "(";
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (i > 0) std::cout << ",";
            printExpr(node->child[i]);
        }
        std::cout << ")";
        return;
    }

    std::cout << "?";
}

// ========== Main ==========

int main() {
    std::cout << "Expression Tree Simplifier Test" << std::endl;
    std::cout << "Enter expressions in S-expression format:" << std::endl;
    std::cout << "  Numbers: 42, -5, 3/4" << std::endl;
    std::cout << "  Variables: x, y, e, pi, i, phi" << std::endl;
    std::cout << "  Functions: +(a,b), *(a,b), ^(a,b), sin(x), sqrt(x), ..." << std::endl;
    std::cout << "  Example: +(1,*(4,/(sqrt(999),9)))" << std::endl;
    std::cout << "  Type 'quit' to exit." << std::endl;
    std::cout << std::endl;

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty()) continue;
        if (line == "quit" || line == "exit" || line == "q") break;

        // Check for expand command: expand <expr>
        bool doExpand = false;
        std::string exprStr = line;
        if (line.rfind("expand ", 0) == 0) {
            doExpand = true;
            exprStr = line.substr(7);
        }

        // Parse
        size_t pos = 0;
        Exptree* expr = parseExpr(exprStr, pos);

        if (!expr) {
            std::cout << "Parse error at position " << pos << std::endl;
            continue;
        }

        // Print input
        std::cout << "In : ";
        printExpr(expr);
        std::cout << std::endl;

        // Simplify
        TreeSimplifier::simplify(expr);

        // Expand if requested
        if (doExpand) {
            TreeExpander::expand(expr);
            // Simplify again after expansion
            TreeSimplifier::simplify(expr);
        }

        // Print output
        std::cout << "Out: ";
        printExpr(expr);
        std::cout << std::endl;

        // Free tree
        SimpUtil::freeTree(expr);
    }

    std::cout << "Bye." << std::endl;
    return 0;
}