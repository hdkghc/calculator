#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <cstdio>
#include <cstdlib>

// GFXglyph structure matches Adafruit GFX font format
typedef struct {
    uint16_t bitmapOffset; // Starting index into bitmap array
    uint8_t  width;        // Glyph width in pixels
    uint8_t  height;       // Glyph height in pixels
    uint8_t  xAdvance;     // Horizontal cursor advance after drawing
    int8_t   xOffset;      // X offset from cursor position
    int8_t   yOffset;      // Y offset from cursor position
} GFXglyph;

// GFXfont structure stores complete font data
typedef struct {
    uint8_t  *bitmap;      // Pixel bitmap array
    GFXglyph *glyph;       // Array of glyph descriptors
    uint16_t first;        // First character code in font
    uint16_t last;         // Last character code in font
    uint8_t  yAdvance;     // Vertical line spacing
    const char *subset;    // String of supported characters
} GFXfont;

// Parse hex values (0xXX) from a text line into a byte vector
static std::vector<uint8_t> parseUint8Array(const std::string& line)
{
    std::vector<uint8_t> res;
    size_t pos = 0;
    while (pos < line.size()) {
        size_t s = line.find("0x", pos);
        if (s == std::string::npos) break;
        uint8_t val = static_cast<uint8_t>(strtoul(&line[s + 2], nullptr, 16));
        res.push_back(val);
        pos = s + 2;
    }
    return res;
}

// Parse one line of glyph data into a GFXglyph struct
static bool parseGlyphLine(const std::string& line, GFXglyph& g)
{
    int off, w, h, adv, xo, yo;
    if (sscanf(line.c_str(), "  { %d, %d, %d, %d, %d, %d }",
               &off, &w, &h, &adv, &xo, &yo) == 6) {
        g.bitmapOffset = static_cast<uint16_t>(off);
        g.width = static_cast<uint8_t>(w);
        g.height = static_cast<uint8_t>(h);
        g.xAdvance = static_cast<uint8_t>(adv);
        g.xOffset = static_cast<int8_t>(xo);
        g.yOffset = static_cast<int8_t>(yo);
        return true;
    }
    return false;
}

// Render a single glyph to terminal using ANSI escape codes
void printGlyphInTerminal(const GFXglyph& g, const std::vector<uint8_t>& bitmap)
{
    printf("  Glyph: w=%d h=%d\n", g.width, g.height);
    uint32_t bitIdx = 0;

    for (int y = 0; y < g.height; y++) {
        printf("  ");
        for (int x = 0; x < g.width; x++) {
            size_t byteIdx = g.bitmapOffset + (bitIdx / 8);
            uint8_t bitPos = 7 - (bitIdx % 8);
            bitIdx++;

            if (byteIdx >= bitmap.size()) {
                printf("  ");
                continue;
            }

            // ANSI green block for 1, empty space for 0
            if (bitmap[byteIdx] & (1 << bitPos)) {
                printf("\033[42m  \033[0m");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
    printf("  ----------------------------------------\n");
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <gfx_font.h>" << std::endl;
        return 1;
    }

    std::ifstream f(argv[1]);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open file" << std::endl;
        return 1;
    }

    std::vector<uint8_t> bitmapData;
    std::vector<GFXglyph> glyphList;
    std::string subset;
    bool inBitmap = false;
    bool inGlyph = false;
    std::string line;

    // Parse entire font file
    while (std::getline(f, line)) {
        // Extract bitmap array
        if (line.find("Bitmaps[]") != std::string::npos) {
            inBitmap = true;
            continue;
        }
        if (inBitmap && line.find("};") != std::string::npos) {
            inBitmap = false;
        }
        if (inBitmap) {
            auto data = parseUint8Array(line);
            bitmapData.insert(bitmapData.end(), data.begin(), data.end());
        }

        // Extract glyph array
        if (line.find("Glyphs[]") != std::string::npos) {
            inGlyph = true;
            continue;
        }
        if (inGlyph && line.find("};") != std::string::npos) {
            inGlyph = false;
        }
        if (inGlyph) {
            GFXglyph g{};
            if (parseGlyphLine(line, g)) {
                glyphList.push_back(g);
            }
        }

        // Extract character subset string
        size_t q1 = line.find("\"");
        size_t q2 = line.find("\"", q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
            subset = line.substr(q1 + 1, q2 - q1 - 1);
        }
    }

    f.close();

    // Validate parsed data
    if (bitmapData.empty() || glyphList.empty() || subset.empty()) {
        std::cerr << "Error: Failed to parse font file" << std::endl;
        return 1;
    }

    std::cout << "Enter HEX code (without 0x, q to quit): " << std::flush;

    std::string input;
    while (true) {
        std::cout << "Enter HEX code: ";
        std::cin >> input;

        if (input == "q" || input == "Q") {
            std::cout << "Exit preview." << std::endl;
            break;
        }

        uint8_t targetChar = static_cast<uint8_t>(strtoul(input.c_str(), nullptr, 16));
        int index = -1;

        for (size_t i = 0; i < subset.size(); i++) {
            if (static_cast<uint8_t>(subset[i]) == targetChar) {
                index = static_cast<int>(i);
                break;
            }
        }

        if (index == -1 || index >= static_cast<int>(glyphList.size())) {
            std::cout << "Error: Character 0x" << input << " is not included in this font.\n" << std::endl;
            continue;
        }

        char c = subset[index];
        printf("\nCharacter: '%c' (0x%02X)\n", c, targetChar);
        printGlyphInTerminal(glyphList[index], bitmapData);
    }

    return 0;
}