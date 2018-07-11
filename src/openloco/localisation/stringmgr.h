#pragma once

#include <cstdint>

#ifdef small
#error "small is defined, likely by windows.h"
#endif

namespace openloco
{
    using string_id = uint16_t;

    namespace string_ids
    {
        constexpr string_id null = 0xFFFF;
    }

    namespace font
    {
        constexpr int16_t m1 = -1;
        constexpr int16_t m2 = -2;

        constexpr int16_t medium_normal = 0;
        constexpr int16_t medium_bold = 224;
        constexpr int16_t small = 448;
        constexpr int16_t large = 672;
    }

    namespace format_flags
    {
        constexpr uint8_t textflag_5 = (1ULL << 5); // 0x20
        constexpr uint8_t textflag_6 = (1ULL << 6); // 0x40

        constexpr uint8_t fd = 0xFD;
        constexpr uint8_t fe = 0xFE;
    }

    namespace control_code
    {
        // Arguments: uint8_t
        constexpr uint8_t move_x = 1;

        constexpr uint8_t adjust_palette = 2;
        constexpr uint8_t newline = 5;
        constexpr uint8_t newline_smaller = 6;
        constexpr uint8_t font_small = 7;
        constexpr uint8_t font_large = 8;
        constexpr uint8_t font_bold = 9;
        constexpr uint8_t font_regular = 10;
        constexpr uint8_t outline = 11;
        constexpr uint8_t outline_off = 12;
        constexpr uint8_t window_colour_1 = 13;
        constexpr uint8_t window_colour_2 = 14;
        constexpr uint8_t window_colour_3 = 15;

        // Arguments: int8_t, int8_t
        constexpr uint8_t newline_x_y = 17;

        // Arguments: uint32_t
        constexpr uint8_t inline_sprite = 23;

        constexpr uint8_t colour_black = 144;
        constexpr uint8_t colour_grey = 145;
        constexpr uint8_t colour_white = 146;
        constexpr uint8_t colour_red = 147;
        constexpr uint8_t colour_green = 148;
        constexpr uint8_t colour_yellow = 149;
        constexpr uint8_t colour_topaz = 150;
        constexpr uint8_t colour_celadon = 151;
        constexpr uint8_t colour_babyblue = 152;
        constexpr uint8_t colour_palelavender = 153;
        constexpr uint8_t colour_palegold = 154;
        constexpr uint8_t colour_lightpink = 155;
        constexpr uint8_t colour_pearlaqua = 156;
        constexpr uint8_t colour_palesilver = 157;
    }
}

namespace openloco::stringmgr
{
    const char* get_string(string_id id);
    char* format_string(char* buffer, string_id id, void* args);
}
