/** @file /ino/keypad/keypad.ino
 *  @brief Keypad reader & data transmitter for the calculator project (nano)
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

#include <Wire.h>
#include <Keypad.h>

#define PICO_ADDR 0x08
#define ACK       0x06
#define NAK       0x15

const byte ROWS = 6;
const byte COLS = 6;

// All +1 to avoid 0x00 (Keypad lib swallows '\0')
char keys[ROWS][COLS] = {
    {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    {0x11, 0x12, 0x13, 0x14, 0x15, 0x16},
    {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
    {0x31, 0x32, 0x33, 0x34, 0x35, 0x36},
    {0x41, 0x42, 0x43, 0x44, 0x45, 0x46},
    {0x51, 0x52, 0x53, 0x54, 0x55, 0x56}
};

byte rowPins[ROWS] = {2, 3, 4, 5, 6, 7};
byte colPins[COLS] = {8, 9, 10, 11, 12, A0};  // C6 moved to A0

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
    keypad.setDebounceTime(25);
    Wire.begin();  // Master mode
}

void loop() {
    char c = keypad.getKey();
    if (!c) { delay(10); return; }

    uint8_t data = (uint8_t)c - 1;  // Restore original encoding

    // Send to Pico, retry until ACK
    while (true) {
        Wire.beginTransmission(PICO_ADDR);
        Wire.write(&data, 1);
        if (Wire.endTransmission() != 0) { delay(5); continue; }

        delay(1);
        int len = Wire.requestFrom(PICO_ADDR, (uint8_t)1);
        if (len == 1 && Wire.read() == ACK) break;
        delay(5);
    }
}