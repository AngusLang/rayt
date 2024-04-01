#include "ui/msdf_font.h"
#include "foundation/io.h"
#include "foundation/ustring.h"
#include "foundation/udata.h"
#include "gpu/gpu_const.h"
#include "os/os.h"

#include <stb_ds.h>

msdf_font *msdf_alloc() {
    msdf_font *font = (msdf_font *)malloc(sizeof(msdf_font));
    font->char_map = NULL;
    font->kerning_map = NULL;
    hmdefault(font->char_map, (msdf_glyph){0});
    hmdefault(font->kerning_map, 0);
    return font;
}

msdf_font *msdf_font_load(ustring json_path, ustring image_path) {
    msdf_font *font = msdf_alloc();
    return font;
}

float2 msdf_font_compute_size_and_offset(msdf_font *font, ustring_view text, f32 *offsets) {
    float2 size = (float2){.x = 0.f, .y = (f32)font->line_height};
    int prev_id = -1;
    for (int i = 0; i < text.length; ++i) {
        int char_code = (int)text.base.data[i];
        msdf_glyph g = msdf_font_get_glyph(font, char_code);
        if (g.id == 0)
            continue;
        size.x += (f32)g.xadvance + (f32)msdf_font_computer_kerning(font, prev_id, g.id);
        prev_id = g.id;
        if (offsets != NULL)
            offsets[i] = size.x;
    }
    return size;
}

msdf_glyph msdf_font_get_glyph(msdf_font *font, int index) { return hmget(font->char_map, index); }

int msdf_font_computer_kerning(msdf_font *font, int prev, int next) {
    const kerning_key key = (kerning_key){.first = prev, .second = next};
    return hmget(font->kerning_map, key);
}

