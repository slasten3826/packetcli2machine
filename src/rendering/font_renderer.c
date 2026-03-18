#include "packet_adventure/font_renderer.h"

#include "packet_adventure/render_primitives.h"

typedef struct {
    char ch;
    const char *rows[7];
} PaGlyph;

typedef struct {
    char ch;
    const char *rows[5];
} PaGlyphSmall;

static const PaGlyph k_glyphs[] = {
    {'A', {" ### ","#   #","#   #","#####","#   #","#   #","#   #"}},
    {'B', {"#### ","#   #","#   #","#### ","#   #","#   #","#### "}},
    {'C', {" ####","#    ","#    ","#    ","#    ","#    "," ####"}},
    {'D', {"#### ","#   #","#   #","#   #","#   #","#   #","#### "}},
    {'E', {"#####","#    ","#    ","#### ","#    ","#    ","#####"}},
    {'F', {"#####","#    ","#    ","#### ","#    ","#    ","#    "}},
    {'G', {" ####","#    ","#    ","#  ##","#   #","#   #"," ####"}},
    {'H', {"#   #","#   #","#   #","#####","#   #","#   #","#   #"}},
    {'I', {"#####","  #  ","  #  ","  #  ","  #  ","  #  ","#####"}},
    {'K', {"#   #","#  # ","# #  ","##   ","# #  ","#  # ","#   #"}},
    {'M', {"#   #","## ##","# # #","#   #","#   #","#   #","#   #"}},
    {'N', {"#   #","##  #","##  #","# # #","#  ##","#  ##","#   #"}},
    {'L', {"#    ","#    ","#    ","#    ","#    ","#    ","#####"}},
    {'O', {" ### ","#   #","#   #","#   #","#   #","#   #"," ### "}},
    {'P', {"#### ","#   #","#   #","#### ","#    ","#    ","#    "}},
    {'R', {"#### ","#   #","#   #","#### ","# #  ","#  # ","#   #"}},
    {'S', {" ####","#    ","#    "," ### ","    #","    #","#### "}},
    {'T', {"#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  "}},
    {'U', {"#   #","#   #","#   #","#   #","#   #","#   #"," ### "}},
    {'V', {"#   #","#   #","#   #","#   #"," # # "," # # ","  #  "}},
    {'W', {"#   #","#   #","#   #","# # #","# # #","## ##","#   #"}},
    {'0', {" ### ","#   #","#  ##","# # #","##  #","#   #"," ### "}},
    {'1', {"  #  "," ##  ","  #  ","  #  ","  #  ","  #  ","#####"}},
    {'2', {" ### ","#   #","    #","   # ","  #  "," #   ","#####"}},
    {'3', {" ### ","#   #","    #"," ### ","    #","#   #"," ### "}},
    {'4', {"#   #","#   #","#   #","#####","    #","    #","    #"}},
    {'5', {"#####","#    ","#    ","#### ","    #","    #","#### "}},
    {'6', {" ####","#    ","#    ","#### ","#   #","#   #"," ### "}},
    {'7', {"#####","    #","   # ","  #  "," #   ","#    ","#    "}},
    {'8', {" ### ","#   #","#   #"," ### ","#   #","#   #"," ### "}},
    {'9', {" ### ","#   #","#   #"," ####","    #","    #"," ### "}},
    {' ', {"     ","     ","     ","     ","     ","     ","     "}},
    {'-', {"     ","     ","     ","#####","     ","     ","     "}},
    {'>', {"#    "," #   ","  #  ","   # ","  #  "," #   ","#    "}},
    {':', {"     "," ##  "," ##  ","     "," ##  "," ##  ","     "}},
};

