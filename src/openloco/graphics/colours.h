#pragma once

#include <cstdint>

namespace openloco
{
    using colour_t = uint8_t;
    using palette_index_t = uint8_t;

    namespace colour
    {
        constexpr uint8_t translucent_flag = 1 << 7;

        constexpr colour_t black = 0;
        constexpr colour_t grey = 1;
        constexpr colour_t white = 2;
        constexpr colour_t dark_purple = 3;
        constexpr colour_t light_purple = 4;
        constexpr colour_t bright_purple = 5;
        constexpr colour_t dark_blue = 6;
        constexpr colour_t light_blue = 7;
        constexpr colour_t icy_blue = 8;
        constexpr colour_t teal = 9;
        constexpr colour_t aquamarine = 10;
        constexpr colour_t saturated_green = 11;
        constexpr colour_t dark_green = 12;
        constexpr colour_t moss_green = 13;
        constexpr colour_t bright_green = 14;
        constexpr colour_t olive_green = 15;
        constexpr colour_t dark_olive_green = 16;
        constexpr colour_t bright_yellow = 17;
        constexpr colour_t yellow = 18;
        constexpr colour_t dark_yellow = 19;
        constexpr colour_t light_orange = 20;
        constexpr colour_t dark_orange = 21;
        constexpr colour_t light_brown = 22;
        constexpr colour_t saturated_brown = 23;
        constexpr colour_t dark_brown = 24;
        constexpr colour_t salmon_pink = 25;
        constexpr colour_t bordeaux_red = 26;
        constexpr colour_t saturated_red = 27;
        constexpr colour_t bright_red = 28;
        constexpr colour_t dark_pink = 29;
        constexpr colour_t bright_pink = 30;
        constexpr colour_t light_pink = 31;

        constexpr colour_t translucent(colour_t c)
        {
            return c | translucent_flag;
        }

        constexpr colour_t opaque(colour_t c)
        {
            return c & ~translucent_flag;
        }

        void init_colour_map();
        palette_index_t get_shade(colour_t colour, uint8_t shade);
    }

    namespace palette_index
    {
        constexpr palette_index_t transparent = 0;
        constexpr palette_index_t index_0A = 0x0A;
    }
}