static msdf_font system_font;
msdf_font *msdf_font_system_font() {
    static bool system_font_initialized = false;
    if (system_font_initialized)
        return &system_font;
    system_font_initialized = true;

    system_font.char_map = NULL;
    system_font.kerning_map = NULL;
    system_font.name = ustring_STR("Lato-Regular");
    system_font.line_height = 50;
    system_font.size = 42;
    system_font.texture_width = 512;
    system_font.texture_height = 256;
    int char_data[][9] = {
        87,  58, -2, 6,  43, 46, 34, 0,   0,   36,  7,  0,  2,  24, 24, 43, 0,   35,  40,  11, 1,  3,  13, 12, 43, 0,   79,
        41,  12, 0,  3,  13, 12, 43, 0,   123, 124, 95, 3,  4,  13, 7,  43, 0,   167, 91,  62, 1,  4,  13, 12, 42, 0,   211,
        93,  64, 0,  4,  13, 12, 42, 8,   167, 106, 77, -3, 5,  11, 13, 42, 13,  210, 123, 94, -1, 4,  13, 14, 42, 13,  79,
        125, 96, 0,  4,  13, 14, 42, 25,  35,  81,  52, 0,  6,  34, 35, 41, 13,  122, 64,  35, 0,  7,  35, 35, 38, 28,  78,
        47,  18, -2, 5,  16, 20, 37, 40,  35,  92,  63, -2, 5,  16, 20, 37, 21,  164, 119, 90, -2, 15, 32, 36, 25, 47,  0,
        37,  8,  0,  6,  33, 34, 35, 61,  26,  38,  9,  0,  6,  30, 32, 35, 27,  202, 48,  19, -1, 6,  24, 26, 35, 42,  164,
        51,  22, 0,  6,  24, 24, 35, 49,  117, 56,  27, 0,  6,  24, 24, 35, 64,  62,  63,  34, -1, 6,  17, 19, 35, 60,  200,
        67,  38, 0,  6,  29, 29, 35, 69,  153, 71,  42, 0,  6,  31, 30, 35, 74,  98,  77,  48, 2,  6,  39, 35, 34, 89,  62,
        79,  50, 0,  6,  34, 34, 35, 96,  0,   83,  54, -1, 6,  22, 24, 35, 80,  189, 98,  69, 1,  5,  23, 23, 35, 99,  134,
        100, 71, 0,  5,  23, 23, 35, 105, 97,  102, 73, -1, 5,  14, 17, 35, 125, 36,  104, 75, 1,  5,  23, 22, 35, 131, 0,
        105, 76, 1,  5,  11, 9,  35, 105, 170, 107, 78, 1,  5,  22, 22, 35, 105, 206, 108, 79, 1,  5,  11, 8,  35, 115, 170,
        33,  4,  3,  6,  14, 9,  34, 123, 133, 35,  6,  -1, 6,  24, 26, 34, 124, 168, 49,  20, 2,  6,  24, 22, 34, 128, 203,
        50,  21, 0,  6,  24, 24, 34, 129, 72,  52,  23, -1, 6,  24, 27, 34, 143, 36,  53,  24, 0,  6,  24, 23, 34, 154, 0,
        54,  25, 0,  6,  24, 24, 34, 133, 107, 55,  26, 0,  6,  24, 24, 34, 154, 71,  57,  28, 1,  6,  24, 24, 34, 171, 35,
        65,  36, -2, 6,  29, 32, 34, 178, 0,   66,  37, 2,  6,  27, 25, 34, 151, 142, 68,  39, 2,  6,  32, 30, 34, 158, 106,
        69,  40, 2,  6,  24, 23, 34, 179, 70,  70,  41, 2,  6,  24, 23, 34, 196, 35,  72,  43, 2,  6,  32, 28, 34, 211, 0,
        73,  44, 2,  6,  13, 8,  34, 151, 177, 74,  45, -1, 6,  19, 18, 34, 151, 212, 75,  46, 2,  6,  29, 28, 34, 160, 177,
        76,  47, 2,  6,  22, 21, 34, 177, 141, 78,  49, 2,  6,  32, 28, 34, 189, 105, 80,  51, 2,  6,  26, 24, 34, 203, 70,
        82,  53, 2,  6,  27, 26, 34, 220, 35,  84,  55, -1, 6,  25, 28, 34, 240, 0,   85,  56, 1,  6,  31, 28, 34, 170, 212,
        86,  57, -2, 6,  29, 32, 34, 189, 176, 88,  59, -2, 6,  27, 30, 34, 199, 140, 89,  60, -2, 6,  26, 30, 34, 218, 105,
        90,  61, 0,  6,  26, 27, 34, 228, 70,  103, 74, -1, 14, 21, 24, 33, 247, 35,  109, 80, 1,  14, 34, 33, 26, 269, 0,
        112, 83, 1,  14, 23, 23, 33, 199, 211, 113, 84, 0,  14, 23, 23, 33, 222, 175, 116, 87, -1, 8,  16, 18, 33, 230, 140,
        121, 92, -2, 15, 22, 25, 32, 223, 209, 59,  30, 1,  15, 11, 9,  30, 80,  225, 97,  68, 0,  14, 21, 21, 26, 246, 174,
        99,  70, 0,  14, 20, 21, 26, 249, 201, 101, 72, 0,  14, 22, 23, 26, 249, 228, 110, 81, 1,  14, 23, 22, 26, 249, 105,
        111, 82, 0,  14, 23, 24, 26, 256, 69,  114, 85, 1,  14, 17, 17, 26, 249, 132, 115, 86, -1, 14, 18, 19, 26, 272, 27,
        117, 88, 1,  15, 23, 22, 26, 267, 132, 43,  14, 0,  12, 24, 24, 25, 96,  36,  58,  29, 1,  15, 11, 9,  25, 84,  0,
        118, 89, -2, 15, 22, 25, 25, 272, 96,  120, 91, -1, 15, 21, 24, 25, 268, 159, 122, 93, -1, 15, 19, 20, 25, 290, 122,
        126, 97, 0,  20, 24, 23, 11, 199, 245, 60,  31, 1,  13, 24, 20, 22, 281, 54,  61,  32, 1,  17, 24, 22, 14, 128, 238,
        62,  33, 3,  13, 24, 20, 22, 292, 27,  94,  65, 1,  6,  24, 21, 18, 74,  134, 95,  66, -2, 39, 17, 21, 7,  151, 247,
        42,  13, 0,  4,  17, 17, 18, 133, 142, 34,  5,  1,  6,  17, 14, 15, 90,  225, 39,  10, 1,  6,  10, 7,  15, 268, 185,
        44,  15, 0,  31, 9,  9,  15, 64,  98,  45,  16, 0,  22, 15, 14, 7,  90,  241, 96,  67, -1, 6,  13, 12, 10, 49,  153,
        46,  17, 0,  31, 9,  9,  9,  272, 122, 32,  3,  -2, 36, 8,  0,  0,  8,   210,
    };
    int kerning_data[][3] = {
        34,  38,  -4, 34,  44,  -5, 34,  45,  -4, 34,  46,  -5, 34,  47,  -4, 34,  64,  -1, 34,  65,  -4, 34,  67,  -1,
        34,  71,  -1, 34,  79,  -1, 34,  81,  -1, 34,  86,  1,  34,  87,  1,  34,  89,  1,  34,  92,  1,  34,  97,  -1,
        34,  99,  -2, 34,  100, -2, 34,  101, -2, 34,  111, -2, 34,  113, -2, 39,  38,  -4, 39,  44,  -5, 39,  45,  -4,
        39,  46,  -5, 39,  47,  -4, 39,  64,  -1, 39,  65,  -4, 39,  67,  -1, 39,  71,  -1, 39,  79,  -1, 39,  81,  -1,
        39,  86,  1,  39,  87,  1,  39,  89,  1,  39,  92,  1,  39,  97,  -1, 39,  99,  -2, 39,  100, -2, 39,  101, -2,
        39,  111, -2, 39,  113, -2, 40,  64,  -1, 40,  67,  -1, 40,  71,  -1, 40,  79,  -1, 40,  81,  -1, 40,  99,  -1,
        40,  100, -1, 40,  101, -1, 40,  111, -1, 40,  113, -1, 42,  38,  -4, 42,  44,  -5, 42,  45,  -4, 42,  46,  -5,
        42,  47,  -4, 42,  64,  -1, 42,  65,  -4, 42,  67,  -1, 42,  71,  -1, 42,  79,  -1, 42,  81,  -1, 42,  86,  1,
        42,  87,  1,  42,  89,  1,  42,  92,  1,  42,  97,  -1, 42,  99,  -2, 42,  100, -2, 42,  101, -2, 42,  111, -2,
        42,  113, -2, 44,  34,  -5, 44,  39,  -5, 44,  42,  -5, 44,  45,  -3, 44,  64,  -1, 44,  67,  -1, 44,  71,  -1,
        44,  79,  -1, 44,  81,  -1, 44,  84,  -4, 44,  86,  -4, 44,  87,  -3, 44,  89,  -3, 44,  92,  -4, 44,  118, -3,
        44,  119, -1, 44,  121, -3, 45,  34,  -4, 45,  38,  -1, 45,  39,  -4, 45,  42,  -4, 45,  44,  -3, 45,  46,  -3,
        45,  47,  -1, 45,  65,  -1, 45,  84,  -4, 45,  86,  -2, 45,  87,  -1, 45,  88,  -1, 45,  89,  -3, 45,  90,  -1,
        45,  92,  -2, 46,  34,  -5, 46,  39,  -5, 46,  42,  -5, 46,  45,  -3, 46,  64,  -1, 46,  67,  -1, 46,  71,  -1,
        46,  79,  -1, 46,  81,  -1, 46,  84,  -4, 46,  86,  -4, 46,  87,  -3, 46,  89,  -3, 46,  92,  -4, 46,  118, -3,
        46,  119, -1, 46,  121, -3, 47,  34,  1,  47,  38,  -3, 47,  39,  1,  47,  42,  1,  47,  44,  -4, 47,  45,  -2,
        47,  46,  -4, 47,  47,  -3, 47,  58,  -2, 47,  59,  -2, 47,  63,  1,  47,  64,  -1, 47,  65,  -3, 47,  67,  -1,
        47,  71,  -1, 47,  74,  -3, 47,  79,  -1, 47,  81,  -1, 47,  97,  -2, 47,  99,  -2, 47,  100, -2, 47,  101, -2,
        47,  102, -1, 47,  103, -3, 47,  109, -2, 47,  110, -2, 47,  111, -2, 47,  112, -2, 47,  113, -2, 47,  114, -2,
        47,  115, -2, 47,  116, -1, 47,  117, -2, 47,  118, -1, 47,  120, -1, 47,  121, -1, 47,  122, -2, 64,  34,  -1,
        64,  38,  -1, 64,  39,  -1, 64,  41,  -1, 64,  42,  -1, 64,  44,  -1, 64,  46,  -1, 64,  47,  -1, 64,  65,  -1,
        64,  84,  -2, 64,  86,  -1, 64,  88,  -1, 64,  89,  -2, 64,  90,  -1, 64,  92,  -1, 64,  93,  -1, 64,  125, -1,
        65,  34,  -4, 65,  39,  -4, 65,  42,  -4, 65,  45,  -1, 65,  63,  -1, 65,  64,  -1, 65,  67,  -1, 65,  71,  -1,
        65,  74,  1,  65,  79,  -1, 65,  81,  -1, 65,  84,  -3, 65,  85,  -1, 65,  86,  -3, 65,  87,  -2, 65,  89,  -3,
        65,  92,  -3, 65,  118, -2, 65,  121, -2, 67,  45,  -3, 68,  34,  -1, 68,  38,  -1, 68,  39,  -1, 68,  41,  -1,
        68,  42,  -1, 68,  44,  -1, 68,  46,  -1, 68,  47,  -1, 68,  65,  -1, 68,  84,  -2, 68,  86,  -1, 68,  88,  -1,
        68,  89,  -2, 68,  90,  -1, 68,  92,  -1, 68,  93,  -1, 68,  125, -1, 70,  38,  -3, 70,  44,  -4, 70,  46,  -4,
        70,  47,  -3, 70,  58,  -1, 70,  59,  -1, 70,  63,  1,  70,  65,  -3, 70,  74,  -4, 70,  99,  -1, 70,  100, -1,
        70,  101, -1, 70,  109, -1, 70,  110, -1, 70,  111, -1, 70,  112, -1, 70,  113, -1, 70,  114, -1, 70,  117, -1,
        74,  38,  -1, 74,  44,  -1, 74,  46,  -1, 74,  47,  -1, 74,  65,  -1, 75,  45,  -1, 75,  64,  -1, 75,  67,  -1,
        75,  71,  -1, 75,  79,  -1, 75,  81,  -1, 75,  99,  -1, 75,  100, -1, 75,  101, -1, 75,  102, -1, 75,  111, -1,
        75,  113, -1, 75,  116, -2, 75,  118, -1, 75,  119, -1, 75,  121, -1, 76,  34,  -6, 76,  39,  -6, 76,  42,  -6,
        76,  44,  1,  76,  45,  -4, 76,  46,  1,  76,  63,  -1, 76,  64,  -2, 76,  67,  -2, 76,  71,  -2, 76,  79,  -2,
        76,  81,  -2, 76,  84,  -4, 76,  86,  -4, 76,  87,  -3, 76,  89,  -4, 76,  92,  -4, 76,  99,  -1, 76,  100, -1,
        76,  101, -1, 76,  111, -1, 76,  113, -1, 76,  118, -2, 76,  119, -2, 76,  121, -2, 79,  34,  -1, 79,  38,  -1,
        79,  39,  -1, 79,  41,  -1, 79,  42,  -1, 79,  44,  -1, 79,  46,  -1, 79,  47,  -1, 79,  65,  -1, 79,  84,  -2,
        79,  86,  -1, 79,  88,  -1, 79,  89,  -2, 79,  90,  -1, 79,  92,  -1, 79,  93,  -1, 79,  125, -1, 80,  38,  -3,
        80,  44,  -5, 80,  46,  -5, 80,  47,  -3, 80,  65,  -3, 80,  74,  -4, 80,  97,  -1, 80,  99,  -1, 80,  100, -1,
        80,  101, -1, 80,  111, -1, 80,  113, -1, 81,  34,  -1, 81,  38,  -1, 81,  39,  -1, 81,  41,  -1, 81,  42,  -1,
        81,  44,  -1, 81,  46,  -1, 81,  47,  -1, 81,  65,  -1, 81,  84,  -2, 81,  86,  -1, 81,  88,  -1, 81,  89,  -2,
        81,  90,  -1, 81,  92,  -1, 81,  93,  -1, 81,  125, -1, 82,  64,  -1, 82,  67,  -1, 82,  71,  -1, 82,  79,  -1,
        82,  81,  -1, 82,  84,  -1, 82,  85,  -1, 84,  38,  -3, 84,  44,  -4, 84,  45,  -4, 84,  46,  -4, 84,  47,  -3,
        84,  58,  -3, 84,  59,  -3, 84,  64,  -2, 84,  65,  -3, 84,  67,  -2, 84,  71,  -2, 84,  74,  -4, 84,  79,  -2,
        84,  81,  -2, 84,  97,  -5, 84,  99,  -4, 84,  100, -4, 84,  101, -4, 84,  103, -4, 84,  109, -3, 84,  110, -3,
        84,  111, -4, 84,  112, -3, 84,  113, -4, 84,  114, -3, 84,  115, -3, 84,  117, -3, 84,  118, -4, 84,  119, -3,
        84,  120, -3, 84,  121, -4, 84,  122, -3, 85,  38,  -1, 85,  44,  -1, 85,  46,  -1, 85,  47,  -1, 85,  65,  -1,
        86,  34,  1,  86,  38,  -3, 86,  39,  1,  86,  42,  1,  86,  44,  -4, 86,  45,  -2, 86,  46,  -4, 86,  47,  -3,
        86,  58,  -2, 86,  59,  -2, 86,  63,  1,  86,  64,  -1, 86,  65,  -3, 86,  67,  -1, 86,  71,  -1, 86,  74,  -3,
        86,  79,  -1, 86,  81,  -1, 86,  97,  -2, 86,  99,  -2, 86,  100, -2, 86,  101, -2, 86,  102, -1, 86,  103, -3,
        86,  109, -2, 86,  110, -2, 86,  111, -2, 86,  112, -2, 86,  113, -2, 86,  114, -2, 86,  115, -2, 86,  116, -1,
        86,  117, -2, 86,  118, -1, 86,  120, -1, 86,  121, -1, 86,  122, -2, 87,  34,  1,  87,  38,  -2, 87,  39,  1,
        87,  42,  1,  87,  44,  -3, 87,  45,  -1, 87,  46,  -3, 87,  47,  -2, 87,  63,  1,  87,  65,  -2, 87,  74,  -2,
        87,  97,  -2, 87,  99,  -1, 87,  100, -1, 87,  101, -1, 87,  103, -2, 87,  111, -1, 87,  113, -1, 87,  115, -1,
        88,  45,  -1, 88,  64,  -1, 88,  67,  -1, 88,  71,  -1, 88,  79,  -1, 88,  81,  -1, 88,  99,  -1, 88,  100, -1,
        88,  101, -1, 88,  102, -1, 88,  111, -1, 88,  113, -1, 88,  116, -2, 88,  118, -1, 88,  119, -1, 88,  121, -1,
        89,  34,  1,  89,  38,  -3, 89,  39,  1,  89,  42,  1,  89,  44,  -3, 89,  45,  -3, 89,  46,  -3, 89,  47,  -3,
        89,  58,  -3, 89,  59,  -3, 89,  63,  1,  89,  64,  -2, 89,  65,  -3, 89,  67,  -2, 89,  71,  -2, 89,  74,  -4,
        89,  79,  -2, 89,  81,  -2, 89,  97,  -3, 89,  99,  -3, 89,  100, -3, 89,  101, -3, 89,  103, -4, 89,  109, -3,
        89,  110, -3, 89,  111, -3, 89,  112, -3, 89,  113, -3, 89,  114, -3, 89,  115, -3, 89,  117, -3, 89,  118, -2,
        89,  119, -2, 89,  120, -3, 89,  121, -2, 90,  45,  -1, 90,  63,  1,  90,  64,  -1, 90,  67,  -1, 90,  71,  -1,
        90,  79,  -1, 90,  81,  -1, 90,  99,  -1, 90,  100, -1, 90,  101, -1, 90,  111, -1, 90,  113, -1, 90,  115, -1,
        90,  118, -1, 90,  121, -1, 91,  64,  -1, 91,  67,  -1, 91,  71,  -1, 91,  79,  -1, 91,  81,  -1, 91,  99,  -1,
        91,  100, -1, 91,  101, -1, 91,  111, -1, 91,  113, -1, 92,  34,  -4, 92,  39,  -4, 92,  42,  -4, 92,  45,  -1,
        92,  63,  -1, 92,  64,  -1, 92,  67,  -1, 92,  71,  -1, 92,  74,  1,  92,  79,  -1, 92,  81,  -1, 92,  84,  -3,
        92,  85,  -1, 92,  86,  -3, 92,  87,  -2, 92,  89,  -3, 92,  92,  -3, 92,  118, -2, 92,  121, -2, 97,  34,  -2,
        97,  39,  -2, 97,  42,  -2, 97,  118, -1, 97,  119, 0,  97,  121, -1, 98,  34,  -2, 98,  39,  -2, 98,  41,  -1,
        98,  42,  -2, 98,  86,  -2, 98,  87,  -1, 98,  92,  -2, 98,  93,  -1, 98,  118, -1, 98,  120, -1, 98,  121, -1,
        98,  125, -1, 101, 34,  -2, 101, 39,  -2, 101, 41,  -1, 101, 42,  -2, 101, 86,  -2, 101, 87,  -1, 101, 92,  -2,
        101, 93,  -1, 101, 118, -1, 101, 120, -1, 101, 121, -1, 101, 125, -1, 102, 34,  1,  102, 39,  1,  102, 42,  1,
        102, 44,  -3, 102, 46,  -3, 104, 34,  -2, 104, 39,  -2, 104, 42,  -2, 104, 118, -1, 104, 119, 0,  104, 121, -1,
        107, 99,  -1, 107, 100, -1, 107, 101, -1, 107, 111, -1, 107, 113, -1, 109, 34,  -2, 109, 39,  -2, 109, 42,  -2,
        109, 118, -1, 109, 119, 0,  109, 121, -1, 110, 34,  -2, 110, 39,  -2, 110, 42,  -2, 110, 118, -1, 110, 119, 0,
        110, 121, -1, 111, 34,  -2, 111, 39,  -2, 111, 41,  -1, 111, 42,  -2, 111, 86,  -2, 111, 87,  -1, 111, 92,  -2,
        111, 93,  -1, 111, 118, -1, 111, 120, -1, 111, 121, -1, 111, 125, -1, 112, 34,  -2, 112, 39,  -2, 112, 41,  -1,
        112, 42,  -2, 112, 86,  -2, 112, 87,  -1, 112, 92,  -2, 112, 93,  -1, 112, 118, -1, 112, 120, -1, 112, 121, -1,
        112, 125, -1, 114, 44,  -3, 114, 46,  -3, 114, 97,  -1, 118, 38,  -2, 118, 44,  -3, 118, 46,  -3, 118, 47,  -2,
        118, 65,  -2, 118, 99,  -1, 118, 100, -1, 118, 101, -1, 118, 111, -1, 118, 113, -1, 119, 44,  -1, 119, 46,  -1,
        120, 99,  -1, 120, 100, -1, 120, 101, -1, 120, 111, -1, 120, 113, -1, 121, 38,  -2, 121, 44,  -3, 121, 46,  -3,
        121, 47,  -2, 121, 65,  -2, 121, 99,  -1, 121, 100, -1, 121, 101, -1, 121, 111, -1, 121, 113, -1, 123, 64,  -1,
        123, 67,  -1, 123, 71,  -1, 123, 79,  -1, 123, 81,  -1, 123, 99,  -1, 123, 100, -1, 123, 101, -1, 123, 111, -1,
        123, 113, -1,
    };

    int char_count = count_of(char_data);
    for (int i = 0; i < char_count; ++i) {
        int *g_data = char_data[i];
        msdf_glyph g = (msdf_glyph){.id = g_data[0],
                                    .index = g_data[1],
                                    .xoffset = g_data[2],
                                    .yoffset = g_data[3],
                                    .xadvance = g_data[4],
                                    .width = g_data[5],
                                    .height = g_data[6],
                                    .x = g_data[7],
                                    .y = g_data[8]};
        hmput(system_font.char_map, g_data[0], g);
    }

    int kerning_count = count_of(kerning_data);
    for (int i = 0; i < kerning_count; ++i) {
        int *k_data = kerning_data[i];
        kerning_key key = (kerning_key){.first = k_data[0], .second = k_data[1]};
        hmput(system_font.kerning_map, key, k_data[2]);
    }

#if defined(OS_MACOS) || defined (OS_IOS)
    ustring image_path = os_get_bundle_path(ustring_STR("Contents/Resources/public/font/Lato-Regular.png"));
#else
    ustring image_path = ustring_STR("public/font/Lato-Regular.png");
#endif

    int width, height, channel;
    u8 *data = io_load_image(ustring_view_from_ustring(image_path), &width, &height, &channel, 4);
    
    gpu_texture_desc desc = { .width = width, .height = height };
    desc.format = PIXELFORMAT_RGBA8;
    desc.data = (udata){.data = (i8*)data, .length = width * height * 4};
    system_font.texture = gpu_create_texture(&desc);
    return &system_font;
}
