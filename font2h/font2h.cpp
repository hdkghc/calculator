/** @file /font2h/font2h.cpp
 *  @brief Font transformation tool
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
#include <map>
#include <cctype>
#include <cstring>
#include <ft2build.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include FT_FREETYPE_H

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_ORANGE  "\033[38;5;214m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"

typedef struct {
    uint16_t bitmapOffset;
    uint8_t  width;
    uint8_t  height;
    uint8_t  xAdvance;
    int8_t   xOffset;
    int8_t   yOffset;
} GFXglyph;

typedef struct {
    uint8_t  *bitmap;
    GFXglyph *glyph;
    uint16_t first;
    uint16_t last;
    uint8_t  yAdvance;
    const char *subset;
} GFXfont;

#define CHECK_FT(rc, msg) if (rc) { \
    std::cerr << COLOR_RED << "Error: " << msg << " (" << rc << ")" << COLOR_RESET << "\n"; \
    return 1; \
}

static auto start_time = std::chrono::steady_clock::now();

static void log_info(const std::string &msg) {
    auto now = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
    std::cout << COLOR_ORANGE << "[" << std::setw(10) << us << "] " << COLOR_GREEN << "info" << COLOR_RESET 
              << " " << msg << "\n";
}

static void log_gen(const std::string &msg) {
    auto now = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
    std::cout << COLOR_ORANGE << "[" << std::setw(10) << us << "] " << COLOR_GREEN << "gen" << COLOR_RESET 
              << "  " << msg << "\n";
}

static void log_skip(const std::string &msg) {
    auto now = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
    std::cout << COLOR_ORANGE << "[" << std::setw(10) << us << "] " << COLOR_GREEN << "skip" << COLOR_RESET 
              << " " << msg << "\n";
}

static void log_progress(int done, int total) {
    auto now = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
    std::cout << COLOR_ORANGE << "[" << std::setw(10) << us << "] " << COLOR_GREEN << "prog" << COLOR_RESET 
              << " " << done << "/" << total << " (" << (100 * done / total) << "%)\r" << std::flush;
}

std::string get_safe_font_name(FT_Face face) {
    std::string name = face->family_name ? face->family_name : "CustomFont";
    std::string style = face->style_name ? face->style_name : "";
    if (!style.empty()) name += "_" + style;
    for (char &c : name) {
        if (!isalnum(static_cast<unsigned char>(c))) c = '_';
    }
    return name;
}

std::string build_subset(const std::map<uint32_t, bool> &chars) {
    std::string s;
    for (const auto &p : chars) {
        uint32_t c = p.first;
        if (c <= 0xFFFF && !(c >= 0xD800 && c <= 0xDFFF)) {
            if (c <= 0x7F) {
                s += static_cast<char>(c);
            } else if (c <= 0x7FF) {
                s += static_cast<char>(0xC0 | (c >> 6));
                s += static_cast<char>(0x80 | (c & 0x3F));
            } else {
                s += static_cast<char>(0xE0 | (c >> 12));
                s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                s += static_cast<char>(0x80 | (c & 0x3F));
            }
        }
    }
    return s;
}

std::string format_size(uint32_t bytes) {
    if (bytes >= 1024 * 1024) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        return ss.str();
    }
    if (bytes >= 1024) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        return ss.str();
    }
    return std::to_string(bytes) + " bytes";
}

/**
 * @brief Generate a solid filled box bitmap
 * @param w  Width in pixels
 * @param h  Height in pixels
 * @return   Packed 1bpp MSB-first bitmap vector
 */
static std::vector<uint8_t> make_solid_box(int w, int h) {
    int bytes_per_row = (w + 7) / 8;
    std::vector<uint8_t> buf(bytes_per_row * h, 0);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            buf[y * bytes_per_row + x / 8] |= (0x80 >> (x % 8));
        }
    }
    return buf;
}

