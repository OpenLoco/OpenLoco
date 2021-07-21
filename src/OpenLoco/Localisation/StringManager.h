#pragma once

#include "../Types.hpp"
#include <cstddef>
#include <cstdint>

#ifdef small
#error "small is defined, likely by windows.h"
#endif

namespace OpenLoco
{
    namespace StringIds
    {
        constexpr string_id null = 0xFFFF;
    }

    namespace Font
    {
        constexpr int16_t m1 = -1;
        constexpr int16_t m2 = -2;

        constexpr int16_t medium_normal = 0;
        constexpr int16_t medium_bold = 224;
        constexpr int16_t small = 448;
        constexpr int16_t large = 672;
    }

    namespace FormatFlags
    {
        constexpr uint8_t textflag_5 = (1ULL << 5); // 0x20
        constexpr uint8_t textflag_6 = (1ULL << 6); // 0x40

        constexpr uint8_t fd = 0xFD;
        constexpr uint8_t fe = 0xFE;
        constexpr uint8_t ff = 0xFF;
    }

    namespace ControlCodes
    {
        // Arguments (1 byte): uint8_t
        constexpr uint8_t move_x = 1;
        constexpr uint8_t adjust_palette = 2; // Not used
        // 3 Not used
        // 4 Not used

        // Arguements: none
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
        constexpr uint8_t window_colour_4 = 16; // Not used

        // Arguments (2 bytes): int8_t, int8_t
        constexpr uint8_t newline_x_y = 17;
        // 18 Not used
        // 19 Not used
        // 20 Not used
        // 21 Not used
        // 22 Not used

        // Arguments (4 bytes): uint32_t
        constexpr uint8_t inline_sprite_str = 23;
        // 24 Not used
        // 25 Not used
        // 26 Not used
        // 27 Not used
        // 28 Not used
        // 29 Not used
        // 30 Not used
        // 31 Not used

        // Arguments in Args buffer
        // Note:
        // Pre formatString:
        //     ControlCodes valid args in args buffer.
        // Post formatString:
        //     ControlCodes are invalid
        //     inline_sprite_args replaced with inline_sprite_str, arg is in string
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

        // Arguments: none
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

    namespace DateModifier
    {
        constexpr uint8_t dmy_full = 0;
        constexpr uint8_t my_full = 4;
        constexpr uint8_t my_abbr = 5;
        constexpr uint8_t raw_my_abbr = 8;
    }
}

namespace OpenLoco::StringManager
{
    void reset();
    const char* getString(string_id id);
    char* formatString(char* buffer, string_id id, const void* args = nullptr);
    char* formatString(char* buffer, size_t bufferLen, string_id id, const void* args = nullptr);
    string_id userStringAllocate(char* str, uint8_t cl);
    void emptyUserString(string_id stringId);
    string_id isTownName(string_id stringId);
    string_id toTownName(string_id stringId);
    string_id fromTownName(string_id stringId);
}
