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
#include FT_FREETYPE_H

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

#define CHECK_FT(rc, msg) if (rc) { std::cerr << "Error: " << msg << " (" << rc << ")\n"; return 1; }

std::string get_safe_font_name(FT_Face face) {
    std::string name = face->family_name ? face->family_name : "CustomFont";
    std::string style = face->style_name ? face->style_name : "";
    if (!style.empty()) name += "_" + style;
    for (char &c : name) {
        if (!isalnum(static_cast<unsigned char>(c))) c = '_';
    }
    return name;
}

void pack_bitmap(const uint8_t *src, uint8_t *dst, int w, int h) {
    int bits = 0;
    std::fill(dst, dst + (w * h + 7) / 8, 0);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (src[y * w + x] > 127) {
                dst[bits / 8] |= 1 << (7 - (bits % 8));
            }
            bits++;
        }
    }
}

std::string build_subset(const std::map<uint16_t, bool> &chars) {
    std::string s;
    for (const auto &p : chars) {
        uint16_t c = p.first;
        if (c >= 32 && c <= 126) s += static_cast<char>(c);
    }
    return s;
}

void generate_header(
    const std::string &font_name, int font_size,
    const std::vector<uint8_t> &bitmaps,
    const std::vector<GFXglyph> &glyphs,
    const std::map<uint16_t, bool> &char_map,
    const std::string &out_path
) {
    uint32_t sz = bitmaps.size() + glyphs.size() * sizeof(GFXglyph) + sizeof(GFXfont);
    std::ofstream f(out_path);
    if (!f) return;
    std::stringstream ss;

    std::string var = font_name + std::to_string(font_size) + "pt";
    std::string subset = build_subset(char_map);
    uint16_t first = char_map.begin()->first;
    uint16_t last = char_map.rbegin()->first;
    uint8_t y_adv = glyphs.empty() ? 0 : glyphs[0].xAdvance;

    ss << "const uint8_t " << var << "Bitmaps[] PROGMEM = {\n  ";
    for (size_t i = 0; i < bitmaps.size(); ++i) {
        ss << "0x";
        if (bitmaps[i] < 16) ss << "0";
        ss << std::hex << static_cast<int>(bitmaps[i]) << std::dec;
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
        ss << "// 0x" << std::hex << p.first << std::dec;
        if (p.first >= 32 && p.first <= 126) ss << " '" << static_cast<char>(p.first) << "'";
        ss << "\n";
    }
    ss << "};\n\n";

    ss << "const GFXfont " << var << " PROGMEM = {\n";
    ss << "  (uint8_t  *)" << var << "Bitmaps,\n";
    ss << "  (GFXglyph *)" << var << "Glyphs,\n";
    ss << "  0x" << std::hex << first << ", 0x" << last << std::dec << ", "
      << static_cast<int>(y_adv) << ",\n";
    std::string chset;
    for (auto u : subset) {
        if (u == '\\') chset += "\\\\";
        else if (u == '"') chset += "\\\"";
        else if (isprint(static_cast<unsigned char>(u))) chset += u;
        else {
            char buf[5];
            snprintf(buf, sizeof(buf), "\\x%02X", static_cast<unsigned char>(u));
            chset += buf;
        }
    }
    ss << "  \"" << chset << "\"\n";
    ss << "};\n\n";
    ss << "// Approx: " << sz << " bytes\n";
    f << ss.str();

    std::cout << "Generated: " << out_path << "\n";
    std::cout << "Characters: " << char_map.size() << "\n";
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <font.otf/ttf> <output.h> [size]\n";
        std::cout << "Example: " << argv[0] << " font.otf font.h 12\n";
        return 1;
    }

    std::string font_path = argv[1];
    std::string out_path = argv[2];
    int font_size = (argc >= 4) ? std::stoi(argv[3]) : 12;

    FT_Library ft;
    FT_Face face;
    CHECK_FT(FT_Init_FreeType(&ft), "Failed to initialize FreeType");
    CHECK_FT(FT_New_Face(ft, font_path.c_str(), 0, &face), "Failed to open font");
    CHECK_FT(FT_Set_Pixel_Sizes(face, 0, font_size), "Failed to set font size");

    std::string font_name = get_safe_font_name(face);
    std::map<uint16_t, bool> char_map;

    FT_UInt idx;
    FT_ULong code = FT_Get_First_Char(face, &idx);
    while (idx != 0) {
        if (code > 0 && code <= 0xFFFF) char_map[static_cast<uint16_t>(code)] = true;
        code = FT_Get_Next_Char(face, code, &idx);
    }

    std::vector<uint8_t> bitmaps;
    std::vector<GFXglyph> glyphs;
    uint16_t offset = 0;

    for (const auto &p : char_map) {
        uint16_t c = p.first;
        FT_UInt gidx = FT_Get_Char_Index(face, c);
        if (FT_Load_Glyph(face, gidx, FT_LOAD_RENDER | FT_LOAD_MONOCHROME)) continue;

        FT_GlyphSlot g = face->glyph;
        int w = g->bitmap.width;
        int h = g->bitmap.rows;
        std::vector<uint8_t> buf((w * h + 7) / 8);
        pack_bitmap(g->bitmap.buffer, buf.data(), w, h);

        GFXglyph glyph{};
        glyph.bitmapOffset = offset;
        glyph.width = w;
        glyph.height = h;
        glyph.xAdvance = g->metrics.horiAdvance / 64;
        glyph.xOffset = g->bitmap_left;
        glyph.yOffset = -g->bitmap_top;

        bitmaps.insert(bitmaps.end(), buf.begin(), buf.end());
        glyphs.push_back(glyph);
        offset += static_cast<uint16_t>(buf.size());
    }

    generate_header(font_name, font_size, bitmaps, glyphs, char_map, out_path);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return 0;
}