void generate_header(
    const std::string &font_name, int font_size,
    const std::vector<uint8_t> &bitmaps,
    const std::vector<GFXglyph> &glyphs,
    const std::map<uint32_t, bool> &char_map,
    const std::string &out_path
) {
    uint32_t bm_sz = bitmaps.size();
    uint32_t gl_sz = glyphs.size() * sizeof(GFXglyph);
    uint32_t font_sz = sizeof(GFXfont);
    uint32_t total_sz = bm_sz + gl_sz + font_sz;
    
    std::ofstream f(out_path);
    if (!f) {
        std::cerr << COLOR_RED << "Error: Cannot open " << out_path << COLOR_RESET << "\n";
        return;
    }
    std::stringstream ss;

    std::string var = font_name + std::to_string(font_size) + "pt";
    std::string subset = build_subset(char_map);
    uint16_t first = static_cast<uint16_t>(char_map.begin()->first);
    uint16_t last  = static_cast<uint16_t>(char_map.rbegin()->first);
    
    uint8_t y_adv = font_size;
    for (const auto &g : glyphs) {
        if (g.height > y_adv) y_adv = g.height;
    }

    ss << "const uint8_t " << var << "Bitmaps[] PROGMEM = {\n  ";
    for (size_t i = 0; i < bitmaps.size(); ++i) {
        ss << "0x" << std::hex << std::uppercase;
        if (bitmaps[i] < 0x10) ss << "0";
        ss << static_cast<int>(bitmaps[i]) << std::dec;
        if (i != bitmaps.size() - 1) ss << ", ";
        if ((i + 1) % 12 == 0) ss << "\n  ";
    }
    ss << "\n};\n\n";

    ss << "const GFXglyph " << var << "Glyphs[] PROGMEM = {\n";
    int idx = 0;
    for (const auto &p : char_map) {
        const GFXglyph &g = glyphs[idx++];
        ss << "  { " << g.bitmapOffset << ", "
           << static_cast<int>(g.width) << ", "
           << static_cast<int>(g.height) << ", "
           << static_cast<int>(g.xAdvance) << ", "
           << static_cast<int>(g.xOffset) << ", "
           << static_cast<int>(g.yOffset) << " }, ";
        ss << "// U+" << std::hex << std::uppercase;
        if (p.first < 0x1000) ss << "0";
        if (p.first < 0x0100) ss << "0";
        if (p.first < 0x0010) ss << "0";
        ss << p.first << std::dec;
        if (p.first >= 0x20 && p.first <= 0x7E) ss << " '" << static_cast<char>(p.first) << "'";
        ss << "\n";
    }
    ss << "};\n\n";

    ss << "const GFXfont " << var << " PROGMEM = {\n";
    ss << "  (uint8_t  *)" << var << "Bitmaps,\n";
    ss << "  (GFXglyph *)" << var << "Glyphs,\n";
    ss << "  0x" << std::hex << std::uppercase << first << ", 0x" << last << std::dec << ", "
       << static_cast<int>(y_adv) << ",\n";
    std::string chset_esc;
    for (auto u : subset) {
        if (u == '\\') chset_esc += "\\\\";
        else if (u == '"') chset_esc += "\\\"";
        else if (u == '\n') chset_esc += "\\n";
        else if (u == '\r') chset_esc += "\\r";
        else if (u == '\t') chset_esc += "\\t";
        else if (static_cast<unsigned char>(u) < 0x20) {
            chset_esc += "\\x";
            chset_esc += "0123456789ABCDEF"[(unsigned char)u >> 4];
            chset_esc += "0123456789ABCDEF"[(unsigned char)u & 0x0F];
        } else {
            chset_esc += u;
        }
    }
    ss << "  \"" << chset_esc << "\"\n";
    ss << "};\n\n";
    
    ss << "// Memory usage:\n";
    ss << "//   Bitmaps : " << format_size(bm_sz) << "\n";
    ss << "//   Glyphs  : " << format_size(gl_sz) << "\n";
    ss << "//   Font    : " << format_size(font_sz) << "\n";
    ss << "//   Total   : " << format_size(total_sz) << "\n";
    
    f << ss.str();
    f.close();

    log_gen("Output  " + out_path);
    log_info("Glyphs  " + std::to_string(char_map.size()));
    log_info("Size    " + format_size(total_sz));
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <font.otf/ttf> <output.h> [size]\n";
        return 1;
    }

    std::string font_path = argv[1];
    std::string out_path = argv[2];
    int font_size = (argc >= 4) ? std::stoi(argv[3]) : 12;

    log_info("Font    " + font_path);
    log_info("Size    " + std::to_string(font_size) + "px");

    FT_Library ft;
    FT_Face face;
    CHECK_FT(FT_Init_FreeType(&ft), "FreeType init");
    CHECK_FT(FT_New_Face(ft, font_path.c_str(), 0, &face), "Open font");
    CHECK_FT(FT_Set_Pixel_Sizes(face, 0, font_size), "Set font size");

    std::string font_name = get_safe_font_name(face);
    log_info("Name    " + font_name);

    // Collect all glyphs present in the font (Unicode)
    std::map<uint32_t, bool> char_map;
    FT_UInt gindex;
    FT_ULong charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0) {
        char_map[charcode] = true;
        charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    // Ensure all ASCII characters (0x00-0x7F) are present
    for (uint32_t c = 0x00; c <= 0x7F; c++) {
        if (char_map.find(c) == char_map.end()) {
            char_map[c] = true; // will be rendered as solid box
        }
    }

    int total = char_map.size();
    log_info("Chars   " + std::to_string(total));

    // Box dimensions for missing glyphs
    const int BOX_W = font_size * 3 / 4;
    const int BOX_H = font_size;
    std::vector<uint8_t> box_bitmap = make_solid_box(BOX_W, BOX_H);

    std::vector<uint8_t> bitmaps;
    std::vector<GFXglyph> glyphs;
    uint16_t offset = 0;
    int done = 0;

    for (const auto &p : char_map) {
        uint32_t c = p.first;

        log_progress(done, total);

        FT_UInt gidx = FT_Get_Char_Index(face, c);

        // Missing glyph or load failure → solid box
        if (gidx == 0 || FT_Load_Glyph(face, gidx, FT_LOAD_RENDER)) {
            GFXglyph box{};
            box.bitmapOffset = offset;
            box.width  = BOX_W;
            box.height = BOX_H;
            box.xAdvance = BOX_W;
            box.xOffset  = 0;
            box.yOffset  = -BOX_H;

            bitmaps.insert(bitmaps.end(), box_bitmap.begin(), box_bitmap.end());
            glyphs.push_back(box);
            offset += static_cast<uint16_t>(box_bitmap.size());
            done++;

            std::stringstream ss;
            ss << "U+" << std::hex << std::uppercase;
            if (c < 0x1000) ss << "0";
            if (c < 0x0100) ss << "0";
            if (c < 0x0010) ss << "0";
            ss << c << std::dec;
            if (c >= 0x20 && c <= 0x7E) ss << " '" << static_cast<char>(c) << "'";
            ss << " -> box";
            log_skip(ss.str());
            continue;
        }

        FT_GlyphSlot g = face->glyph;
        int w = g->bitmap.width;
        int h = g->bitmap.rows;
        int bytes_per_row = (w + 7) / 8;
        std::vector<uint8_t> buf(bytes_per_row * h, 0);

        // Pack grayscale bitmap to 1bpp MSB-first
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (g->bitmap.buffer[y * g->bitmap.pitch + x] > 127) {
                    buf[y * bytes_per_row + x / 8] |= (0x80 >> (x % 8));
                }
            }
        }

        GFXglyph glyph{};
        glyph.bitmapOffset = offset;
        glyph.width = w;
        glyph.height = h;
        glyph.xAdvance = g->metrics.horiAdvance / 64;
        if (glyph.xAdvance == 0) glyph.xAdvance = w + 1;
        glyph.xOffset = g->bitmap_left;
        glyph.yOffset = -g->bitmap_top;

        bitmaps.insert(bitmaps.end(), buf.begin(), buf.end());
        glyphs.push_back(glyph);
        offset += static_cast<uint16_t>(buf.size());
        done++;

        std::stringstream ss;
        ss << "U+" << std::hex << std::uppercase;
        if (c < 0x1000) ss << "0";
        if (c < 0x0100) ss << "0";
        if (c < 0x0010) ss << "0";
        ss << c << std::dec;
        if (c >= 0x20 && c <= 0x7E) ss << " '" << static_cast<char>(c) << "'";
        ss << " off=" << glyph.bitmapOffset
           << " w=" << w
           << " h=" << h
           << " sz=" << buf.size()
           << " adv=" << static_cast<int>(glyph.xAdvance);
        log_gen(ss.str());
    }

    std::cout << "\n";
    generate_header(font_name, font_size, bitmaps, glyphs, char_map, out_path);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    
    log_info("Done    " + std::to_string(glyphs.size()) + " characters");
    return 0;
}