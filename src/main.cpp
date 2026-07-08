/** @file /src/main.cpp
 *  @brief Main entry point for the calculator project
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

extern "C" {
    #include <pico/stdlib.h>
    #include <pico/time.h>
}

#ifndef PROGMEM
#define PROGMEM __attribute__((aligned(4), section(".rodata")))
#endif
#include "dispinterface/stddisplay.hpp"
#include "keypadio.hpp"
#include "../fonts/CW.h"
#include "rgb.h"

#include "expbuild.hpp"
#include "cas/treesimp.hpp"
#include "cas/parser.hpp"
#include "cas/expand.hpp"

#include <sstream>

using namespace std;
using namespace Keypad;
using namespace CAS;

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
void printExpr(stringstream &ss, Exptree* node) {
    if (!node) {
        ss << "null";
        return;
    }

    if (node->valtp == Exptree::val_t::valRational) {
        Intg num = node->value.numerator();
        Intg den = node->value.den;
        ss << std::string(num);
        if (den != Intg(1)) {
            ss << "/" << std::string(den);
        }
        return;
    }

    if (node->valtp == Exptree::val_t::valVariable) {
        ss << getDisplayName(node->var);
        return;
    }

    if (node->valtp == Exptree::val_t::valFunction) {
        ss << getDisplayName(node->var) << "(";
        for (size_t i = 0; i < node->child.size(); ++i) {
            if (i > 0) ss << ",";
            printExpr(ss, node->child[i]);
        }
        ss << ")";
        return;
    }

    ss << "?";
    return;
}

int main() {
    // LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Init display
    Display::RedTFTdisp display;
    display.InitPin();
    display.InitDisplay();
    display.ClearScreen(0x0000);

    // Init keypad I2C
    Keypad::KeypadIO keypad;
    keypad.init();

    char buf[32];

    Expbuild expb;
    Exptree *expt, *st;

    while (true) {
        uint8_t row, col;
        if (keypad.read(row, col)) {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);

            display.ClearScreen(0x0000);

            // snprintf(buf, sizeof(buf), "R:%d C:%d", row, col);
            // display.DrawText(0, 12, &ClassWiz_CW_Display_Regular12pt, 1,
            //                 buf, 0xFFFF);

            uint8_t r = expb.press(row, col);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            sleep_ms(20);
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            display.DrawText(0, 12, &ClassWiz_CW_Display_Regular12pt, 1,
                            expb.exp.c_str(), ((expb.flg & 7) == 0) ? ((uint16_t)Color::ORANGE) : RGB111_to_RGB565(expb.flg & M_ALPHA, expb.flg & M_SHIFT, expb.flg & M_CTRL));
            display.DrawLine(expb.cp * 9, 0, expb.cp * 9, 12, (expb.flg & M_INSERT) ? (uint16_t)Color::PURPLE : (uint16_t)Color::ORANGE);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            if(r == B_EXEC) {
                stringstream ss;
                ss.clear();
                expt = Parser::parse(expb.exp);
                if(expt == nullptr) {
                    ss << Parser::getError();
                }
                st = TreeSimplifier::simplify(expt);
                display.ClearScreen(0x0000);
                printExpr(ss, st);
                // printExpr(ss, expt);
                SimpUtil::freeTree(expt);
                display.DrawText(0, 12, &ClassWiz_CW_Display_Regular12pt, 1,
                            ss.str().c_str(), ((uint16_t)Color::ORANGE));
                SimpUtil::freeTree(st);
            }
            if(r == B_EXPAND) {
                stringstream ss;
                ss.clear();
                expt = Parser::parse(expb.exp);
                if(expt == nullptr) {
                    ss << Parser::getError();
                }
                st = TreeSimplifier::simplify(expt);
                Exptree *ep = TreeExpander::expand(st);
                display.ClearScreen(0x0000);
                printExpr(ss, ep);
                // printExpr(ss, expt);
                SimpUtil::freeTree(expt);
                display.DrawText(0, 12, &ClassWiz_CW_Display_Regular12pt, 1,
                            ss.str().c_str(), ((uint16_t)Color::ORANGE));
                SimpUtil::freeTree(st);
                SimpUtil::freeTree(ep);
            }
        }
        sleep_ms(20);
    }

    return 0;
}