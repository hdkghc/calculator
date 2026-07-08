/** @file /font2h/preview.cpp
 *  @brief Font preview tool - renders all glyphs in an OTF/TTF font to PPM
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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <ft2build.h>
#include FT_FREETYPE_H

constexpr int CHARS_PER_ROW = 16;
constexpr int CELL_PADDING  = 6;
constexpr int LABEL_HEIGHT  = 28;       ///< 2 lines of 5x7 labels
constexpr int HEADER_HEIGHT = 34;

struct Image {
    int w, h;
    std::vector<uint8_t> px;

    Image(int w_, int h_) : w(w_), h(h_), px(w_ * h_ * 3, 255) {}

    void set(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || x >= w || y < 0 || y >= h) return;
        int i = (y * w + x) * 3;
        px[i] = r; px[i+1] = g; px[i+2] = b;
    }

    void rect(int x, int y, int rw, int rh, uint8_t r, uint8_t g, uint8_t b) {
        for (int dy = 0; dy < rh; dy++)
            for (int dx = 0; dx < rw; dx++)
                set(x + dx, y + dy, r, g, b);
    }

    void hline(int x1, int x2, int y, uint8_t r, uint8_t g, uint8_t b) {
        for (int x = x1; x <= x2; x++) set(x, y, r, g, b);
    }

    void vline(int x, int y1, int y2, uint8_t r, uint8_t g, uint8_t b) {
        for (int y = y1; y <= y2; y++) set(x, y, r, g, b);
    }

    void bitmap(int x, int y, const uint8_t *bm, int bw, int bh, int pitch,
                uint8_t r, uint8_t g, uint8_t b) {
        for (int dy = 0; dy < bh; dy++)
            for (int dx = 0; dx < bw; dx++)
                if (bm[dy * pitch + dx] > 127)
                    set(x + dx, y + dy, r, g, b);
    }

    /**
     * @brief 5x7 pixel font (same as before, full ASCII coverage)
     */
    void draw_char_5x7(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b) {
        static const uint8_t FONT[][7] = {
            {0x70,0x88,0x98,0xA8,0xC8,0x88,0x70}, // 0 00
            {0x20,0x60,0x20,0x20,0x20,0x20,0x70}, // 1 01
            {0x70,0x88,0x08,0x10,0x20,0x40,0xF8}, // 2 02
            {0x70,0x88,0x08,0x30,0x08,0x88,0x70}, // 3 03
            {0x10,0x30,0x50,0x90,0xF8,0x10,0x10}, // 4 04
            {0xF8,0x80,0xF0,0x08,0x08,0x88,0x70}, // 5 05
            {0x70,0x80,0xF0,0x88,0x88,0x88,0x70}, // 6 06
            {0xF8,0x08,0x10,0x20,0x40,0x40,0x40}, // 7 07
            {0x70,0x88,0x88,0x70,0x88,0x88,0x70}, // 8 08
            {0x70,0x88,0x88,0x78,0x08,0x10,0x60}, // 9 09
            {0x70,0x88,0x88,0xF8,0x88,0x88,0x88}, // A 10
            {0xF0,0x88,0x88,0xF0,0x88,0x88,0xF0}, // B 11
            {0x70,0x88,0x80,0x80,0x80,0x88,0x70}, // C 12
            {0xE0,0x90,0x88,0x88,0x88,0x90,0xE0}, // D 13
            {0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8}, // E 14
            {0xF8,0x80,0x80,0xF0,0x80,0x80,0x80}, // F 15
            {0x70,0x88,0x80,0xB8,0x88,0x88,0x70}, // G 16
            {0x88,0x88,0x88,0xF8,0x88,0x88,0x88}, // H 17
            {0x70,0x20,0x20,0x20,0x20,0x20,0x70}, // I 18
            {0x38,0x10,0x10,0x10,0x10,0x90,0x60}, // J 19
            {0x88,0x90,0xA0,0xC0,0xA0,0x90,0x88}, // K 20
            {0x80,0x80,0x80,0x80,0x80,0x80,0xF8}, // L 21
            {0x88,0xD8,0xA8,0xA8,0x88,0x88,0x88}, // M 22
            {0x88,0xC8,0xA8,0x98,0x88,0x88,0x88}, // N 23
            {0x70,0x88,0x88,0x88,0x88,0x88,0x70}, // O 24
            {0xF0,0x88,0x88,0xF0,0x80,0x80,0x80}, // P 25
            {0x70,0x88,0x88,0x88,0xA8,0x90,0x68}, // Q 26
            {0xF0,0x88,0x88,0xF0,0xA0,0x90,0x88}, // R 27
            {0x70,0x88,0x80,0x70,0x08,0x88,0x70}, // S 28
            {0xF8,0x20,0x20,0x20,0x20,0x20,0x20}, // T 29
            {0x88,0x88,0x88,0x88,0x88,0x88,0x70}, // U 30
            {0x88,0x88,0x88,0x88,0x50,0x50,0x20}, // V 31
            {0x88,0x88,0x88,0xA8,0xA8,0xD8,0x88}, // W 32
            {0x88,0x88,0x50,0x20,0x50,0x88,0x88}, // X 33
            {0x88,0x88,0x50,0x20,0x20,0x20,0x20}, // Y 34
            {0xF8,0x08,0x10,0x20,0x40,0x80,0xF8}, // Z 35
            {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space 36
            {0x00,0x00,0x00,0xF8,0x00,0x00,0x00}, // - 37
            {0x00,0x00,0x00,0x00,0x00,0x60,0x60}, // . 38
            {0x00,0x60,0x60,0x00,0x60,0x60,0x00}, // : 39
            {0x00,0x20,0x20,0xF8,0x20,0x20,0x00}, // + 40
            {0x20,0x20,0x40,0x00,0x00,0x00,0x00}, // ' 41
            {0x10,0x20,0x40,0x40,0x40,0x20,0x10}, // ( 42
            {0x40,0x20,0x10,0x10,0x10,0x20,0x40}, // ) 43
            {0x70,0x40,0x40,0x40,0x40,0x40,0x70}, // [ 44
            {0x70,0x10,0x10,0x10,0x10,0x10,0x70}, // ] 45
            {0x08,0x10,0x10,0x20,0x20,0x40,0x80}, // / 46
            {0x00,0x00,0xF8,0x00,0xF8,0x00,0x00}, // = 47
            {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space 48 (for #)
        };

        int idx = 48;  // default: space
        if (c >= '0' && c <= '9') idx = c - '0';
        else if (c >= 'A' && c <= 'F') idx = 10 + (c - 'A');
        else if (c >= 'G' && c <= 'L') idx = 16 + (c - 'G');
        else if (c >= 'M' && c <= 'R') idx = 22 + (c - 'M');
        else if (c >= 'S' && c <= 'X') idx = 28 + (c - 'S');
        else if (c == 'Y') idx = 34;
        else if (c == 'Z') idx = 35;
        else if (c == ' ') idx = 36;
        else if (c == '-') idx = 37;
        else if (c == '.') idx = 38;
        else if (c == ':') idx = 39;
        else if (c == '+') idx = 40;
        else if (c == '\'') idx = 41;
        else if (c == '(') idx = 42;
        else if (c == ')') idx = 43;
        else if (c == '[') idx = 44;
        else if (c == ']') idx = 45;
        else if (c == '/') idx = 46;
        else if (c == '=') idx = 47;
        else if (c == '#') idx = 48;
        else return;

        for (int dy = 0; dy < 7; dy++) {
            uint8_t row = FONT[idx][dy];
            for (int dx = 0; dx < 5; dx++)
                if (row & (0x80 >> dx))
                    set(x + dx, y + dy, r, g, b);
        }
    }

    void draw_text_5x7(int x, int y, const std::string &s, uint8_t r, uint8_t g, uint8_t b) {
        for (size_t i = 0; i < s.size(); i++)
            draw_char_5x7(x + i * 6, y, s[i], r, g, b);
    }

    void draw_text_right_5x7(int x, int y, const std::string &s, uint8_t r, uint8_t g, uint8_t b) {
        draw_text_5x7(x - (int)s.size() * 6, y, s, r, g, b);
    }

    void draw_unicode_label(int x, int y, uint32_t code, uint8_t r, uint8_t g, uint8_t b) {
        char buf[12];
        if (code <= 0xFFFF)
            snprintf(buf, sizeof(buf), "U+%04X", (unsigned)code);
        else
            snprintf(buf, sizeof(buf), "U+%06X", (unsigned)code);
        draw_text_5x7(x, y, buf, r, g, b);
    }

    void draw_index_label(int x, int y, int idx, uint8_t r, uint8_t g, uint8_t b) {
        char buf[12];
        snprintf(buf, sizeof(buf), "#%d", idx);
        draw_text_5x7(x, y, buf, r, g, b);
    }
};

void save_ppm(const std::string &path, const Image &img) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << img.w << " " << img.h << "\n255\n";
    f.write((const char*)img.px.data(), img.px.size());
    std::cout << "Saved: " << path << " (" << img.w << "x" << img.h << ")\n";
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <font.otf> [size] [output.ppm]\n";
        return 1;
    }

    std::string font_path = argv[1];
    int         font_size = (argc >= 3) ? std::stoi(argv[2]) : 12;
    std::string out_path  = (argc >= 4) ? argv[3] : "preview.ppm";

    FT_Library ft;
    FT_Face    face;
    if (FT_Init_FreeType(&ft) || FT_New_Face(ft, font_path.c_str(), 0, &face)) {
        std::cerr << "Error: Cannot open font\n";
        return 1;
    }
    FT_Set_Pixel_Sizes(face, 0, font_size);

    std::string family = face->family_name ? face->family_name : "Unknown";
    std::string style  = face->style_name  ? face->style_name  : "";
    int num_glyphs     = face->num_glyphs;
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);

    std::cout << "Font: " << family << " " << style << " (" << font_size << "px)\n";
    std::cout << "Glyphs: " << num_glyphs << "\n";

    struct Glyph {
        uint32_t code;
        bool     valid;
        int      w, h, pitch, left, top;
        std::vector<uint8_t> data;
    };
    std::vector<Glyph> cache;

    FT_UInt  gidx;
    FT_ULong charcode = FT_Get_First_Char(face, &gidx);
    while (gidx != 0) {
        Glyph g;
        g.code  = charcode;
        g.valid = false;
        if (FT_Load_Glyph(face, gidx, FT_LOAD_RENDER) == 0) {
            FT_GlyphSlot slot = face->glyph;
            g.valid = true;
            g.w     = slot->bitmap.width;
            g.h     = slot->bitmap.rows;
            g.pitch = slot->bitmap.pitch;
            g.left  = slot->bitmap_left;
            g.top   = slot->bitmap_top;
            g.data.assign(slot->bitmap.buffer,
                          slot->bitmap.buffer + slot->bitmap.rows * slot->bitmap.pitch);
        }
        cache.push_back(g);
        charcode = FT_Get_Next_Char(face, charcode, &gidx);
    }

    int total = cache.size();
    std::cout << "Mapped: " << total << " characters\n";

    std::sort(cache.begin(), cache.end(),
              [](const Glyph &a, const Glyph &b) { return a.code < b.code; });

    int max_w = 0, max_h = 0, max_top = 0;
    for (const auto &g : cache) {
        if (g.w > max_w) max_w = g.w;
        if (g.h > max_h) max_h = g.h;
        if (g.top > max_top) max_top = g.top;
    }

    int cell_w = max_w + CELL_PADDING * 2;
    // Make cell wide enough for labels (U+XXXX = 6 chars * 6px = 36px, #123 = 4*6=24px)
    if (cell_w < 42) cell_w = 42;
    int cell_h = max_h + CELL_PADDING * 2 + LABEL_HEIGHT;
    int rows   = (total + CHARS_PER_ROW - 1) / CHARS_PER_ROW;
    int img_w  = CHARS_PER_ROW * cell_w + 1;
    int img_h  = HEADER_HEIGHT + rows * cell_h + 1;
    Image img(img_w, img_h);

    // --- Header ---
    img.rect(0, 0, img_w, HEADER_HEIGHT, 25, 25, 35);
    char info[80];
    snprintf(info, sizeof(info), "%s %s  %dpx  %d glyphs  %d mapped  U+%04X-%06X",
             family.c_str(), style.c_str(), font_size, num_glyphs, total,
             cache.front().code, cache.back().code);
    img.draw_text_5x7(4, 4, info, 200, 210, 230);
    img.draw_text_5x7(4, 16, "white=glyph  red=missing  blue=control", 160, 170, 190);

    // --- Grid ---
    for (int i = 0; i < total; i++) {
        const auto &g = cache[i];
        int col = i % CHARS_PER_ROW;
        int row = i / CHARS_PER_ROW;
        int cx  = col * cell_w;
        int cy  = HEADER_HEIGHT + row * cell_h;

        // Background
        if (g.valid && g.w > 0)
            img.rect(cx, cy, cell_w, cell_h, (row%2)?255:250, (row%2)?255:250, (row%2)?255:250);
        else
            img.rect(cx, cy, cell_w, cell_h, 255, 220, 220);
        if (g.code < 0x20 || (g.code >= 0x7F && g.code < 0xA0))
            img.rect(cx, cy, cell_w, cell_h, 225, 230, 255);

        // Glyph (centered horizontally in cell)
        if (g.valid && g.w > 0) {
            int gx = cx + (cell_w - g.w) / 2 + g.left;
            int gy = cy + CELL_PADDING + (max_top - g.top);
            img.bitmap(gx, gy, g.data.data(), g.w, g.h, g.pitch, 0, 0, 0);
        }

        // Labels: line 1 = U+XXXX, line 2 = #index
        int lx = cx + 4;
        int ly = cy + CELL_PADDING + max_h + 2;
        uint8_t lr = g.valid ? 70 : 200;
        uint8_t lg_ = g.valid ? 70 : 0;
        uint8_t lb = g.valid ? 70 : 0;
        img.draw_unicode_label(lx, ly, g.code, lr, lg_, lb);
        img.draw_index_label(lx, ly + 8, i, 120, 120, 130);
    }

    // --- Grid lines ---
    for (int r = 0; r <= rows; r++)
        img.hline(0, img_w-1, HEADER_HEIGHT + r*cell_h, 190, 190, 190);
    for (int c = 0; c <= CHARS_PER_ROW; c++)
        img.vline(c*cell_w, HEADER_HEIGHT, img_h-1, 190, 190, 190);

    // --- Baseline dots ---
    for (int r = 0; r < rows; r++) {
        int y = HEADER_HEIGHT + CELL_PADDING + max_top + r*cell_h;
        for (int x = 0; x < img_w; x += 6) img.set(x, y, 255, 60, 60);
    }

    // --- Row range labels ---
    for (int r = 0; r < rows; r++) {
        int first = r * CHARS_PER_ROW;
        int last  = std::min(total-1, (r+1)*CHARS_PER_ROW-1);
        int y = HEADER_HEIGHT + r*cell_h + cell_h - 2;
        char buf[24];
        if (cache[first].code == cache[last].code)
            snprintf(buf, sizeof(buf), "U+%04X", cache[first].code);
        else
            snprintf(buf, sizeof(buf), "U+%04X-%04X", cache[first].code, cache[last].code);
        img.draw_text_right_5x7(cell_w*CHARS_PER_ROW-2, y, buf, 150, 150, 160);
    }

    save_ppm(out_path, img);

    int missing = 0;
    for (const auto &g : cache)
        if (!g.valid || g.w == 0) missing++;
    std::cout << "Missing: " << missing << "\n";
    std::cout << "Grid: " << CHARS_PER_ROW << "x" << rows << "\n";
    std::cout << "Max glyph: " << max_w << "x" << max_h << "\n";
    std::cout << "Cell: " << cell_w << "x" << cell_h << "\n";

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return 0;
}