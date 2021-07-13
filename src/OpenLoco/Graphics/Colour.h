#pragma once

#include <cstdint>

namespace OpenLoco
{
    using Colour_t = uint8_t;
    using PaletteIndex_t = uint8_t;

    namespace Colour
    {
        constexpr uint8_t outline_flag = 1 << 5;
        constexpr uint8_t inset_flag = 1 << 6;
        constexpr uint8_t translucent_flag = 1 << 7;

        constexpr Colour_t black = 0;
        constexpr Colour_t grey = 1;
        constexpr Colour_t white = 2;
        constexpr Colour_t dark_purple = 3;
        constexpr Colour_t light_purple = 4;
        constexpr Colour_t bright_purple = 5;
        constexpr Colour_t dark_blue = 6;
        constexpr Colour_t light_blue = 7;
        constexpr Colour_t icy_blue = 8;
        constexpr Colour_t teal = 9;
        constexpr Colour_t aquamarine = 10;
        constexpr Colour_t saturated_green = 11;
        constexpr Colour_t dark_green = 12;
        constexpr Colour_t moss_green = 13;
        constexpr Colour_t bright_green = 14;
        constexpr Colour_t olive_green = 15;
        constexpr Colour_t dark_olive_green = 16;
        constexpr Colour_t bright_yellow = 17;
        constexpr Colour_t yellow = 18;
        constexpr Colour_t dark_yellow = 19;
        constexpr Colour_t light_orange = 20;
        constexpr Colour_t dark_orange = 21;
        constexpr Colour_t light_brown = 22;
        constexpr Colour_t saturated_brown = 23;
        constexpr Colour_t dark_brown = 24;
        constexpr Colour_t salmon_pink = 25;
        constexpr Colour_t bordeaux_red = 26;
        constexpr Colour_t saturated_red = 27;
        constexpr Colour_t bright_red = 28;
        constexpr Colour_t dark_pink = 29;
        constexpr Colour_t bright_pink = 30;
        constexpr Colour_t light_pink = 31;

        constexpr Colour_t outline(Colour_t c)
        {
            return c | outline_flag;
        }

        constexpr Colour_t inset(Colour_t c)
        {
            return c | inset_flag;
        }

        constexpr Colour_t translucent(Colour_t c)
        {
            return c | translucent_flag;
        }

        constexpr Colour_t opaque(Colour_t c)
        {
            return c & ~translucent_flag;
        }

        void initColourMap();
        PaletteIndex_t getShade(Colour_t colour, uint8_t shade);
    }

    namespace PaletteIndex
    {
        constexpr PaletteIndex_t transparent = 0;
        constexpr PaletteIndex_t index_0A = 0x0A;
        constexpr PaletteIndex_t index_0C = 0x0C;
        constexpr PaletteIndex_t index_0E = 0x0E;
        constexpr PaletteIndex_t index_11 = 0x11;
        constexpr PaletteIndex_t index_12 = 0x12;
        constexpr PaletteIndex_t index_15 = 0x15;
        constexpr PaletteIndex_t index_1F = 0x1F;
        constexpr PaletteIndex_t index_24 = 0x24;
        constexpr PaletteIndex_t index_29 = 0x29;
        constexpr PaletteIndex_t index_2C = 0x2C;
        constexpr PaletteIndex_t index_2E = 0x2E;
        constexpr PaletteIndex_t index_30 = 0x30;
        constexpr PaletteIndex_t index_31 = 0x31;
        constexpr PaletteIndex_t index_32 = 0x32;
        constexpr PaletteIndex_t index_35 = 0x35;
        constexpr PaletteIndex_t index_38 = 0x38;
        constexpr PaletteIndex_t index_3B = 0x3B;
        constexpr PaletteIndex_t index_3F = 0x3F;
        constexpr PaletteIndex_t index_41 = 0x41;
        constexpr PaletteIndex_t index_43 = 0x43;
        constexpr PaletteIndex_t index_4B = 0x4B;
        constexpr PaletteIndex_t index_50 = 0x50;
        constexpr PaletteIndex_t index_58 = 0x58;
        constexpr PaletteIndex_t index_64 = 0x64;
        constexpr PaletteIndex_t index_66 = 0x66;
        constexpr PaletteIndex_t index_67 = 0x67;
        constexpr PaletteIndex_t index_68 = 0x68;
        constexpr PaletteIndex_t index_71 = 0x71;
        constexpr PaletteIndex_t index_74 = 0x74;
        constexpr PaletteIndex_t index_7D = 0x7D;
        constexpr PaletteIndex_t index_85 = 0x85;
        constexpr PaletteIndex_t index_89 = 0x89;
        constexpr PaletteIndex_t index_9D = 0x9D;
        constexpr PaletteIndex_t index_A1 = 0xA1;
        constexpr PaletteIndex_t index_A2 = 0xA2;
        constexpr PaletteIndex_t index_A3 = 0xA3;
        constexpr PaletteIndex_t index_AC = 0xAC;
        constexpr PaletteIndex_t index_AD = 0xAD;
        constexpr PaletteIndex_t index_B8 = 0xB8;
        constexpr PaletteIndex_t index_BA = 0xBA;
        constexpr PaletteIndex_t index_BB = 0xBB;
        constexpr PaletteIndex_t index_BC = 0xBC;
        constexpr PaletteIndex_t index_C3 = 0xC3;
        constexpr PaletteIndex_t index_C6 = 0xC6;
        constexpr PaletteIndex_t index_D0 = 0xD0;
        constexpr PaletteIndex_t index_D3 = 0xD3;
        constexpr PaletteIndex_t index_DB = 0xDB;
        constexpr PaletteIndex_t index_DE = 0xDE;
    }
}
