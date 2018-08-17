#pragma once

#include <cstdint>

namespace openloco
{
    using colour_t = uint8_t;
    using palette_index_t = uint8_t;

    namespace colour
    {
        constexpr uint8_t translucent_flag = 1 << 7;

        constexpr colour_t darkGrey = 0;
        constexpr colour_t grey = 1;
        constexpr colour_t lightGrey = 2;
        constexpr colour_t darkViolet = 3;
        constexpr colour_t violet = 4;
        constexpr colour_t purple = 5;
        constexpr colour_t darkBlue = 6;
        constexpr colour_t blue = 7;
        constexpr colour_t turquoise = 8;
        constexpr colour_t pastelBlue = 9;
        constexpr colour_t darkGreen = 10;
        constexpr colour_t palegreen = 11;
        constexpr colour_t mossGreen = 12;
        constexpr colour_t green = 13;
        constexpr colour_t lightOliveGreen = 14;
        constexpr colour_t darkOliveGreen = 15;
        constexpr colour_t yellow = 16;
        constexpr colour_t darkYellow = 17;
        constexpr colour_t orange = 18;
        constexpr colour_t amber = 19;
        constexpr colour_t darkOrange = 20;
        constexpr colour_t sandBrown = 21;
        constexpr colour_t darkSandBrown = 22;
        constexpr colour_t darkBrown = 23;
        constexpr colour_t beigeRed = 24;
        constexpr colour_t bordeauxRed = 25;
        constexpr colour_t darkRed = 26;
        constexpr colour_t red = 27;
        constexpr colour_t darkMagenta = 28;
        constexpr colour_t magenta = 29;
        constexpr colour_t salmonPink = 30;

        constexpr colour_t translucent(colour_t c)
        {
            return c | translucent_flag;
        }

        constexpr colour_t opaque(colour_t c)
        {
            return c & ~translucent_flag;
        }

        palette_index_t get_shade(colour_t colour, uint8_t shade);
    }

    namespace palette_index
    {
        constexpr palette_index_t transparent = 0;
        constexpr palette_index_t index_0A = 0x0A;
    }
}
