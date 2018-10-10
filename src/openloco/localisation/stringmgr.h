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
        constexpr uint8_t ff = 0xFF;
    }

    namespace control_codes
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
        constexpr uint8_t inline_sprite_str = 23;

        constexpr uint8_t int32_grouped = 123 + 0;
        constexpr uint8_t int32_ungrouped = 123 + 1;
        constexpr uint8_t int16_decimals = 123 + 2;
        constexpr uint8_t int32_decimals = 123 + 3;
        constexpr uint8_t int16_grouped = 123 + 4;
        constexpr uint8_t uint16_ungrouped = 123 + 5;
        constexpr uint8_t currency32 = 123 + 6;
        constexpr uint8_t currency48 = 123 + 7;
        constexpr uint8_t stringid_args = 123 + 8;
        constexpr uint8_t stringid_str = 123 + 9;
        constexpr uint8_t string_ptr = 123 + 10;
        constexpr uint8_t date = 123 + 11;
        constexpr uint8_t velocity = 123 + 12;
        constexpr uint8_t pop16 = 123 + 13;
        constexpr uint8_t push16 = 123 + 14;
        constexpr uint8_t timeMS = 123 + 15;
        constexpr uint8_t timeHM = 123 + 16;
        constexpr uint8_t distance = 123 + 17;
        constexpr uint8_t height = 123 + 18;
        constexpr uint8_t power = 123 + 19;
        constexpr uint8_t inline_sprite_args = 123 + 20;

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

    namespace date_modifier
    {
        constexpr uint8_t dmy_full = 0;
        constexpr uint8_t my_full = 4;
        constexpr uint8_t my_abbr = 5;
        constexpr uint8_t raw_my_abbr = 8;
    }
}

namespace openloco::stringmgr
{
    const char* get_string(string_id id);
    char* format_string(char* buffer, string_id id, void* args = nullptr);
}
