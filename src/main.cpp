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
    #include <hardware/spi.h>
    #include <pico/stdlib.h>
    #include <pico/time.h>
}

#ifndef PROGMEM
#define PROGMEM __attribute__((section(".rodata")))
#endif

#include "dispinterface/stddisplay.hpp"
#include "rgb.h"

using namespace std;

// Include the ClassWiz font
#include "../fonts/CW.h"

/**
 * @brief Test pattern: Display color bars to verify all color channels
 * @param display Reference to display driver
 */
void test_color_bars(Display::RedTFTdisp &display) {
    uint8_t bar_width = TFT_WIDTH / 8;
    uint16_t colors[] = {
        (uint16_t)Color::RED,
        (uint16_t)Color::GREEN,
        (uint16_t)Color::BLUE,
        (uint16_t)Color::YELLOW,
        (uint16_t)Color::CYAN,
        (uint16_t)Color::MAGENTA,
        (uint16_t)Color::WHITE,
        (uint16_t)Color::BLACK
    };
    
    for (int i = 0; i < 8; i++) {
        display.DrawRect(i * bar_width, 0, bar_width, TFT_HEIGHT, colors[i]);
    }
    sleep_ms(2000);
}

/**
 * @brief Test pattern: Display basic ASCII text with ClassWiz font
 * @param display Reference to display driver
 */
void test_basic_text(Display::RedTFTdisp &display) {
    display.ClearScreen((uint16_t)Color::BLACK);
    
    // Display font name
    display.DrawText(5, 5, &ClassWiz_CW_Display_Regular12pt, 1, 
                    "ClassWiz Font", (uint16_t)Color::WHITE);
    
    // Display ASCII characters
    display.DrawText(5, 25, &ClassWiz_CW_Display_Regular12pt, 1,
                    "ABCDEFGHIJKLM", (uint16_t)Color::CYAN);
    display.DrawText(5, 42, &ClassWiz_CW_Display_Regular12pt, 1,
                    "NOPQRSTUVWXYZ", (uint16_t)Color::CYAN);
    display.DrawText(5, 59, &ClassWiz_CW_Display_Regular12pt, 1,
                    "abcdefghijklm", (uint16_t)Color::GREEN);
    display.DrawText(5, 76, &ClassWiz_CW_Display_Regular12pt, 1,
                    "nopqrstuvwxyz", (uint16_t)Color::GREEN);
    display.DrawText(5, 93, &ClassWiz_CW_Display_Regular12pt, 1,
                    "0123456789", (uint16_t)Color::YELLOW);
    
    // Display special characters
    display.DrawText(5, 110, &ClassWiz_CW_Display_Regular12pt, 1,
                    "!@#$%^&*()_+-=[]", (uint16_t)Color::MAGENTA);
    display.DrawText(5, 127, &ClassWiz_CW_Display_Regular12pt, 1,
                    "{}|;:',.<>?/~`", (uint16_t)Color::MAGENTA);
    
    sleep_ms(3000);
}

/**
 * @brief Test pattern: Display mathematical expressions
 * @param display Reference to display driver
 */
void test_math_expressions(Display::RedTFTdisp &display) {
    display.ClearScreen((uint16_t)Color::BLACK);
    
    // Mathematical expressions
    display.DrawText(5, 5, &ClassWiz_CW_Display_Regular12pt, 1,
                    "Math Test:", (uint16_t)Color::WHITE);
    
    display.DrawText(5, 25, &ClassWiz_CW_Display_Regular12pt, 1,
                    "sin(x) + cos(x)", (uint16_t)Color::CYAN);
    display.DrawText(5, 42, &ClassWiz_CW_Display_Regular12pt, 1,
                    "x^2 + y^2 = r^2", (uint16_t)Color::GREEN);
    display.DrawText(5, 59, &ClassWiz_CW_Display_Regular12pt, 1,
                    "a/b + c/d = ?", (uint16_t)Color::YELLOW);
    display.DrawText(5, 76, &ClassWiz_CW_Display_Regular12pt, 1,
                    "sqrt(9) = 3", (uint16_t)Color::MAGENTA);
    display.DrawText(5, 93, &ClassWiz_CW_Display_Regular12pt, 1,
                    "pi * r^2", (uint16_t)Color::RED);
    display.DrawText(5, 110, &ClassWiz_CW_Display_Regular12pt, 1,
                    "e^(i*pi) = -1", (uint16_t)Color::WHITE);
    
    sleep_ms(3000);
}

/**
 * @brief Test pattern: Display calculator-style interface
 * @param display Reference to display driver
 */
