/** @file /test/test_keypad.cpp
 *  @brief Interactive test for keypad input handler
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

#include "keypad.hpp"
#include "expbuild.hpp"
#include <iostream>
#include <string>
#include <cstdint>

using namespace Keypad;

/** @brief Print expression with visible markers */
void printExpr(const std::string& expr, uint16_t cursor) {
    std::cout << "[";
    for (size_t i = 0; i < expr.size(); ++i) {
        if (i == cursor) std::cout << "|";
        unsigned char c = static_cast<unsigned char>(expr[i]);
        if (c >= 0x01 && c <= 0x05) {
            std::cout << "\\x" << std::hex << (int)c << std::dec;
            ++i;
            c = static_cast<unsigned char>(expr[i]);
            std::cout << "\\x" << std::hex << (int)c << std::dec;
        } else if (c == static_cast<unsigned char>(Ctrl::BLOCKL[0]) &&
                   i+1 < expr.size() &&
                   static_cast<unsigned char>(expr[i+1]) == static_cast<unsigned char>(Ctrl::BLOCKL[1])) {
            std::cout << "[>";
            ++i;
        } else if (c == static_cast<unsigned char>(Ctrl::BLOCKR[0]) &&
                   i+1 < expr.size() &&
                   static_cast<unsigned char>(expr[i+1]) == static_cast<unsigned char>(Ctrl::BLOCKR[1])) {
            std::cout << "<]";
            ++i;
        } else {
            std::cout << expr[i];
        }
    }
    if (cursor == expr.size()) std::cout << "|";
    std::cout << "]" << std::endl;
}

/** @brief Print modifier state */
void printMods(uint8_t flags) {
    std::cout << "  Mods:";
    if (flags & MASK_SHIFT)  std::cout << " SHIFT";
    if (flags & MASK_ALPHA)  std::cout << " ALPHA";
    if (flags & MASK_CTRL)   std::cout << " CTRL";
    if (flags & MASK_LOCK)   std::cout << " LOCK";
    if (flags & MASK_INSERT) std::cout << " INSERT";
    if (!(flags & (MASK_SHIFT|MASK_ALPHA|MASK_CTRL|MASK_LOCK|MASK_INSERT))) std::cout << " (none)";
    std::cout << std::endl;
}

int main() {
    std::cout << "Keypad Input Handler Test" << std::endl;
    std::cout << "Enter key as <row><col> (e.g. A1, B4, E3)." << std::endl;
    std::cout << "Special: DEL AC LEFT RIGHT SHOW HELP QUIT" << std::endl;
    std::cout << "Modifier keys (A1-A3, F6) toggle internal state." << std::endl;
    std::cout << std::endl;

    ExpressionBuilder builder;
    std::string line;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line.empty()) continue;
        for (auto &c : line) c = toupper(c);
        if (line == "QUIT" || line == "EXIT" || line == "Q") break;

        if (line == "DEL")  { builder.del(); printExpr(builder.expr(), builder.cursor()); continue; }
        if (line == "AC")   { builder.ac();  printExpr(builder.expr(), builder.cursor()); continue; }
        if (line == "LEFT") { builder.moveLeft();  printExpr(builder.expr(), builder.cursor()); continue; }
        if (line == "RIGHT"){ builder.moveRight(); printExpr(builder.expr(), builder.cursor()); continue; }
        if (line == "SHOW") { printExpr(builder.expr(), builder.cursor()); printMods(builder.flags()); continue; }
        if (line == "HELP") {
            std::cout << "  A1=SHIFT A2=ALPHA A3=CTRL F6(A6)=LOCK  C5(B5)=INS" << std::endl;
            std::cout << "  Modifiers auto-release after content key unless LOCKed." << std::endl;
            continue;
        }

        if (line.size() < 2) { std::cout << "Invalid key" << std::endl; continue; }

        uint8_t row = line[0] - 'A';
        uint8_t col = line[1] - '1';
        if (row > 5 || col > 5) { std::cout << "Invalid key" << std::endl; continue; }

        uint8_t r = builder.pressKey(row, col);
        if (!r) std::cout << "  (empty)" << std::endl;

        printExpr(builder.expr(), builder.cursor());
        printMods(builder.flags());
    }

    std::cout << "Bye." << std::endl;
    return 0;
}