static const PaGlyphSmall k_small_glyphs[] = {
    {'A', {" # ","# #","###","# #","# #"}},
    {'B', {"## ","# #","## ","# #","## "}},
    {'C', {" ##","#  ","#  ","#  "," ##"}},
    {'D', {"## ","# #","# #","# #","## "}},
    {'E', {"###","#  ","## ","#  ","###"}},
    {'F', {"###","#  ","## ","#  ","#  "}},
    {'G', {" ##","#  ","# #","# #"," ##"}},
    {'H', {"# #","# #","###","# #","# #"}},
    {'I', {"###"," # "," # "," # ","###"}},
    {'K', {"# #","## ","#  ","## ","# #"}},
    {'L', {"#  ","#  ","#  ","#  ","###"}},
    {'M', {"# #","###","###","# #","# #"}},
    {'N', {"# #","###","###","###","# #"}},
    {'O', {"###","# #","# #","# #","###"}},
    {'P', {"## ","# #","## ","#  ","#  "}},
    {'R', {"## ","# #","## ","## ","# #"}},
    {'S', {" ##","#  ","###","  #","## "}},
    {'T', {"###"," # "," # "," # "," # "}},
    {'U', {"# #","# #","# #","# #","###"}},
    {'V', {"# #","# #","# #","# #"," # "}},
    {'W', {"# #","# #","###","###","# #"}},
    {'Y', {"# #","# #"," # "," # "," # "}},
    {'0', {"###","# #","# #","# #","###"}},
    {'1', {" # ","## "," # "," # ","###"}},
    {'2', {"## ","  #","###","#  ","###"}},
    {'3', {"## ","  #","## ","  #","## "}},
    {'4', {"# #","# #","###","  #","  #"}},
    {'5', {"###","#  ","## ","  #","## "}},
    {' ', {"   ","   ","   ","   ","   "}},
    {'>', {"#  "," # ","  #"," # ","#  "}},
};

static const char *const *pa_find_rows(char ch) {
    unsigned int i;
    for (i = 0; i < sizeof(k_glyphs) / sizeof(k_glyphs[0]); ++i) {
        if (k_glyphs[i].ch == ch) {
            return k_glyphs[i].rows;
        }
    }
    return k_glyphs[sizeof(k_glyphs) / sizeof(k_glyphs[0]) - 1].rows;
}

static const char *const *pa_find_small_rows(char ch) {
    unsigned int i;
    for (i = 0; i < sizeof(k_small_glyphs) / sizeof(k_small_glyphs[0]); ++i) {
        if (k_small_glyphs[i].ch == ch) {
            return k_small_glyphs[i].rows;
        }
    }
    return k_small_glyphs[sizeof(k_small_glyphs) / sizeof(k_small_glyphs[0]) - 1].rows;
}

void pa_draw_text(PaSurface *surface, int x, int y, const char *text, uint32_t color, int scale) {
    int cursor = x;
    int row;
    int col;

    if (scale < 1) {
        scale = 1;
    }

    while (*text != '\0') {
        const char *const *rows = pa_find_rows(*text);
        for (row = 0; row < 7; ++row) {
            for (col = 0; col < 5; ++col) {
                if (rows[row][col] != ' ') {
                    pa_fill_rect(surface,
                        (PaRect){cursor + (col * scale), y + (row * scale), scale, scale},
                        color);
                }
            }
        }
        cursor += 6 * scale;
        text += 1;
    }
}

int pa_text_width(const char *text, int scale) {
    int count = 0;
    if (scale < 1) {
        scale = 1;
    }
    while (*text != '\0') {
        count += 1;
        text += 1;
    }
    if (count == 0) {
        return 0;
    }
    return count * 6 * scale;
}

void pa_draw_text_small(PaSurface *surface, int x, int y, const char *text, uint32_t color) {
    int cursor = x;
    int row;
    int col;

    while (*text != '\0') {
        const char *const *rows = pa_find_small_rows(*text);
        for (row = 0; row < 5; ++row) {
            for (col = 0; col < 3; ++col) {
                if (rows[row][col] != ' ') {
                    pa_fill_rect(surface, (PaRect){cursor + col, y + row, 1, 1}, color);
                }
            }
        }
        cursor += 4;
        text += 1;
    }
}

int pa_text_small_width(const char *text) {
    int count = 0;
    while (*text != '\0') {
        count += 1;
        text += 1;
    }
    if (count == 0) {
        return 0;
    }
    return count * 4;
}