void test_calculator_ui(Display::RedTFTdisp &display) {
    display.ClearScreen((uint16_t)Color::BLACK);
    
    // Title bar
    display.DrawRect(0, 0, TFT_WIDTH, 15, (uint16_t)Color::BLUE);
    display.DrawText(2, 2, &ClassWiz_CW_Display_Regular12pt, 1,
                    "CALC", (uint16_t)Color::WHITE);
    
    // Display area
    display.DrawRect(0, 15, TFT_WIDTH, 40, (uint16_t)Color::ORANGE);
    display.DrawText(5, 20, &ClassWiz_CW_Display_Regular12pt, 1,
                    "123 + 456", (uint16_t)Color::WHITE);
    display.DrawText(5, 37, &ClassWiz_CW_Display_Regular12pt, 1,
                    "= 579", (uint16_t)Color::GREEN);
    
    // Button labels
    display.DrawText(5, 62, &ClassWiz_CW_Display_Regular12pt, 1,
                    "sin cos tan log", (uint16_t)Color::CYAN);
    display.DrawText(5, 79, &ClassWiz_CW_Display_Regular12pt, 1,
                    "x^2 sqrt pi e", (uint16_t)Color::YELLOW);
    
    // Number pad simulation
    int start_y = 100;
    const char* buttons[] = {
        "7", "8", "9", "+",
        "4", "5", "6", "-",
        "1", "2", "3", "*",
        "0", ".", "=", "/"
    };
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int x = 5 + col * 30;
            int y = start_y + row * 15;
            display.DrawRect(x, y, 28, 13, (uint16_t)Color::ORANGE);
            display.DrawText(x + 2, y + 2, &ClassWiz_CW_Display_Regular12pt, 
                           1, buttons[row * 4 + col], (uint16_t)Color::WHITE);
        }
    }
    
    sleep_ms(4000);
}

/**
 * @brief Test pattern: Display memory and statistics
 * @param display Reference to display driver
 */
void test_memory_stats(Display::RedTFTdisp &display) {
    display.ClearScreen((uint16_t)Color::BLACK);
    
    // Header
    display.DrawText(5, 5, &ClassWiz_CW_Display_Regular12pt, 1,
                    "Statistics:", (uint16_t)Color::WHITE);
    
    // Statistical data
    display.DrawText(5, 25, &ClassWiz_CW_Display_Regular12pt, 1,
                    "n = 100", (uint16_t)Color::CYAN);
    display.DrawText(5, 42, &ClassWiz_CW_Display_Regular12pt, 1,
                    "x = 50.5", (uint16_t)Color::GREEN);
    display.DrawText(5, 59, &ClassWiz_CW_Display_Regular12pt, 1,
                    "sx = 10.2", (uint16_t)Color::YELLOW);
    display.DrawText(5, 76, &ClassWiz_CW_Display_Regular12pt, 1,
                    "minX = 20", (uint16_t)Color::MAGENTA);
    display.DrawText(5, 93, &ClassWiz_CW_Display_Regular12pt, 1,
                    "maxX = 80", (uint16_t)Color::RED);
    
    // Memory indicators
    display.DrawText(5, 115, &ClassWiz_CW_Display_Regular12pt, 1,
                    "M= A= B= C= D=", (uint16_t)Color::WHITE);
    
    // Angle mode
    display.DrawText(5, 135, &ClassWiz_CW_Display_Regular12pt, 1,
                    "DEG  FIX  SCI", (uint16_t)Color::CYAN);
    
    sleep_ms(3000);
}

/**
 * @brief Test pattern: Display all available symbols
 * @param display Reference to display driver
 */
void test_symbols(Display::RedTFTdisp &display) {
    display.ClearScreen((uint16_t)Color::BLACK);
    
    // Greek letters and math symbols
    display.DrawText(5, 5, &ClassWiz_CW_Display_Regular12pt, 1,
                    "Symbols:", (uint16_t)Color::WHITE);
    
    // Some Unicode math symbols (if available in font)
    // Note: These characters need to be properly encoded
    string math_syms = "x^2 + y^2 = z^2";
    display.DrawText(5, 25, &ClassWiz_CW_Display_Regular12pt, 1,
                    math_syms, (uint16_t)Color::CYAN);
    
    string frac_test = "1/2 + 2/3 = 7/6";
    display.DrawText(5, 42, &ClassWiz_CW_Display_Regular12pt, 1,
                    frac_test, (uint16_t)Color::GREEN);
    
    string sqrt_test = "sqrt(16) = 4";
    display.DrawText(5, 59, &ClassWiz_CW_Display_Regular12pt, 1,
                    sqrt_test, (uint16_t)Color::YELLOW);
    
    string power_test = "10^3 = 1000";
    display.DrawText(5, 76, &ClassWiz_CW_Display_Regular12pt, 1,
                    power_test, (uint16_t)Color::MAGENTA);
    
    sleep_ms(3000);
}

/**
 * @brief Main entry point
 * 
 * Initializes the display and runs through a series of test patterns
 * to verify correct operation of the ST7735 red tab display driver
 * with the ClassWiz font.
 * 
 * @return int Exit code (never returns)
 */
int main() {
    // Initialize LED for status indication
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    // Initialize display
    Display::RedTFTdisp display;
    display.InitPin();
    display.InitDisplay();
    
    // Welcome blink
    for (int i = 0; i < 3; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(100);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(100);
    }
    
    // Run test sequence
    while (true) {
        // Test 1: Color bars (hardware test)
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        test_color_bars(display);
        
        // Test 2: Basic ASCII text
        test_basic_text(display);
        
        // Test 3: Mathematical expressions
        test_math_expressions(display);
        
        // Test 4: Calculator UI simulation
        test_calculator_ui(display);
        
        // Test 5: Memory and statistics
        test_memory_stats(display);
        
        // Test 6: Symbols
        test_symbols(display);
        
        // Cycle complete indicator
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(200);
    }
    
    return 0;
}